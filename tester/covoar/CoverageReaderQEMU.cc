/*! @file CoverageReaderQEMU.cc
 *  @brief CoverageReaderQEMU Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the QEMU coverage data files.
 */

#include "covoar-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "app_common.h"
#include "CoverageReaderQEMU.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"

#include "qemu-traces.h"

#if HAVE_STAT64
#define OPEN fopen64
#else
#define OPEN fopen
#endif

namespace Coverage {

  CoverageReaderQEMU::CoverageReaderQEMU()
  {
    BranchInfoAvailable = true;
  }

  CoverageReaderQEMU::~CoverageReaderQEMU()
  {
  }

  void CoverageReaderQEMU::processFile(
    const char* const     file,
    ExecutableInfo* const executableInformation
  )
  {
    struct trace_header header;
    uintptr_t           i;
    int                 status;
    FILE*               traceFile;
    uint8_t             taken;
    uint8_t             notTaken;
    uint8_t             branchInfo;

    taken    = TargetInfo->qemuTakenBit();
    notTaken = TargetInfo->qemuNotTakenBit();
    branchInfo = taken | notTaken;

    //
    // Open the coverage file and read the header.
    //
    traceFile = OPEN( file, "r" );
    if (!traceFile) {
      fprintf(
        stderr,
        "ERROR: CoverageReaderQEMU::processFile - Unable to open %s\n",
        file
      );
      exit( -1 );
    }

    status = fread( &header, sizeof(trace_header), 1, traceFile );
    if (status != 1) {
      fprintf(
        stderr,
        "ERROR: CoverageReaderQEMU::processFile - "
        "Unable to read header from %s\n",
        file
      );
      exit( -1 );
    }

    #if 0
      fprintf(
        stderr,
        "magic = %s\n"
        "version = %d\n"
        "kind = %d\n"
        "sizeof_target_pc = %d\n"
        "big_endian = %d\n"
        "machine = %02x:%02x\n",
        header.magic,
        header.version,
        header.kind,
        header.sizeof_target_pc,
        header.big_endian,
        header.machine[0], header.machine[1]
       );
    #endif

    //
    // Read ENTRIES number of trace entries.
    //
#define ENTRIES 1024
    while (1) {
      CoverageMapBase     *aCoverageMap = NULL;
      struct trace_entry  entries[ENTRIES];
      struct trace_entry  *entry;
      int                 num_entries;


      // Read and process each line of the coverage file.
      num_entries = fread(
        entries,
        sizeof(struct trace_entry),
        ENTRIES,
        traceFile
      );
      if (num_entries == 0)
        break;

      // Get the coverage map for each entry.  Note that the map is
      // the same for each entry in the coverage map
      for (int count=0; count<num_entries; count++) {

        entry = &entries[count];

        // Mark block as fully executed.
        // Obtain the coverage map containing the specified address.
        aCoverageMap = executableInformation->getCoverageMap( entry->pc );

        // Ensure that coverage map exists.
        if (!aCoverageMap)
          continue;

        // Set was executed for each TRACE_OP_BLOCK
        if (entry->op & TRACE_OP_BLOCK) {
         for (i=0; i<entry->size; i++) {
            aCoverageMap->setWasExecuted( entry->pc + i );
          }
        }

        // Determine if additional branch information is available.
        if ( (entry->op & branchInfo) != 0 ) {
          uint32_t  offset_e, offset_a;
          uint32_t  a = entry->pc + entry->size - 1;
          if ((aCoverageMap->determineOffset( a, &offset_a ) != true)   ||
             (aCoverageMap->determineOffset( entry->pc, &offset_e ) != true))
          {
            fprintf(
              stderr,
              "*** Trace block is inconsistent with coverage map\n"
              "*** Trace block (0x%08x - 0x%08x) for %d bytes\n"
              "*** Coverage map XXX \n",
              entry->pc,
              a,
              entry->size
            );
          } else {
            while (!aCoverageMap->isStartOfInstruction(a))
              a--;
            if (entry->op & taken) {
              aCoverageMap->setWasTaken( a );
            } else if (entry->op & notTaken) {
              aCoverageMap->setWasNotTaken( a );
            }
          }
        }
      }
    }
    fclose( traceFile );
  }
}
