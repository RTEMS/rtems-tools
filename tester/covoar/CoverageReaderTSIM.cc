/*! @file CoverageReaderTSIM.cc
 *  @brief CoverageReaderTSIM Implementation
 *
 *  This file contains the implementation of the CoverageReader class
 *  for the coverage files written by the SPARC simulator TSIM.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <rld.h>

#include "CoverageReaderTSIM.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"

namespace Coverage {

  CoverageReaderTSIM::CoverageReaderTSIM()
  {

  }

  CoverageReaderTSIM::~CoverageReaderTSIM()
  {
  }

  void CoverageReaderTSIM::processFile(
    const std::string&    file,
    ExecutableInfo* const executableInformation
  )
  {
    CoverageMapBase* aCoverageMap = NULL;
    int              baseAddress;
    int              cover;
    std::ifstream    coverageFile;
    int              i;
    char             ignore;

    //
    // Open the coverage file.
    //
    coverageFile.open( file );
    if ( !coverageFile.is_open() ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageReaderTSIM::processFile" );
    }

    //
    // Read and process each line of the coverage file.
    //
    while ( true ) {
      coverageFile >> std::hex >> baseAddress >> ignore >> std::dec;
      if ( coverageFile.gcount() == 0 || coverageFile.fail() ) {
        break;
      }

      for (i = 0; i < 0x80; i += 4) {
        unsigned int a;

        coverageFile >> std::hex >> cover >> std::dec;
        if ( coverageFile.fail() || coverageFile.gcount() == 0 ) {
          std::cerr << "CoverageReaderTSIM: WARNING! Short line in "
                    << file
                    << " at address 0x"
                    << std::hex << std::setfill('0')
                    << baseAddress
                    << std::setfill(' ') << std::dec
                    << std::endl;
          break;
        }

        //
        // Obtain the coverage map containing the address and
        // mark the address as executed.
        //
        a = baseAddress + i;
        aCoverageMap = executableInformation->getCoverageMap( a );
        if ( !aCoverageMap )
          continue;
        if ( cover & 0x01 ) {
          aCoverageMap->setWasExecuted( a );
          aCoverageMap->setWasExecuted( a + 1 );
          aCoverageMap->setWasExecuted( a + 2 );
          aCoverageMap->setWasExecuted( a + 3 );
          if ( cover & 0x08 ) {
            aCoverageMap->setWasTaken( a );
            branchInfoAvailable_m = true;
          }
          if ( cover & 0x10 ) {
            aCoverageMap->setWasNotTaken( a );
            branchInfoAvailable_m = true;
          }
        }
      }
    }

  }
}
