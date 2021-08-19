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
/*! @file TraceWriterQEMU.cc
 *  @brief TraceWriterQEMU Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading the QEMU coverage data files.
 */

#include <cstring>
#include <cstdio>

#include <iostream>
#include <iomanip>

#include <rld-process.h>

#include "TraceWriterQEMU.h"
#include "ExecutableInfo.h"
#include "CoverageMap.h"
#include "qemu-traces.h"

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

namespace Trace {

  TraceWriterQEMU::TraceWriterQEMU():
    TraceWriterBase()
  {
  }

  TraceWriterQEMU::~TraceWriterQEMU()
  {
  }

  bool TraceWriterQEMU::writeFile(
    const char* const          file,
    Trace::TraceReaderBase    *log,
    bool                       verbose
  )
  {
    struct trace_header header;
    int                 status;
    FILE*               traceFile;
    uint8_t             taken;
    uint8_t             notTaken;

    taken    = targetInfo_m->qemuTakenBit();
    notTaken = targetInfo_m->qemuNotTakenBit();

    //
    // Verify that the TraceList has a non-zero size.
    //
    if ( log->Trace.set.begin() == log->Trace.set.end() ){
      fprintf( stderr, "ERROR: Empty TraceList\n" );
      return false;
    }

    //
    // Open the trace file.
    //
    traceFile = ::OPEN( file, "w" );
    if (!traceFile) {
      std::ostringstream what;
      std::cerr << "Unable to open " << file << std::endl;
      return false;
    }

    //
    //  Write the Header to the file
    //
    // The header.magic field is actually 12 bytes, but QEMU_TRACE_MAGIC is
    // 13 bytes including the NULL.
    memcpy( header.magic, QEMU_TRACE_MAGIC, sizeof(header.magic) );
    header.version = QEMU_TRACE_VERSION;
    header.kind    = QEMU_TRACE_KIND_RAW;  // XXX ??
    header.sizeof_target_pc = 32;
    header.big_endian = false;
    header.machine[0] = 0; // XXX ??
    header.machine[1] = 0; // XXX ??
    header._pad = 0;
    status = ::fwrite( &header, sizeof(trace_header), 1, traceFile );
    if (status != 1) {
      std::cerr << "Unable to write header to " << file << std::endl;
      ::fclose( traceFile );
      return false;
    }

    if (verbose)
      std::cerr << "magic = " << QEMU_TRACE_MAGIC << std::endl
                << "version = " << header.version << std::endl
                << "kind = " << header.kind << std::endl
                << "sizeof_target_pc = " << header.sizeof_target_pc << std::endl
                << "big_endian = " << header.big_endian << std::endl
                << std::hex << std::setfill('0')
                << "machine = " << std::setw(2) << header.machine[0]
                << ':' << header.machine[1]
                << std::dec << std::setfill(' ')
                << std::endl;

    //
    // Loop through log and write each entry.
    //

    for (const auto & itr : log->Trace.set) {
      struct trace_entry32  entry;

      entry._pad[0] = 0;
      entry.pc      = itr.lowAddress;
      entry.size    = itr.length;
      entry.op      = TRACE_OP_BLOCK;
      switch (itr.exitReason) {
        case TraceList::EXIT_REASON_BRANCH_TAKEN:
          entry.op |= taken;
          break;
        case TraceList::EXIT_REASON_BRANCH_NOT_TAKEN:
          entry.op |= notTaken;
          break;
        case TraceList::EXIT_REASON_OTHER:
          break;
        default:
          throw rld::error( "Unknown exit Reason", "TraceWriterQEMU::writeFile" );
          break;
       }

      if ( verbose )
        std::cerr << std::hex << std::setfill('0')
                  << entry.pc << ' ' << entry.size << ' ' << entry.op
                  << std::dec << std::setfill(' ')
                  << std::endl;

      status = ::fwrite( &entry, sizeof(entry), 1, traceFile );
      if (status != 1) {
        ::fclose( traceFile );
        std::cerr << "Unable to write entry to " << file << std::endl;
        return false;
      }
    }

    ::fclose( traceFile );
    return true;
  }
}
