/*! @file CoverageWriterTSIM.cc
 *  @brief CoverageWriterTSIM Implementation
 *
 *  This file contains the implementation of the CoverageWriter class
 *  for the coverage files written by the SPARC simulator TSIM.
 */

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
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
    const std::string& file,
    CoverageMapBase*   coverage,
    uint32_t           lowAddress,
    uint32_t           highAddress
  )
  {
    uint32_t      a;
    int           cover;
    std::ofstream coverageFile;
    int           i;

    /*
     *  read the file and update the coverage map passed in
     */
    coverageFile.open( file );
    if ( !coverageFile.is_open() ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageWriterTSIM::writeFile" );
    }

    for ( a = lowAddress; a < highAddress; a += 0x80 ) {
      coverageFile << std::hex << a << " : " << std::dec;
      if ( coverageFile.fail() ) {
        break;
      }

      for ( i = 0; i < 0x80; i += 4 ) {
        cover = ( ( coverage->wasExecuted( a + i ) ) ? 1 : 0 );
        coverageFile << cover << " ";

        if ( coverageFile.fail() ) {
          std::ostringstream what;
          what << "write to " << file
               << " at address 0x"
               << std::hex << std::setfill( '0' )
               << std::setw( 8 ) << a
               << std::setfill( ' ' ) << std::dec
               << "failed";
          throw rld::error( what, "CoverageWriterTSIM::writeFile" );
        }
      }
      coverageFile << std::endl;
    }
  }
}
