#include <iostream>
#include <iomanip>

#include <cxxabi.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <list>

#include <rld.h>
#include <rld-process.h>

#include "app_common.h"
#include "CoverageFactory.h"
#include "CoverageMap.h"
#include "DesiredSymbols.h"
#include "ExecutableInfo.h"
#include "Explanations.h"
#include "ObjdumpProcessor.h"
#include "ReportsBase.h"
#include "TargetFactory.h"
#include "GcovData.h"

#ifdef _WIN32
  #define kill(p,s) raise(s)
#endif

typedef std::list<std::string> CoverageNames;
typedef std::list<Coverage::ExecutableInfo*> Executables;
typedef std::string option_error;

/*
 * Create a build path from the executable paths. Also extract the build prefix
 * and BSP names.
 */
static void createBuildPath(Executables& executablesToAnalyze,
                            std::string& buildPath,
                            std::string& buildPrefix,
                            std::string& buildBSP)
{
  for (const auto& exe : executablesToAnalyze) {
    rld::strings eparts;
    rld::split(eparts, rld::path::path_abs(exe->getFileName()), RLD_PATH_SEPARATOR);
    std::string fail; // empty means all is OK else an error string
    for (rld::path::paths::reverse_iterator pri = eparts.rbegin();
         pri != eparts.rend();
         ++pri) {
      if (*pri == "testsuites") {
        ++pri;
        if (pri == eparts.rend()) {
          fail = "invalid executable path, no BSP";
          break;
        }
        if (buildBSP.empty()) {
          buildBSP = *pri;
        } else {
          if (buildBSP != *pri) {
            fail = "executable BSP does not match: " + buildBSP;
            break;
          }
        }
        ++pri;
        if (pri == eparts.rend() || *pri != "c") {
          fail = "invalid executable path, no 'c'";
          break;
        }
        ++pri;
        if (pri == eparts.rend()) {
          fail = "invalid executable path, no arch prefix";
          break;
        }
        if (buildPrefix.empty()) {
          buildPrefix = *pri;
        } else {
          if (buildPrefix != *pri) {
            fail = "executable build prefix does not match: " + buildPrefix;
            break;
          }
        }
        ++pri;
        if (pri == eparts.rend()) {
          fail = "invalid executable path, no build top";
          break;
        }
        //
        // The remaining parts of the path is the build path. Iterator over them
        // and collect into a new paths variable to join to make a path.
        //
        rld::path::paths bparts;
        for (; pri != eparts.rend(); ++pri)
          bparts.insert(bparts.begin(), *pri);
        std::string thisBuildPath;
        rld::path::path_join(thisBuildPath, bparts, thisBuildPath);
        if (buildPath.empty()) {
          buildPath = thisBuildPath;
        } else {
          if (buildPath != thisBuildPath) {
            fail = "executable build path does not match: " + buildPath;
          }
        }
        break;
      }
    }
    if (!fail.empty()) {
      throw rld::error( fail, "createBuildPath" );
    }
  }
}

/*
 *  Print program usage message
 */
void usage(const std::string& progname)
{
  std::cerr <<"Usage: " << progname
            <<" [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -1 EXECUTABLE coverage1 ... coverageN" << std::endl
            << "--OR--" << std::endl
            << "Usage: " << progname
            << " [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -e EXE_EXTENSION -c COVERAGEFILE_EXTENSION EXECUTABLE1 ... EXECUTABLE2" << std::endl
            << std::endl
            << "  -v                        - verbose at initialization" << std::endl
            << "  -T TARGET                 - target name" << std::endl
            << "  -f FORMAT                 - coverage file format (RTEMS, QEMU, TSIM or Skyeye)" << std::endl
            << "  -E EXPLANATIONS           - name of file with explanations" << std::endl
            << "  -s SYMBOL_SET_FILE        - path to the INI format symbol sets" << std::endl
            << "  -1 EXECUTABLE             - name of executable to get symbols from" << std::endl
            << "  -e EXE_EXTENSION          - extension of the executables to analyze" << std::endl
            << "  -c COVERAGEFILE_EXTENSION - extension of the coverage files to analyze" << std::endl
            << "  -g GCNOS_LIST             - name of file with list of *.gcno files" << std::endl
            << "  -p PROJECT_NAME           - name of the project" << std::endl
            << "  -C ConfigurationFileName  - name of configuration file" << std::endl
            << "  -O Output_Directory       - name of output directory (default=." << std::endl
            << "  -d debug                  - disable cleaning of tempfile" << std::endl
            << std::endl;
}

int covoar(
  int    argc,
  char** argv
)
{
  CoverageNames                 coverageFileNames;
  std::string                   coverageFileName;
  Executables                   executablesToAnalyze;
  Coverage::ExecutableInfo*     executableInfo = NULL;
  std::string                   executableExtension = "exe";
  std::string                   coverageExtension = "cov";
  Coverage::CoverageFormats_t   coverageFormat = Coverage::COVERAGE_FORMAT_QEMU;
  Coverage::CoverageReaderBase* coverageReader = NULL;
  char*                         executable = NULL;
  const char*                   explanations = NULL;
  const char*                   gcnosFileName = NULL;
  char                          gcnoFileName[FILE_NAME_LENGTH];
  char                          gcdaFileName[FILE_NAME_LENGTH];
  char                          gcovBashCommand[256];
  std::string                   target;
  const char*                   format = "html";
  FILE*                         gcnosFile = NULL;
  Gcov::GcovData*               gcovFile;
  const char*                   singleExecutable = NULL;
  rld::process::tempfile        objdumpFile( ".dmp" );
  rld::process::tempfile        err( ".err" );
  rld::process::tempfile        syms( ".syms" );
  bool                          debug = false;
  std::string                   symbolSet;
  std::string                   option;
  int                           opt;

  //
  // Process command line options.
  //

  while ((opt = getopt(argc, argv, "1:L:e:c:g:E:f:s:S:T:O:p:vd")) != -1) {
    switch (opt) {
      case '1': singleExecutable    = optarg; break;
      case 'L': dynamicLibrary      = optarg; break;
      case 'e': executableExtension = optarg; break;
      case 'c': coverageExtension   = optarg; break;
      case 'g': gcnosFileName       = optarg; break;
      case 'E': explanations        = optarg; break;
      case 'f': format              = optarg; break;
      case 'S': symbolSet           = optarg; break;
      case 'T': target              = optarg; break;
      case 'O': outputDirectory     = optarg; break;
      case 'v': Verbose             = true;
                rld::verbose_inc ();          break;
      case 'p': projectName         = optarg; break;
      case 'd': debug               = true;   break;
      default: /* '?' */
        throw option_error( "unknown option" );
    }
  }

  /*
   * Validate inputs.
   */

  /*
   * Validate that we have a symbols of interest file.
   */
  if ( symbolSet.empty() )
    throw option_error( "symbol set file -S" );

  /*
   * Has path to explanations.txt been specified.
   */
  if ( !explanations )
    throw option_error( "explanations -E" );

  /*
   * Check for project name.
   */
  if ( !projectName )
    throw option_error( "project name -p" );

  // If a single executable was specified, process the remaining
  // arguments as coverage file names.
  if (singleExecutable) {

    // Ensure that the executable is readable.
    if (!FileIsReadable( singleExecutable )) {
      std::cerr << "warning: Unable to read executable: " << singleExecutable
                << std::endl;
    } else {

      for (int i = optind; i < argc; i++) {
        // Ensure that the coverage file is readable.
        if (!FileIsReadable( argv[i] )) {
          std::cerr << "warning: Unable to read coverage file: " << argv[i]
                    << std::endl;
        } else {
          coverageFileNames.push_back( argv[i] );
        }
      }

      // If there was at least one coverage file, create the
      // executable information.
      if (!coverageFileNames.empty()) {
        if (dynamicLibrary) {
          executableInfo = new Coverage::ExecutableInfo(
            singleExecutable, dynamicLibrary
          );
        } else {
          executableInfo = new Coverage::ExecutableInfo( singleExecutable );
        }

        executablesToAnalyze.push_back( executableInfo );
      }
    }
  }
  else {
    // If not invoked with a single executable, process the remaining
    // arguments as executables and derive the coverage file names.
    for (int i = optind; i < argc; i++) {
      // Ensure that the executable is readable.
      if (!FileIsReadable( argv[i] )) {
        std::cerr << "warning: Unable to read executable: " << argv[i] << std::endl;
      } else {
        coverageFileName = argv[i];
        coverageFileName.append( "." + coverageExtension );

        if (!FileIsReadable( coverageFileName.c_str() )) {
          std::cerr << "warning: Unable to read coverage file: " << coverageFileName
                    << std::endl;
        } else {
          executableInfo = new Coverage::ExecutableInfo( argv[i] );
          executablesToAnalyze.push_back( executableInfo );
          coverageFileNames.push_back( coverageFileName );
        }
      }
    }
  }

  // Ensure that there is at least one executable to process.
  if (executablesToAnalyze.empty())
    throw rld::error( "No information to analyze", "covoar" );

  // The executablesToAnalyze and coverageFileNames containers need
  // to be the name size of some of the code below breaks. Lets
  // check and make sure.
  if (executablesToAnalyze.size() != coverageFileNames.size())
    throw rld::error( "executables and coverage name size mismatch", "covoar" );

  //
  // Find the top of the BSP's build tree and if we have found the top
  // check the executable is under the same path and BSP.
  //
  std::string buildPath;
  std::string buildTarget;
  std::string buildBSP;
  createBuildPath(executablesToAnalyze,
                  buildPath,
                  buildTarget,
                  buildBSP);

  //
  // Use a command line target if provided.
  //
  if (!target.empty()) {
    buildTarget = target;
  }

  if (Verbose) {
    if (singleExecutable) {
      std::cerr << "Processing a single executable and multiple coverage files"
                << std::endl;
    } else {
      std::cerr << "Processing multiple executable/coverage file pairs" << std::endl;
    }
    std::cerr << "Coverage Format : " << format << std::endl
              << "Target          : " << buildTarget.c_str() << std::endl
              << std::endl;

    // Process each executable/coverage file pair.
    Executables::iterator eitr = executablesToAnalyze.begin();
    for (const auto& cname : coverageFileNames) {
      std::cerr << "Coverage file " << cname
                << " for executable: " << (*eitr)->getFileName() << std::endl;
      if (!singleExecutable)
        eitr++;
    }
  }

  //
  // Create data to support analysis.
  //

  // Create data based on target.
  TargetInfo = Target::TargetFactory( buildTarget );

  // Create the set of desired symbols.
  SymbolsToAnalyze = new Coverage::DesiredSymbols();

  //
  // Read symbol configuration file and load needed symbols.
  //
  SymbolsToAnalyze->load( symbolSet, buildTarget, buildBSP, Verbose );

  if ( Verbose )
    std::cerr << "Analyzing " << SymbolsToAnalyze->set.size()
              << " symbols" << std::endl;

  // Create explanations.
  AllExplanations = new Coverage::Explanations();
  if ( explanations )
    AllExplanations->load( explanations );

  // Create coverage map reader.
  coverageReader = Coverage::CreateCoverageReader(coverageFormat);
  if (!coverageReader)
    throw rld::error( "Unable to create coverage file reader", "covoar" );

  // Create the objdump processor.
  objdumpProcessor = new Coverage::ObjdumpProcessor();

  // Prepare each executable for analysis.
  for (auto& exe : executablesToAnalyze) {
    if (Verbose)
      std::cerr << "Extracting information from: " << exe->getFileName()
                << std::endl;

    // If a dynamic library was specified, determine the load address.
    if (dynamicLibrary) {
      exe->setLoadAddress( objdumpProcessor->determineLoadAddress( exe ) );
    }

    // Load the objdump for the symbols in this executable.
    objdumpProcessor->load( exe, objdumpFile, err );
  }

  //
  // Analyze the coverage data.
  //

  // Process each executable/coverage file pair.
  Executables::iterator eitr = executablesToAnalyze.begin();
  for (const auto& cname : coverageFileNames) {
    Coverage::ExecutableInfo* exe = *eitr;
    if (Verbose)
      std::cerr << "Processing coverage file " << cname
                << " for executable " << exe->getFileName()
                << std::endl;

    // Process its coverage file.
    coverageReader->processFile( cname.c_str(), exe );

    // Merge each symbols coverage map into a unified coverage map.
    exe->mergeCoverage();

    // DEBUG Print ExecutableInfo content
    //exe->dumpExecutableInfo();

    if (!singleExecutable) {
      eitr++;
    }
  }

  // Do necessary preprocessing of uncovered ranges and branches
  if (Verbose)
    std::cerr << "Preprocess uncovered ranges and branches" << std::endl;

  SymbolsToAnalyze->preprocess();

  //
  // Generate Gcov reports
  //
  if (gcnosFileName) {
    if (Verbose)
      std::cerr << "Generating Gcov reports..." << std::endl;

    gcnosFile = fopen ( gcnosFileName , "r" );

    if ( !gcnosFile )
      std::cerr << "Unable to open " << gcnosFileName << std::endl;
    else {
      while ( fscanf( gcnosFile, "%s", inputBuffer ) != EOF) {
        gcovFile = new Gcov::GcovData();
        strcpy( gcnoFileName, inputBuffer );

        if ( Verbose )
          std::cerr << "Processing file: " << gcnoFileName << std::endl;

        if ( gcovFile->readGcnoFile( gcnoFileName ) ) {
          // Those need to be in this order
          gcovFile->processCounters();
          gcovFile->writeReportFile();
          gcovFile->writeGcdaFile();
          gcovFile->writeGcovFile();
        }

        delete gcovFile;
      }
      fclose( gcnosFile );
    }
  }

  // Determine the uncovered ranges and branches.
  if (Verbose)
    std::cerr << "Computing uncovered ranges and branches" << std::endl;

  SymbolsToAnalyze->computeUncovered();

  // Calculate remainder of statistics.
  if (Verbose)
    std::cerr << "Calculate statistics" << std::endl;

  SymbolsToAnalyze->calculateStatistics();

  // Look up the source lines for any uncovered ranges and branches.
  if (Verbose)
    std::cerr << "Looking up source lines for uncovered ranges and branches"
              << std::endl;

  SymbolsToAnalyze->findSourceForUncovered();

  //
  // Report the coverage data.
  //
  if (Verbose)
    std::cerr << "Generate Reports" << std::endl;

  Coverage::GenerateReports();

  // Write explanations that were not found.
  if ( explanations ) {
    std::string notFound;

    notFound = outputDirectory;
    notFound += "/";
    notFound += "ExplanationsNotFound.txt";

    if (Verbose)
      std::cerr << "Writing Not Found Report (" << notFound<< ')' << std::endl;

    AllExplanations->writeNotFound( notFound.c_str() );
  }

  //Leave tempfiles around if debug flag (-d) is enabled.
  if ( debug ) {
    objdumpFile.override( "objdump_file" );
    objdumpFile.keep();
    err.override( "objdump_exec_log" );
    err.keep();
    syms.override( "symbols_list" );
    syms.keep();
  }

  return 0;
}

#define PrintableString(_s) \
((!(_s)) ? "NOT SET" : (_s))

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
  if ( signal( SIGINT, SIG_IGN ) != SIG_IGN )
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

void
unhandled_exception (void)
{
  std::cerr << "error: exception handling error, please report" << std::endl;
  exit (1);
}

int main(
  int    argc,
  char** argv
)
{
  std::string progname( argv[0] );
  int         ec = 0;

  setup_signals();

  std::set_terminate(unhandled_exception);

  try
  {
    progname = rld::path::basename(argv[0]);
    covoar( argc, argv );
  }
  catch ( option_error oe )
  {
    std::cerr << "error: missing option: " + oe << std::endl;
    usage(progname);
    ec = EXIT_FAILURE;
  }
  catch (rld::error re)
  {
    std::cerr << "error: "
              << re.where << ": " << re.what
              << std::endl;
    ec = 10;
  }
  catch (std::exception e)
  {
    int   status;
    char* realname;
    realname = abi::__cxa_demangle (e.what(), 0, 0, &status);
    std::cerr << "error: exception: " << realname << " [";
    ::free (realname);
    const std::type_info &ti = typeid (e);
    realname = abi::__cxa_demangle (ti.name(), 0, 0, &status);
    std::cerr << realname << "] " << e.what () << std::endl << std::flush;
    ::free (realname);
    ec = 11;
  }
  catch (...)
  {
    /*
     * Helps to know if this happens.
     */
    std::cerr << "error: unhandled exception" << std::endl;
    ec = 12;
  }

  return ec;
}
