/*! @file CoverageReaderQEMU.cc
 *  @brief CoverageReaderQEMU Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the QEMU coverage data files.
 */

#include "covoar-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <rld.h>

#include "CoverageReaderQEMU.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"

#include "qemu-traces.h"

namespace Coverage {

  CoverageReaderQEMU::CoverageReaderQEMU()
  {
    branchInfoAvailable_m = true;
  }

  CoverageReaderQEMU::~CoverageReaderQEMU()
  {
  }

  void CoverageReaderQEMU::processFile(
    const std::string&    file,
    ExecutableInfo* const executableInformation
  )
  {
    struct trace_header header;
    uintptr_t           i;
    std::ifstream       traceFile;
    uint8_t             taken;
    uint8_t             notTaken;
    uint8_t             branchInfo;

    taken      = targetInfo_m->qemuTakenBit();
    notTaken   = targetInfo_m->qemuNotTakenBit();
    branchInfo = taken | notTaken;

    //
    // Open the coverage file and read the header.
    //
    traceFile.open( file );
    if ( !traceFile.is_open() ) {
      std::ostringstream what;
      what << "Unable to open " << file;
      throw rld::error( what, "CoverageReaderQEMU::processFile" );
    }

    traceFile.read( (char *) &header, sizeof( trace_header ) );
    if ( traceFile.fail() || traceFile.gcount() != sizeof( trace_header ) ) {
      std::ostringstream what;
      what << "Unable to read header from " << file;
      throw rld::error( what, "CoverageReaderQEMU::processFile" );
    }

    //
    // Read ENTRIES number of trace entries.
    //
#define ENTRIES 1024
    while ( true ) {
      CoverageMapBase*    aCoverageMap = NULL;
      struct trace_entry  entries[ENTRIES];
      struct trace_entry* entry;

      // Read and process each line of the coverage file.
      traceFile.read( (char *) entries, sizeof( struct trace_entry ) );
      if ( traceFile.gcount() == 0 ) {
        break;
      }

      // Get the coverage map for each entry.  Note that the map is
      // the same for each entry in the coverage map
      for ( int count = 0; count < traceFile.gcount(); count++ ) {

        entry = &entries[count];

        // Mark block as fully executed.
        // Obtain the coverage map containing the specified address.
        aCoverageMap = executableInformation->getCoverageMap( entry->pc );

        // Ensure that coverage map exists.
        if ( !aCoverageMap )
          continue;

        // Set was executed for each TRACE_OP_BLOCK
        if ( entry->op & TRACE_OP_BLOCK ) {
         for ( i = 0; i < entry->size; i++ ) {
            aCoverageMap->setWasExecuted( entry->pc + i );
          }
        }

        // Determine if additional branch information is available.
        if ( ( entry->op & branchInfo ) != 0 ) {
          uint32_t  a = entry->pc + entry->size - 1;
            while ( a > entry->pc && !aCoverageMap->isStartOfInstruction( a ) )
              a--;
            if ( a == entry->pc && !aCoverageMap->isStartOfInstruction( a ) ) {
              // Something went wrong parsing the objdump.
              std::ostringstream what;
              what << "Reached beginning of range in " << file
                << " at " << entry->pc << " with no start of instruction.";
              throw rld::error( what, "CoverageReaderQEMU::processFile" );
            }
            if ( entry->op & taken ) {
              aCoverageMap->setWasTaken( a );
            } else if ( entry->op & notTaken ) {
              aCoverageMap->setWasNotTaken( a );
            }
        }
      }
    }
  }
}
