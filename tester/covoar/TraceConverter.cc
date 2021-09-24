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
#include <signal.h>
#include <unistd.h>

#include <rld.h>
#include <rld-process.h>

#include "qemu-log.h"
#include "TraceReaderLogQEMU.h"
#include "TraceWriterQEMU.h"
#include "TraceList.h"
#include "ObjdumpProcessor.h"
#include "TargetFactory.h"

#if defined(_WIN32) || defined(__CYGWIN__)
  #define kill(p,s) raise(s)
#endif

std::string progname;

void usage()
{
  std::cerr << "Usage: "
            << progname
            << " [-v] -c CPU -e executable -t tracefile [-E logfile]"
            << std::endl;
  exit(1);
}

static void
fatal_signal( int signum )
{
  signal( signum, SIG_DFL );

  rld::process::temporaries_clean_up();

  /*
   * Get the same signal again, this time not handled, so its normal effect
   * occurs.
   */
  kill( getpid(), signum );
}

static void
setup_signals( void )
{
  if ( signal (SIGINT, SIG_IGN) != SIG_IGN )
    signal( SIGINT, fatal_signal );
#ifdef SIGHUP
  if ( signal( SIGHUP, SIG_IGN ) != SIG_IGN )
    signal( SIGHUP, fatal_signal );
#endif
  if ( signal( SIGTERM, SIG_IGN ) != SIG_IGN )
    signal( SIGTERM, fatal_signal );
#ifdef SIGPIPE
  if ( signal( SIGPIPE, SIG_IGN ) != SIG_IGN )
    signal( SIGPIPE, fatal_signal );
#endif
#ifdef SIGCHLD
  signal( SIGCHLD, SIG_DFL );
#endif
}

int main(
  int    argc,
  char** argv
)
{
  int                                 opt;
  Trace::TraceReaderLogQEMU           log;
  Trace::TraceWriterQEMU              trace;
  std::string                         cpuname;
  std::string                         executable;
  std::string                         tracefile;
  std::string                         logname = "/tmp/qemu.log";
  Coverage::ExecutableInfo*           executableInfo;
  rld::process::tempfile              objdumpFile( ".dmp" );
  rld::process::tempfile              err( ".err" );
  Coverage::DesiredSymbols            symbolsToAnalyze;
  bool                                verbose = false;
  std::string                         dynamicLibrary;
  int                                 ec = 0;
  std::shared_ptr<Target::TargetBase> targetInfo;

  setup_signals();

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
      case 'v': verbose = true;          break;
      default:  usage();
    }
  }

  // Make sure we have all the required parameters
  if ( cpuname.empty() ) {
    std::cerr << "cpuname not specified" << std::endl;
    usage();
  }

  if ( executable.empty() ) {
    std::cerr << "executable not specified" << std::endl;
    usage();
  }

  if ( tracefile.empty() ) {
    std::cerr << "output trace file not specified" << std::endl;
    usage();
  }


  // Create toolnames.
  try
  {
    targetInfo.reset( Target::TargetFactory( cpuname ) );
  }
  catch ( rld::error re )
  {
    std::cerr << "error: "
              << re.where << ": " << re.what
              << std::endl;
    ec = 10;

    return ec;
  }

  Coverage::ObjdumpProcessor objdumpProcessor( symbolsToAnalyze, targetInfo );

  if ( !dynamicLibrary.empty() )
    executableInfo = new Coverage::ExecutableInfo(
      executable.c_str(),
      dynamicLibrary,
      false,
      symbolsToAnalyze
    );
  else {
    try
    {
      executableInfo = new Coverage::ExecutableInfo(
        executable.c_str(),
        "",
        false,
        symbolsToAnalyze
      );
    }
    catch ( rld::error re )
    {
      std::cerr << "error: "
                << re.where << ": " << re.what
                << std::endl;
      ec = 10;
    }
  }

  // If a dynamic library was specified, determine the load address.
  if ( !dynamicLibrary.empty() ) {
    try
    {
      executableInfo->setLoadAddress(
        objdumpProcessor.determineLoadAddress( executableInfo )
      );
    }
    catch ( rld::error re )
    {
      std::cerr << "error: "
                << re.where << ": " << re.what
                << std::endl;
      ec = 10;

      return ec;
    }
  }

  try
  {
    objdumpProcessor.loadAddressTable( executableInfo, objdumpFile, err );
    log.processFile( logname.c_str(), objdumpProcessor );
    trace.writeFile( tracefile.c_str(), &log, verbose );
  }
  catch ( rld::error re )
  {
    std::cerr << "error: "
              << re.where << ": " << re.what
              << std::endl;
    ec = 10;
  }

  return ec;
}
