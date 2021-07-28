/*
 * RTEMS Tools Project (http://www.rtems.org/)
 * Copyright 2014 OAR Corporation
 * All rights reserved.
 *
 * This file is part of the RTEMS Tools package in 'rtems-tools'.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*! @file TraceReaderLogQEMU.cc
 *  @brief TraceReaderLogQEMU Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the QEMU coverage data files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "qemu-log.h"
#include "app_common.h"
#include "TraceReaderBase.h"
#include "TraceReaderLogQEMU.h"
#include "TraceList.h"
#include "qemu-traces.h"

#include "rld-process.h"

#if HAVE_STAT64
#define STAT stat64
#else
#define STAT stat
#endif

#if HAVE_OPEN64
#define OPEN fopen64
#else
#define OPEN fopen
#endif

#define MAX_LINE_LENGTH 512

bool ReadUntilFound( FILE *file, const char *line )
{
  char discardBuff[100];
  size_t  len = strlen( line );

  do {
    if ( !fgets( discardBuff, 99, file ) )
      return false;

    if ( strncmp( discardBuff, line, len ) == 0 )
      return true;
  } while (1);
}

namespace Trace {

  TraceReaderLogQEMU::TraceReaderLogQEMU()
  {
  }

  TraceReaderLogQEMU::~TraceReaderLogQEMU()
  {
  }

  bool TraceReaderLogQEMU::processFile(
    const char* const           file,
    Coverage::ObjdumpProcessor& objdumpProcessor
  )
  {
    bool                done          = false;
    QEMU_LOG_IN_Block_t first         = { 0, "", "" };
    QEMU_LOG_IN_Block_t last          = { 0, "", "" };
    QEMU_LOG_IN_Block_t nextExecuted  = { 0, "", "" };
    uint32_t            nextlogical;
    struct STAT         statbuf;
    int                 status;
    FILE*               logFile;
    int                 result;
    char                inputBuffer[MAX_LINE_LENGTH];

    //
    // Verify that the log file has a non-zero size.
    //
    // NOTE: We prefer stat64 because some of the coverage files are HUGE!
    status = STAT( file, &statbuf );
    if (status == -1) {
      fprintf( stderr, "Unable to stat %s\n", file );
      return false;
    }

    if (statbuf.st_size == 0) {
      fprintf( stderr, "%s is 0 bytes long\n", file );
      return false;
    }

    //
    // Open the coverage file and discard the header.
    //
    logFile = OPEN( file, "r" );
    if (!logFile) {
      fprintf( stderr, "Unable to open %s\n", file );
      return false;
    }

    //
    //  Discard Header section
    //
    if (! ReadUntilFound( logFile, QEMU_LOG_SECTION_END ) ) {
      fprintf( stderr, "Unable to locate end of log file header\n" );
      fclose( logFile );
      return false;
    }

    //
    //  Find first IN block
    //
    if (! ReadUntilFound( logFile, QEMU_LOG_IN_KEY )){
      fprintf(stderr,"Error: Unable to locate first IN: Block in Log file \n");
      fclose( logFile );
      return false;
    }

    //
    //  Read First Start Address
    //
    fgets(inputBuffer, MAX_LINE_LENGTH, logFile );
    result = sscanf(
      inputBuffer,
      "0x%08lx: %s %s\n",
      &first.address,
      first.instruction,
      first.data
    );
    if ( result < 2 )
    {
      fprintf(stderr, "Error Unable to Read Initial First Block\n" );
      done = true;
    }

    while (!done) {

      last = first;

      // Read until we get to the last instruction in the block.
      do {
        fgets(inputBuffer, MAX_LINE_LENGTH, logFile );
        result = sscanf(
          inputBuffer,
          "0x%08lx: %s %s\n",
          &last.address,
          last.instruction,
          last.data
        );
      } while( result > 1);

      nextlogical = objdumpProcessor.getAddressAfter(last.address);

      if (! ReadUntilFound( logFile, QEMU_LOG_IN_KEY )) {
        done = true;
        nextExecuted = last;
      } else {
        fgets(inputBuffer, MAX_LINE_LENGTH, logFile );
        result = sscanf(
          inputBuffer,
          "0x%08lx: %s %s\n",
          &nextExecuted.address,
          nextExecuted.instruction,
          nextExecuted.data
        );
        if ( result < 2 )
        {
          fprintf(stderr, "Error Unable to Read First Block\n" );
        }
      }

      // If the nextlogical was not found we are throwing away
      // the block; otherwise add the block to the trace list.
      if (nextlogical != 0) {
        TraceList::exitReason_t reason = TraceList::EXIT_REASON_OTHER;

        if ( objdumpProcessor.IsBranch( last.instruction ) ) {
          if ( nextExecuted.address == nextlogical ) {
            reason = TraceList::EXIT_REASON_BRANCH_NOT_TAKEN;
          }  else {
            reason = TraceList::EXIT_REASON_BRANCH_TAKEN;
          }
        }
        Trace.add( first.address, nextlogical, reason );
      }
      first = nextExecuted;
    }
    fclose( logFile );
    return true;
  }
}
