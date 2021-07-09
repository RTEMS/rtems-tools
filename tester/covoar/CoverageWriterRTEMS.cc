/*! @file CoverageWriterRTEMS.cc
 *  @brief CoverageWriterRTEMS Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  the unified overage file format.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <rld.h>

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
    const std::string& file,
    CoverageMapBase*   coverage,
    uint32_t           lowAddress,
    uint32_t           highAddress
  )
  {
    std::ofstream               coverageFile;
    uint32_t                    a;
    uint8_t                     cover;
    rtems_coverage_map_header_t header;
    std::ofstream::pos_type     bytes_before;

    /*
     *  read the file and update the coverage map passed in
     */
    coverageFile.open( file );
    if ( !coverageFile.is_open() ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageWriterRTEMS::writeFile" );
    }

    /* clear out the header and fill it in */
    memset( &header, 0, sizeof( header ) );
    header.ver           = 0x1;
    header.header_length = sizeof( header );
    header.start         = lowAddress;
    header.end           = highAddress;
    strcpy( header.desc, "RTEMS Coverage Data" );

    bytes_before = coverageFile.tellp();

    coverageFile.write( (char *) &header, sizeof( header ) );
    if ( coverageFile.tellp() - bytes_before != sizeof( header ) ) {
      std::ostringstream what;
      what << "Unable to write header to " << file;
      throw rld::error( what, "CoverageWriterRTEMS::writeFile" );
    }

    for ( a = lowAddress; a < highAddress; a++ ) {
      cover  = ( ( coverage->wasExecuted( a ) ) ? 0x01 : 0 );

      bytes_before = coverageFile.tellp();

      coverageFile.write( (char *) &cover, sizeof( cover ) );
      if ( coverageFile.tellp() - bytes_before != sizeof( cover ) ) {
        std::cerr << "CoverageWriterRTEMS::writeFile - write to "
                  << file
                  << " at address 0x%"
                  << std::hex << std::setfill( '0' )
                  << std::setw( 8 ) << a
                  << std::setfill( ' ' ) << std::dec
                  << " failed\n";
      }
    }

  }
}
