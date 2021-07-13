/*! @file CoverageReaderRTEMS.cc
 *  @brief CoverageReaderRTEMS Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the RTEMS coverage data files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <rld.h>

#include "CoverageReaderRTEMS.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"
#include "rtemscov_header.h"

namespace Coverage {

  CoverageReaderRTEMS::CoverageReaderRTEMS()
  {
  }

  CoverageReaderRTEMS::~CoverageReaderRTEMS()
  {
  }

  void CoverageReaderRTEMS::processFile(
    const std::string&    file,
    ExecutableInfo* const executableInformation
  )
  {
    CoverageMapBase*            aCoverageMap = NULL;
    uintptr_t                   baseAddress;
    uint8_t                     cover;
    std::ifstream               coverageFile;
    rtems_coverage_map_header_t header;
    uintptr_t                   i;
    uintptr_t                   length;

    //
    // Open the coverage file and read the header.
    //
    coverageFile.open( file );
    if ( !coverageFile.is_open() ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageReaderRTEMS::processFile" );
    }

    coverageFile.read( (char *) &header, sizeof( header ) );
    if ( coverageFile.fail() ) {
      std::ostringstream what;
      what << "Unable to read header from " << file;
      throw rld::error( what, "CoverageReaderRTEMS::processFile" );
    }

    baseAddress = header.start;
    length      = header.end - header.start;

    //
    // Read and process each line of the coverage file.
    //
    for ( i = 0; i < length; i++ ) {
      coverageFile.read( (char *) &cover, sizeof( uint8_t ) );
      if ( coverageFile.fail() ) {
        std::cerr << "breaking after 0x"
                  << std::hex << std::setfill( '0' )
                  << std::setw( 8 ) << i
                  << std::setfill( ' ' ) << std::dec
                  << " in " << file
                  << std::endl;
        break;
      }

      //
      // Obtain the coverage map containing the address and
      // mark the address as executed.
      //
      if ( cover ) {
        aCoverageMap = executableInformation->getCoverageMap( baseAddress + i );
        if ( aCoverageMap )
          aCoverageMap->setWasExecuted( baseAddress + i );
      }
    }

  }
}
