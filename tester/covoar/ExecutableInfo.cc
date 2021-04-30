/*! @file ExecutableInfo.cc
 *  @brief ExecutableInfo Implementation
 *
 *  This file contains the implementation of the functionality
 *  of the ExecutableInfo class.
 */

#include <stdio.h>

#include <rld.h>

#include "ExecutableInfo.h"
#include "app_common.h"
#include "CoverageMap.h"
#include "DesiredSymbols.h"
#include "SymbolTable.h"

namespace Coverage {

  ExecutableInfo::ExecutableInfo(
    const char* const theExecutableName,
    const char* const theLibraryName,
    bool              verbose
    ) : fileName(theExecutableName),
        loadAddress(0)
  {
    if (theLibraryName != nullptr)
      libraryName = theLibraryName;

    if (verbose) {
      std::cerr << "Loading executable " << theExecutableName;
      if (theLibraryName != nullptr)
        std::cerr << " (" << theLibraryName << ')';
      std::cerr << std::endl;
    }

    rld::files::object executable(theExecutableName);

    executable.open();
    executable.begin();
    executable.load_symbols(symbols);

    debug.begin(executable.elf());
    debug.load_debug();
    debug.load_functions();

    try {
      for (auto& cu : debug.get_cus()) {
        for (auto& func : cu.get_functions()) {
          if (!func.has_machine_code()) {
            continue;
          }

          if (!SymbolsToAnalyze->isDesired(func.name())) {
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
    } catch (...) {
      debug.end();
      throw;
    }

    // Can't cleanup handles until the destructor because the information is
    // referenced elsewhere. NOTE: This could cause problems from too many open
    // file descriptors.
  }

  ExecutableInfo::~ExecutableInfo()
  {
    debug.end();
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
    debug.get_source (address, file, lno);
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
      if (SymbolsToAnalyze->isDesired( cm.first ))
        SymbolsToAnalyze->mergeCoverageMap( cm.first, cm.second );
    }
  }

  void ExecutableInfo::setLoadAddress( uint32_t address )
  {
    loadAddress = address;
  }

}
