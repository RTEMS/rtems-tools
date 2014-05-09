/*! @file CoverageReaderSkyeye.cc
 *  @brief CoverageReaderSkyeye Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the Skyeye coverage data files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "CoverageReaderSkyeye.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"
#include "skyeye_header.h"

namespace Coverage {

  CoverageReaderSkyeye::CoverageReaderSkyeye()
  {
  }

  CoverageReaderSkyeye::~CoverageReaderSkyeye()
  {
  }

  void CoverageReaderSkyeye::processFile(
    const char* const     file,
    ExecutableInfo* const executableInformation
  )
  {
    CoverageMapBase* aCoverageMap = NULL;
    uintptr_t        baseAddress;
    uint8_t          cover;
    FILE*            coverageFile;
    prof_header_t    header;
    uintptr_t        i;
    uintptr_t        length;
    int              status;

    //
    // Open the coverage file and read the header.
    //
    coverageFile = fopen( file, "r" );
    if (!coverageFile) {
      fprintf(
        stderr,
        "ERROR: CoverageReaderSkyeye::processFile - Unable to open %s\n",
        file
      );
      exit( -1 );
    }

    status = fread( &header, sizeof(header), 1, coverageFile );
    if (status != 1) {
      fprintf(
        stderr,
        "ERROR: CoverageReaderSkyeye::processFile - "
        "Unable to read header from %s\n",
        file
      );
      exit( -1 );
    }

    baseAddress = header.prof_start;
    length      = header.prof_end - header.prof_start;
    
    #if 0
    fprintf( 
      stderr,
      "%s: 0x%08x 0x%08x 0x%08lx/%ld\n", 
      file,
      header.prof_start,
      header.prof_end,
      (unsigned long) length,
      (unsigned long) length
    );
    #endif

    //
    // Read and process each line of the coverage file.
    //
    for (i=0; i<length; i+=8) {
      status = fread( &cover, sizeof(uint8_t), 1, coverageFile );
      if (status != 1) {
        fprintf(
	  stderr,
	  "CoverageReaderSkyeye::ProcessFile - breaking after 0x%08x in %s\n",
	  (unsigned int) i,
          file
        );
        break;
      }

      //
      // Obtain the coverage map containing the address and
      // mark the address as executed.
      //
      // NOTE: This method ONLY works for Skyeye in 32-bit mode.
      //
      if (cover & 0x01) {
        aCoverageMap = executableInformation->getCoverageMap( baseAddress + i );
        if (aCoverageMap) {
          aCoverageMap->setWasExecuted( baseAddress + i );
          aCoverageMap->setWasExecuted( baseAddress + i + 1 );
          aCoverageMap->setWasExecuted( baseAddress + i + 2 );
          aCoverageMap->setWasExecuted( baseAddress + i + 3 );
        }
      }

      if (cover & 0x10) {
        aCoverageMap = executableInformation->getCoverageMap(
          baseAddress + i + 4
        );
        if (aCoverageMap) {
          aCoverageMap->setWasExecuted( baseAddress + i + 4 );
          aCoverageMap->setWasExecuted( baseAddress + i + 5 );
          aCoverageMap->setWasExecuted( baseAddress + i + 6 );
          aCoverageMap->setWasExecuted( baseAddress + i + 7 );
        }
      }
    }

    fclose( coverageFile );
  }
}
