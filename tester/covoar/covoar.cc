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

#include "rld-process.h"

#ifdef _WIN32
  #define kill(p,s) raise(s)
#endif

typedef std::list<std::string> CoverageNames;
typedef std::list<Coverage::ExecutableInfo*> Executables;

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
          if (buildBSP != *pri) {
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
          if (buildBSP != *pri) {
            fail = "executable build path does not match: " + buildPath;
          }
        }
        break;
      }
    }
    if (!fail.empty()) {
      std::cerr << "ERROR: " << fail << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

/*
 *  Print program usage message
 */
void usage(const std::string& progname)
{
  fprintf(
    stderr,
    "Usage: %s [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -1 EXECUTABLE coverage1 ... coverageN\n"
    "--OR--\n"
    "Usage: %s [-v] -T TARGET -f FORMAT [-E EXPLANATIONS] -e EXE_EXTENSION -c COVERAGEFILE_EXTENSION EXECUTABLE1 ... EXECUTABLE2\n"
    "\n"
    "  -v                        - verbose at initialization\n"
    "  -T TARGET                 - target name\n"
    "  -f FORMAT                 - coverage file format "
           "(RTEMS, QEMU, TSIM or Skyeye)\n"
    "  -E EXPLANATIONS           - name of file with explanations\n"
    "  -s SYMBOL_SET_FILE        - path to the INI format symbol sets\n"
    "  -1 EXECUTABLE             - name of executable to get symbols from\n"
    "  -e EXE_EXTENSION          - extension of the executables to analyze\n"
    "  -c COVERAGEFILE_EXTENSION - extension of the coverage files to analyze\n"
    "  -g GCNOS_LIST             - name of file with list of *.gcno files\n"
    "  -p PROJECT_NAME           - name of the project\n"
    "  -C ConfigurationFileName  - name of configuration file\n"
    "  -O Output_Directory       - name of output directory (default=.\n"
    "  -d debug                  - disable cleaning of tempfiles."
    "\n",
    progname.c_str(),
    progname.c_str()
  );
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

int main(
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
  Coverage::CoverageFormats_t   coverageFormat;
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
  std::string                   progname;
  std::string                   option;
  int                           opt;

  setup_signals();

  //
  // Process command line options.
  //
  progname = rld::path::basename(argv[0]);

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
      case 'v': Verbose             = true;   break;
      case 'p': projectName         = optarg; break;
      case 'd': debug               = true;   break;
      default: /* '?' */
        usage(progname);
        exit(EXIT_FAILURE);
    }
  }
  try
  {
    /*
     * Validate inputs.
     */

    /*
     * Validate that we have a symbols of interest file.
     */
    if ( symbolSet.empty() ) {
      option = "symbol set file -S";
      throw option;
    }

    /*
     * Has path to explanations.txt been specified.
     */
    if ( !explanations ) {
      option = "explanations -E";
      throw option;
    }

    /*
     * Check for project name.
     */
    if ( !projectName ) {
      option = "project name -p";
      throw option;
    }
  }
  catch( std::string option )
  {
    std::cerr << "error missing option: " + option << std::endl;
    usage(progname);
    exit(EXIT_FAILURE);
  }

  // If a single executable was specified, process the remaining
  // arguments as coverage file names.
  if (singleExecutable) {

    // Ensure that the executable is readable.
    if (!FileIsReadable( singleExecutable )) {
      fprintf(
        stderr,
        "WARNING: Unable to read executable %s\n",
        singleExecutable
      );
    } else {

      for (int i = optind; i < argc; i++) {
        // Ensure that the coverage file is readable.
        if (!FileIsReadable( argv[i] )) {
          fprintf(
            stderr,
            "WARNING: Unable to read coverage file %s\n",
            argv[i]
          );
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
        fprintf(
          stderr,
          "WARNING: Unable to read executable %s\n",
          argv[i]
        );
      } else {
        coverageFileName = argv[i];
        coverageFileName.replace(
          coverageFileName.length() - executableExtension.size(),
          executableExtension.size(),
          coverageExtension
        );

        if (!FileIsReadable( coverageFileName.c_str() )) {
          fprintf(
            stderr,
            "WARNING: Unable to read coverage file %s\n",
            coverageFileName.c_str()
          );
        } else {
          executableInfo = new Coverage::ExecutableInfo( argv[i] );
          executablesToAnalyze.push_back( executableInfo );
          coverageFileNames.push_back( coverageFileName );
        }
      }
    }
  }

  // Ensure that there is at least one executable to process.
  if (executablesToAnalyze.empty()) {
    fprintf(
      stderr, "ERROR: No information to analyze\n"
    );
    exit(EXIT_FAILURE);
  }

  // The executablesToAnalyze and coverageFileNames containers need
  // to be the name size of some of the code below breaks. Lets
  // check and make sure.
  if (executablesToAnalyze.size() != coverageFileNames.size()) {
    std::cerr << "ERROR: executables and coverage name size mismatch" << std::endl;
    exit(EXIT_FAILURE);
  }

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
      fprintf(
        stderr,
        "Processing a single executable and multiple coverage files\n"
      );
    } else {
      fprintf(
        stderr,
        "Processing multiple executable/coverage file pairs\n"
      );
    }
    fprintf( stderr, "Coverage Format : %s\n", format );
    fprintf( stderr, "Target          : %s\n", buildTarget.c_str() );
    fprintf( stderr, "\n" );

    // Process each executable/coverage file pair.
    Executables::iterator eitr = executablesToAnalyze.begin();
    for (CoverageNames::iterator citr = coverageFileNames.begin();
         citr != coverageFileNames.end();
         citr++) {

      fprintf(
        stderr,
        "Coverage file %s for executable %s\n",
        (*citr).c_str(),
        ((*eitr)->getFileName()).c_str()
      );

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
  if (!SymbolsToAnalyze->load( symbolSet, buildTarget, buildBSP, Verbose )) {
      exit(EXIT_FAILURE);
  }

  if ( Verbose )
    std::cout << "Analyzing " << SymbolsToAnalyze->set.size()
              << " symbols" << std::endl;

  // Create explanations.
  AllExplanations = new Coverage::Explanations();
  if ( explanations )
    AllExplanations->load( explanations );

  // Create coverage map reader.
  coverageReader = Coverage::CreateCoverageReader(coverageFormat);
  if (!coverageReader) {
    fprintf( stderr, "ERROR: Unable to create coverage file reader\n" );
    exit(EXIT_FAILURE);
  }

  // Create the objdump processor.
  objdumpProcessor = new Coverage::ObjdumpProcessor();

  // Prepare each executable for analysis.
  for (Executables::iterator eitr = executablesToAnalyze.begin();
       eitr != executablesToAnalyze.end();
       eitr++) {

    if (Verbose) {
      fprintf(
        stderr,
        "Extracting information from %s\n",
        ((*eitr)->getFileName()).c_str()
      );
    }

    // If a dynamic library was specified, determine the load address.
    if (dynamicLibrary) {
      (*eitr)->setLoadAddress(
        objdumpProcessor->determineLoadAddress( *eitr )
      );
    }

    // Load the objdump for the symbols in this executable.
    objdumpProcessor->load( *eitr, objdumpFile, err );
  }

  //
  // Analyze the coverage data.
  //

  // Process each executable/coverage file pair.
  Executables::iterator eitr = executablesToAnalyze.begin();
  for (const auto& cname : coverageFileNames) {
    if (Verbose) {
      fprintf(
        stderr,
        "Processing coverage file %s for executable %s\n",
        cname.c_str(),
        ((*eitr)->getFileName()).c_str()
      );
    }

    // Process its coverage file.
    coverageReader->processFile( cname.c_str(), *eitr );

    // Merge each symbols coverage map into a unified coverage map.
    (*eitr)->mergeCoverage();

    // DEBUG Print ExecutableInfo content
    //(*eitr)->dumpExecutableInfo();

    if (!singleExecutable) {
      eitr++;
    }
  }

  // Do necessary preprocessing of uncovered ranges and branches
  if (Verbose) {
    fprintf( stderr, "Preprocess uncovered ranges and branches\n" );
  }
  SymbolsToAnalyze->preprocess();

  //
  // Generate Gcov reports
  //
  if (Verbose) {
    fprintf( stderr, "Generating Gcov reports...\n");
  }
  gcnosFile = fopen ( gcnosFileName , "r" );

  if ( !gcnosFile ) {
    fprintf( stderr, "Unable to open %s\n", gcnosFileName );
  }
  else {
    while ( fscanf( gcnosFile, "%s", inputBuffer ) != EOF) {
      gcovFile = new Gcov::GcovData();
      strcpy( gcnoFileName, inputBuffer );

      if ( Verbose ) {
        fprintf( stderr, "Processing file: %s\n", gcnoFileName );
      }

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

  // Determine the uncovered ranges and branches.
  if (Verbose) {
    fprintf( stderr, "Computing uncovered ranges and branches\n" );
  }
  SymbolsToAnalyze->computeUncovered();

  // Calculate remainder of statistics.
  if (Verbose) {
    fprintf( stderr, "Calculate statistics\n" );
  }
  SymbolsToAnalyze->calculateStatistics();

  // Look up the source lines for any uncovered ranges and branches.
  if (Verbose) {
    fprintf(
      stderr, "Looking up source lines for uncovered ranges and branches\n"
    );
  }
  SymbolsToAnalyze->findSourceForUncovered();

  //
  // Report the coverage data.
  //
  if (Verbose) {
    fprintf(
      stderr, "Generate Reports\n"
    );
  }
  Coverage::GenerateReports();

  // Write explanations that were not found.
  if ( explanations ) {
    std::string notFound;

    notFound = outputDirectory;
    notFound += "/";
    notFound += "ExplanationsNotFound.txt";

    if (Verbose) {
      fprintf( stderr, "Writing Not Found Report (%s)\n", notFound.c_str() );
    }
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
