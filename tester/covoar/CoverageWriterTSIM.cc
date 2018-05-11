/*! @file CoverageWriterTSIM.cc
 *  @brief CoverageWriterTSIM Implementation
 *
 *  This file contains the implementation of the CoverageWriter class
 *  for the coverage files written by the SPARC simulator TSIM.
 */

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>

#include <rld.h>

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
    coverageFile = ::fopen( file, "w" );
    if ( !coverageFile ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageWriterTSIM::writeFile" );
    }

    for ( a = lowAddress; a < highAddress; a += 0x80 ) {
      status = fprintf( coverageFile, "%x : ", a );
      if ( status == EOF || status == 0 ) {
        break;
      }
      for ( i = 0; i < 0x80; i += 4 ) {
        cover = ((coverage->wasExecuted( a + i )) ? 1 : 0);
        status = ::fprintf( coverageFile, "%d ", cover );
	if ( status == EOF || status == 0 ) {
          ::fclose( coverageFile );
          std::ostringstream what;
          what << "write to " << file
               << " at address 0x"
               << std::hex << std::setfill('0')
               << std::setw(8) << a
               << std::setfill(' ') << std::dec
               << "failed";
          throw rld::error( what, "CoverageWriterTSIM::writeFile" );
	}
      }
      ::fprintf( coverageFile, "\n" );
    }

    ::fclose( coverageFile );
  }
}
