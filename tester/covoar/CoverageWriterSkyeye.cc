/*! @file CoverageWriterSkyeye.cc
 *  @brief CoverageWriterSkyeye Implementation
 *
 *  This file contains the implementation of the CoverageWriter class
 *  for the coverage files written by the multi-architecture simulator
 *  Skyeye (http://www.skyeye.org).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoverageWriterSkyeye.h"
#include "skyeye_header.h"

namespace Coverage {
  
  CoverageWriterSkyeye::CoverageWriterSkyeye()
  {
  }

  CoverageWriterSkyeye::~CoverageWriterSkyeye()
  {
  }

  void CoverageWriterSkyeye::writeFile(
    const char* const file,
    CoverageMapBase*  coverage,
    uint32_t          lowAddress,
    uint32_t          highAddress
  )
  {
    uint32_t      a;
    uint8_t       cover;
    FILE*         coverageFile;
    prof_header_t header;
    int           status;

    /*
     *  read the file and update the coverage map passed in
     */
    coverageFile = fopen( file, "w" );
    if ( !coverageFile ) {
      fprintf(
        stderr,
        "ERROR: CoverageWriterSkyeye::writeFile - unable to open %s\n",
        file
      );
      exit(-1);
    }

    /* clear out the header and fill it in */
    memset( &header, 0, sizeof(header) );
    header.ver           = 0x1;
    header.header_length = sizeof(header);
    header.prof_start    = lowAddress;
    header.prof_end      = highAddress;
    strcpy( header.desc, "Skyeye Coverage Data" );

    status = fwrite(&header, 1, sizeof(header), coverageFile);
    if (status != sizeof(header)) {
      fprintf(
        stderr,
        "ERROR: CoverageWriterSkyeye::writeFile - unable to write header "
        "to %s\n",
        file
      );
      exit(-1);
    }

    for ( a=lowAddress ; a < highAddress ; a+= 8 ) {
      cover  = ((coverage->wasExecuted( a ))     ? 0x01 : 0);
      cover |= ((coverage->wasExecuted( a + 4 )) ? 0x10 : 0);
      status = fwrite(&cover, 1, sizeof(cover), coverageFile);
      if (status != sizeof(cover)) {
	fprintf(
          stderr,
          "ERROR: CoverageWriterSkyeye::writeFile - write to %s "
          "at address 0x%08x failed\n",
          file,
          a
        );
	exit( -1 );
      }
      // fprintf( stderr, "0x%x %d\n", a, cover );
    }

    fclose( coverageFile );
  }
}
