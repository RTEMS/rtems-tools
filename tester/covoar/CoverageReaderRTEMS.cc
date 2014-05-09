/*! @file CoverageReaderRTEMS.cc
 *  @brief CoverageReaderRTEMS Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the RTEMS coverage data files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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
    const char* const     file,
    ExecutableInfo* const executableInformation
  )
  {
    CoverageMapBase*             aCoverageMap = NULL;
    uintptr_t                    baseAddress;
    uint8_t                      cover;
    FILE*                        coverageFile;
    rtems_coverage_map_header_t  header;
    uintptr_t                    i;
    uintptr_t                    length;
    int                          status;

    //
    // Open the coverage file and read the header.
    //
    coverageFile = fopen( file, "r" );
    if (!coverageFile) {
      fprintf(
        stderr,
        "ERROR: CoverageReaderRTEMS::processFile - Unable to open %s\n",
        file
      );
      exit( -1 );
    }

    status = fread( &header, sizeof(header), 1, coverageFile );
    if (status != 1) {
      fprintf(
        stderr,
        "ERROR: CoverageReaderRTEMS::processFile - "
        "Unable to read header from %s\n",
        file
      );
      exit( -1 );
    }

    baseAddress = header.start;
    length      = header.end - header.start;
    
    #if 0
    fprintf( 
      stderr,
      "%s: 0x%08x 0x%08x 0x%08lx/%ld\n", 
      file,
      header.start,
      header.end,
      (unsigned long) length,
      (unsigned long) length
    );
    #endif

    //
    // Read and process each line of the coverage file.
    //
    for (i=0; i<length; i++) {
      status = fread( &cover, sizeof(uint8_t), 1, coverageFile );
      if (status != 1) {
        fprintf(
          stderr,
          "CoverageReaderRTEMS::ProcessFile - breaking after 0x%08x in %s\n",
          (unsigned int) i,
          file
        );
        break;
      }

      //
      // Obtain the coverage map containing the address and
      // mark the address as executed.
      //
      if (cover) {
        aCoverageMap = executableInformation->getCoverageMap( baseAddress + i );
        if (aCoverageMap)
          aCoverageMap->setWasExecuted( baseAddress + i );
      }
    }

    fclose( coverageFile );
  }
}
