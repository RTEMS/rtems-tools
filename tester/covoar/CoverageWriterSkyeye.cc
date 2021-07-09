/*! @file CoverageWriterSkyeye.cc
 *  @brief CoverageWriterSkyeye Implementation
 *
 *  This file contains the implementation of the CoverageWriter class
 *  for the coverage files written by the multi-architecture simulator
 *  Skyeye (http://www.skyeye.org).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <rld.h>

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
    const std::string&  file,
    CoverageMapBase*    coverage,
    uint32_t            lowAddress,
    uint32_t            highAddress
  )
  {
    uint32_t                a;
    uint8_t                 cover;
    std::ofstream           coverageFile;
    prof_header_t           header;
    std::ofstream::pos_type bytes_before;

    /*
     *  read the file and update the coverage map passed in
     */
    coverageFile.open( file );
    if ( !coverageFile.is_open() ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageWriterSkyeye::writeFile" );
    }

    /* clear out the header and fill it in */
    memset( &header, 0, sizeof( header ) );
    header.ver           = 0x1;
    header.header_length = sizeof( header );
    header.prof_start    = lowAddress;
    header.prof_end      = highAddress;
    strcpy( header.desc, "Skyeye Coverage Data" );

    bytes_before = coverageFile.tellp();

    coverageFile.write( (char *) &header, sizeof( header ) );
    if ( coverageFile.tellp() - bytes_before != sizeof( header ) ) {
      std::ostringstream what;
      what << "Unable to write header to " << file;
      throw rld::error( what, "CoverageWriterSkyeye::writeFile" );
    }

    for ( a = lowAddress; a < highAddress; a += 8 ) {
      cover  = ((coverage->wasExecuted( a ))     ? 0x01 : 0);
      cover |= ((coverage->wasExecuted( a + 4 )) ? 0x10 : 0);

      bytes_before = coverageFile.tellp();

      coverageFile.write( (char *) &cover, sizeof( cover ) );
      if ( coverageFile.tellp() - bytes_before != sizeof( cover ) ) {
        std::ostringstream what;
        what << "write to " << file
             << " at address 0x"
             << std::hex << std::setfill( '0' )
             << std::setw( 8 ) << a
             << std::setfill( ' ' ) << std::dec
             << "failed";
        throw rld::error( what, "CoverageWriterSkyeye::writeFile" );
      }
    }

  }
}
