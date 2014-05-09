/*! @file CoverageWriterRTEMS.cc
 *  @brief CoverageWriterRTEMS Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  the unified overage file format.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoverageWriterRTEMS.h"
#include "rtemscov_header.h"

namespace Coverage {
  
  CoverageWriterRTEMS::CoverageWriterRTEMS()
  {
  }

  CoverageWriterRTEMS::~CoverageWriterRTEMS()
  {
  }

  void CoverageWriterRTEMS::writeFile(
    const char* const file,
    CoverageMapBase*  coverage,
    uint32_t          lowAddress,
    uint32_t          highAddress
  )
  {
    FILE*                       coverageFile;
    uint32_t                    a;
    int                         status;
    uint8_t                     cover;
    rtems_coverage_map_header_t header;

    /*
     *  read the file and update the coverage map passed in
     */
    coverageFile = fopen( file, "w" );
    if ( !coverageFile ) {
      fprintf(
        stderr,
        "ERROR: CoverageWriterRTEMS::writeFile - unable to open %s\n",
        file
      );
      exit(-1);
    }

    /* clear out the header and fill it in */
    memset( &header, 0, sizeof(header) );
    header.ver           = 0x1;
    header.header_length = sizeof(header);
    header.start         = lowAddress;
    header.end           = highAddress;
    strcpy( header.desc, "RTEMS Coverage Data" );

    status = fwrite(&header, 1, sizeof(header), coverageFile);
    if (status != sizeof(header)) {
      fprintf(
        stderr,
        "ERROR: CoverageWriterRTEMS::writeFile - unable to write header "
        "to %s\n",
        file
      );
      exit(-1);
    }

    for ( a=lowAddress ; a < highAddress ; a++ ) {
      cover  = ((coverage->wasExecuted( a ))     ? 0x01 : 0);
      status = fwrite(&cover, 1, sizeof(cover), coverageFile);
      if (status != sizeof(cover)) {
        fprintf(
          stderr,
          "ERROR: CoverageWriterRTEMS::writeFile - write to %s "
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
