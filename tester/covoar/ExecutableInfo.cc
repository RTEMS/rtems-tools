/*! @file ExecutableInfo.cc
 *  @brief ExecutableInfo Implementation
 *
 *  This file contains the implementation of the functionality
 *  of the ExecutableInfo class.
 */

#include <stdio.h>

#include <rld.h>

#include "ExecutableInfo.h"
#include "ObjdumpProcessor.h"
#include "CoverageMap.h"
#include "SymbolTable.h"

namespace Coverage {

  ExecutableInfo::ExecutableInfo(
    const char* const  theExecutableName,
    const std::string& theLibraryName,
    bool               verbose,
    DesiredSymbols&    symbolsToAnalyze
    ) : fileName(theExecutableName),
        loadAddress(0),
        symbolsToAnalyze_m(symbolsToAnalyze)
  {
    if ( !theLibraryName.empty() )
      libraryName = theLibraryName;

    if (verbose) {
      std::cerr << "Loading executable " << theExecutableName;
      if ( !theLibraryName.empty() )
        std::cerr << " (" << theLibraryName << ')';
      std::cerr << std::endl;
    }

    rld::files::object executable(theExecutableName);

    executable.open();
    executable.begin();
    executable.load_symbols(symbols);

    rld::dwarf::file debug;

    debug.begin(executable.elf());
    debug.load_debug();
    debug.load_functions();

    for (auto& cu : debug.get_cus()) {
      AddressLineRange& range = mapper.makeRange(cu.pc_low(), cu.pc_high());
      // Does not filter on desired symbols under the assumption that the test
      // code and any support code is small relative to what is being tested.
      for (const auto &address : cu.get_addresses()) {
        range.addSourceLine(address);
      }

      for (auto& func : cu.get_functions()) {
        if (!func.has_machine_code()) {
          continue;
        }

        if (!symbolsToAnalyze_m.isDesired(func.name())) {
          continue;
        }

        if (func.is_inlined()) {
          if (func.is_external()) {
            // Flag it
            std::cerr << "Function is both external and inlined: "
                      << func.name() << std::endl;
          }

          if (func.has_entry_pc()) {
            continue;
          }

          // If the low PC address is zero, the symbol does not appear in
          // this executable.
          if (func.pc_low() == 0) {
            continue;
          }
        }

        // We can't process a zero size function.
        if (func.pc_high() == 0) {
          continue;
        }

        createCoverageMap (cu.name(), func.name(),
                            func.pc_low(), func.pc_high() - 1);
      }
    }
  }

  ExecutableInfo::~ExecutableInfo()
  {
  }

  void ExecutableInfo::dumpCoverageMaps( void ) {
    ExecutableInfo::CoverageMaps::iterator  itr;

    for (auto& cm : coverageMaps) {
      std::cerr << "Coverage Map for " << cm.first << std::endl;
      cm.second->dump();
    }
  }

  void ExecutableInfo::dumpExecutableInfo( void ){
    std::cout << std::endl
              << "== Executable info ==" << std::endl
              << "executable = " << getFileName () << std::endl
              << "library = " << libraryName << std::endl
              << "loadAddress = " << loadAddress << std::endl;
    theSymbolTable.dumpSymbolTable();
  }

  CoverageMapBase* ExecutableInfo::getCoverageMap ( uint32_t address )
  {
    CoverageMapBase*       aCoverageMap = NULL;
    CoverageMaps::iterator it;
    std::string            itsSymbol;

    // Obtain the coverage map containing the specified address.
    itsSymbol = theSymbolTable.getSymbol( address );
    if (itsSymbol != "") {
      aCoverageMap = &findCoverageMap(itsSymbol);
    }

    return aCoverageMap;
  }

  const std::string& ExecutableInfo::getFileName ( void ) const
  {
    return fileName;
  }

  const std::string ExecutableInfo::getLibraryName( void ) const
  {
    return libraryName;
  }

  uint32_t ExecutableInfo::getLoadAddress( void ) const
  {
    return loadAddress;
  }

  SymbolTable* ExecutableInfo::getSymbolTable ( void )
  {
    return &theSymbolTable;
  }

  CoverageMapBase& ExecutableInfo::findCoverageMap(
    const std::string& symbolName
  )
  {
    CoverageMaps::iterator cmi = coverageMaps.find( symbolName );
    if ( cmi == coverageMaps.end() )
      throw CoverageMapNotFoundError(symbolName);
    return *(cmi->second);
  }

  void ExecutableInfo::createCoverageMap (
    const std::string& fileName,
    const std::string& symbolName,
    uint32_t           lowAddress,
    uint32_t           highAddress
  )
  {
    CoverageMapBase        *theMap;
    CoverageMaps::iterator  itr;

    if ( lowAddress > highAddress ) {
      std::ostringstream what;
      what << "Low address is greater than high address for symbol "
            << symbolName
            << " (" << lowAddress
            << " and " << highAddress
            << ")";
      throw rld::error( what, "ExecutableInfo::createCoverageMap" );
    }

    itr = coverageMaps.find( symbolName );
    if ( itr == coverageMaps.end() ) {
      theMap = new CoverageMap( fileName, lowAddress, highAddress );
      coverageMaps[ symbolName ] = theMap;
    } else {
      theMap = itr->second;
      theMap->Add( lowAddress, highAddress );
    }
  }

  void ExecutableInfo::getSourceAndLine(
    const unsigned int address,
    std::string&       line
  )
  {
    std::string file;
    int         lno;
    mapper.getSource (address, file, lno);
    std::ostringstream ss;
    ss << file << ':' << lno;
    line = ss.str ();
  }

  bool ExecutableInfo::hasDynamicLibrary( void )
  {
    return !libraryName.empty();
  }

  void ExecutableInfo::mergeCoverage( void ) {
    for (auto& cm : coverageMaps) {
      if (symbolsToAnalyze_m.isDesired( cm.first ))
        symbolsToAnalyze_m.mergeCoverageMap( cm.first, cm.second );
    }
  }

  void ExecutableInfo::setLoadAddress( uint32_t address )
  {
    loadAddress = address;
  }

}
