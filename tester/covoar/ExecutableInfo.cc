/*! @file ExecutableInfo.cc
 *  @brief ExecutableInfo Implementation
 *
 *  This file contains the implementation of the functionality
 *  of the ExecutableInfo class.
 */

#include <stdio.h>

#include "ExecutableInfo.h"
#include "app_common.h"
#include "CoverageMap.h"
#include "DesiredSymbols.h"
#include "SymbolTable.h"

namespace Coverage {

  ExecutableInfo::ExecutableInfo(
    const char* const theExecutableName,
    const char* const theLibraryName
  )
  {
    executableName = theExecutableName;
    loadAddress = 0;
    libraryName = "";
    if (theLibraryName)
      libraryName = theLibraryName;
    theSymbolTable = new SymbolTable();
  }

  ExecutableInfo::~ExecutableInfo()
  {
    if (theSymbolTable)
      delete theSymbolTable;
  }

  void ExecutableInfo::dumpCoverageMaps( void ) {
    ExecutableInfo::coverageMaps_t::iterator  itr;

    for (itr = coverageMaps.begin(); itr != coverageMaps.end(); itr++) {
      fprintf( stderr, "Coverage Map for %s\n", ((*itr).first).c_str() );;
      ((*itr).second)->dump();
    }
  }

  void ExecutableInfo::dumpExecutableInfo( void ){
    fprintf( stdout, "\n== Executable info ==\n");
    fprintf( stdout, "executableName = %s\n", executableName.c_str());
    fprintf( stdout, "libraryName = %s\n", libraryName.c_str());
    fprintf( stdout, "loadAddress = %u\n", loadAddress);
    theSymbolTable->dumpSymbolTable();
  }

  CoverageMapBase* ExecutableInfo::getCoverageMap ( uint32_t address )
  {
    CoverageMapBase*         aCoverageMap = NULL;
    coverageMaps_t::iterator it;
    std::string              itsSymbol;

    // Obtain the coverage map containing the specified address.
    itsSymbol = theSymbolTable->getSymbol( address );
    if (itsSymbol != "") {
      it = coverageMaps.find( itsSymbol );
      aCoverageMap = (*it).second;
    }

    return aCoverageMap;
  }

  std::string ExecutableInfo::getFileName ( void ) const
  {
    return executableName;
  }

  std::string ExecutableInfo::getLibraryName( void ) const
  {
    return libraryName;
  }

  uint32_t ExecutableInfo::getLoadAddress( void ) const
  {
    return loadAddress;
  }


  SymbolTable* ExecutableInfo::getSymbolTable ( void ) const
  {
    return theSymbolTable;
  }

  CoverageMapBase* ExecutableInfo::createCoverageMap (
    const std::string& symbolName,
    uint32_t           lowAddress,
    uint32_t           highAddress
  )
  {
    CoverageMapBase                          *theMap;
    ExecutableInfo::coverageMaps_t::iterator  itr;

    itr = coverageMaps.find( symbolName );
    if ( itr == coverageMaps.end() ) {
      theMap = new CoverageMap( lowAddress, highAddress );
      coverageMaps[ symbolName ] = theMap;
    } else {
      theMap = itr->second;
      theMap->Add( lowAddress, highAddress );
    }
    return theMap;
  }

  bool ExecutableInfo::hasDynamicLibrary( void )
  {
     return (libraryName != "");
  }

  void ExecutableInfo::mergeCoverage( void ) {
    ExecutableInfo::coverageMaps_t::iterator  itr;

    for (itr = coverageMaps.begin(); itr != coverageMaps.end(); itr++) {
      SymbolsToAnalyze->mergeCoverageMap( (*itr).first, (*itr).second );
    }
  }

  void ExecutableInfo::setLoadAddress( uint32_t address )
  {
    loadAddress = address;
  }

}
