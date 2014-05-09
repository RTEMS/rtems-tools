/*! @file CoverageWriterTSIM.cc
 *  @brief CoverageWriterTSIM Implementation
 *
 *  This file contains the implementation of the CoverageWriter class
 *  for the coverage files written by the SPARC simulator TSIM.
 */

#include <stdio.h>
#include <stdlib.h>

#include "CoverageWriterTSIM.h"

namespace Coverage {
  
  CoverageWriterTSIM::CoverageWriterTSIM()
  {
  }

  CoverageWriterTSIM::~CoverageWriterTSIM()
  {
  }

  
  void CoverageWriterTSIM::writeFile(
    const char* const file,
    CoverageMapBase*  coverage,
    uint32_t          lowAddress,
    uint32_t          highAddress
  )
  {
    uint32_t a;
    int      cover;
    FILE*    coverageFile;
    int      i;
    int      status;

    /*
     *  read the file and update the coverage map passed in
     */
    coverageFile = fopen( file, "w" );
    if ( !coverageFile ) {
      fprintf(
        stderr,
        "ERROR: CoverageWriterTSIM::writeFile - unable to open %s\n",
        file
      );
      exit(-1);
    }

    for ( a=lowAddress ; a < highAddress ; a+= 0x80 ) {
      status = fprintf( coverageFile, "%x : ", a );
      if ( status == EOF || status == 0 ) {
        break;
      }
      // fprintf( stderr, "%08x : ", baseAddress );
      for ( i=0 ; i < 0x80 ; i+=4 ) {
        cover = ((coverage->wasExecuted( a + i )) ? 1 : 0);
        status = fprintf( coverageFile, "%d ", cover );
	if ( status == EOF || status == 0 ) {
          fprintf(
            stderr,
            "ERROR: CoverageWriterTSIM:writeFile - write to %s "
            "at address 0x%08x failed\n",
            file,
            a
          );
	  exit( -1 );
	}
        // fprintf( stderr, "%d ", cover );
      }
      fprintf( coverageFile, "\n" );
      // fprintf( stderr, "\n" );
  
    }

    fclose( coverageFile );
  }
}
