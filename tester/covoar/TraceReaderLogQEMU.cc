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

#include <iostream>
#include <fstream>

#include "qemu-log.h"
#include "TraceReaderBase.h"
#include "TraceReaderLogQEMU.h"
#include "TraceList.h"
#include "qemu-traces.h"

#include "rld-process.h"

bool ReadUntilFound( std::ifstream& file, const char* line )
{
  char   discardBuff[100] = {};
  size_t len = strlen( line );

  if ( len > sizeof( discardBuff ) ) {
    std::cerr << "ReadUntilFound(): parameter 'line' is too long" << std::endl;
    return false;
  }

  do {
    file.read( discardBuff, sizeof( discardBuff ) - 2 );
    if ( file.fail() ) {
      return false;
    }

    if ( strncmp( discardBuff, line, len ) == 0 ) {
      return true;
    }

  } while( true );
}

namespace Trace {

  TraceReaderLogQEMU::TraceReaderLogQEMU()
  {
  }

  TraceReaderLogQEMU::~TraceReaderLogQEMU()
  {
  }

  bool TraceReaderLogQEMU::processFile(
    const std::string&          file,
    Coverage::ObjdumpProcessor& objdumpProcessor
  )
  {
    bool                done          = false;
    QEMU_LOG_IN_Block_t first         = { 0, "", "" };
    QEMU_LOG_IN_Block_t last          = { 0, "", "" };
    QEMU_LOG_IN_Block_t nextExecuted  = { 0, "", "" };
    uint32_t            nextlogical;
    int                 status;
    std::ifstream       logFile;
    int                 result;
    int                 fileSize;
    char                ignore;

    //
    // Verify that the log file has a non-zero size.
    //
    logFile.open( file, std::ifstream::in | std::ifstream::binary );
    if ( !logFile.is_open() ) {
      std::cerr << "Unable to open " << file << std::endl;
      return false;
    }

    logFile.seekg( 0, std::ios::end );
    fileSize = logFile.tellg();

    if ( fileSize == 0 ) {
      std::cerr << file << " is 0 bytes long" << std::endl;
      return false;
    }

    logFile.close();

    //
    // Open the coverage file and discard the header.
    //
    logFile.open( file );

    //
    //  Discard Header section
    //
    if (! ReadUntilFound( logFile, QEMU_LOG_SECTION_END ) ) {
      std::cerr << "Unable to locate end of log file header" << std::endl;
      return false;
    }

    //
    //  Find first IN block
    //
    if (! ReadUntilFound( logFile, QEMU_LOG_IN_KEY )){
      std::cerr << "Error: Unable to locate first IN: Block in Log file"
                << std::endl;
      return false;
    }

    //
    //  Read First Start Address
    //
    logFile >> std::hex >> first.address >> std::dec
            >> ignore
            >> first.instruction
            >> first.data;

    if ( logFile.fail() ) {
      std::cerr << "Error Unable to Read Initial First Block"
                << std::endl;
      done = true;
    }

    while (!done) {

      last = first;

      // Read until we get to the last instruction in the block.
      do {
        logFile >> std::hex >> last.address >> std::dec
                >> ignore
                >> last.instruction
                >> last.data;
      } while( !logFile.fail() );

      nextlogical = objdumpProcessor.getAddressAfter(last.address);

      if (! ReadUntilFound( logFile, QEMU_LOG_IN_KEY )) {
        done = true;
        nextExecuted = last;
      } else {
        logFile >> std::hex >> nextExecuted.address >> std::dec
                >> ignore
                >> nextExecuted.instruction
                >> nextExecuted.data;

        if ( logFile.fail() ) {
          std::cerr << "Error Unable to Read First Block" << std::endl;
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

    return true;
  }
}
