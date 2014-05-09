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
#include <getopt.h>

#include "qemu-log.h"

#include "TraceReaderLogQEMU.h"
#include "TraceWriterQEMU.h"
#include "TraceList.h"
#include "ObjdumpProcessor.h"
#include "app_common.h"
#include "TargetFactory.h"

char*       progname;

void usage()
{
  fprintf(
    stderr,
    "Usage: %s [-v] -c CPU -e executable -t tracefile [-E logfile]\n",
    progname
  );
  exit(1);
}

int main(
  int    argc,
  char** argv
)
{
  int                          opt;
  Trace::TraceReaderLogQEMU    log;
  Trace::TraceWriterQEMU       trace;
  const char                  *cpuname    = "";
  const char                  *executable = "";
  const char                  *tracefile  =  "";
  const char                  *logname = "/tmp/qemu.log";
  Coverage::ExecutableInfo*    executableInfo;
   
  //
  // Process command line options.
  //
  progname = argv[0];

  while ((opt = getopt(argc, argv, "c:e:l:L:t:v")) != -1) {
    switch (opt) {
      case 'c': cpuname = optarg;        break;
      case 'e': executable = optarg;     break;
      case 'l': logname = optarg;        break;
      case 'L': dynamicLibrary = optarg; break;
      case 't': tracefile = optarg;      break;
      case 'v': Verbose = true;          break;
      default:  usage();
    }
  }

  // Make sure we have all the required parameters
  if ( !cpuname ) {
    fprintf( stderr, "cpuname not specified\n" );
    usage();
  }

  if ( !executable ) {
    fprintf( stderr, "executable not specified\n" );
    usage();
  }

  if ( !tracefile ) {
    fprintf( stderr, "output trace file not specified\n" );
    usage();
  }

  // Create toolnames.
  TargetInfo = Target::TargetFactory( cpuname );

  if (dynamicLibrary)
    executableInfo = new Coverage::ExecutableInfo( executable, dynamicLibrary );
  else
    executableInfo = new Coverage::ExecutableInfo( executable );

  objdumpProcessor = new Coverage::ObjdumpProcessor();
 
  // If a dynamic library was specified, determine the load address.
  if (dynamicLibrary)
    executableInfo->setLoadAddress(
      objdumpProcessor->determineLoadAddress( executableInfo )
    );

  objdumpProcessor->loadAddressTable( executableInfo );

  log.processFile( logname );

  trace.writeFile( tracefile, &log );

}
