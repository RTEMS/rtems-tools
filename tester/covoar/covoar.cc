#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <list>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

/*
 *  Variables to control general behavior
 */
const char*                          coverageFileExtension = NULL;
std::list<std::string>               coverageFileNames;
int                                  coverageExtensionLength = 0;
Coverage::CoverageFormats_t          coverageFormat;
Coverage::CoverageReaderBase*        coverageReader = NULL;
char*                                executable = NULL;
const char*                          executableExtension = NULL;
int                                  executableExtensionLength = 0;
std::list<Coverage::ExecutableInfo*> executablesToAnalyze;
const char*                          explanations = NULL;
char*                                progname;
const char*                          symbolsFile = NULL;
const char*		             gcnosFileName = NULL;
char				     gcnoFileName[FILE_NAME_LENGTH];
char				     gcdaFileName[FILE_NAME_LENGTH];
char				     gcovBashCommand[256];
const char*                          target = NULL;
const char*                          format = NULL;
FILE*				     gcnosFile = NULL;
Gcov::GcovData* 		     gcovFile;

/*
 *  Print program usage message
 */
void usage()
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
    "  -s SYMBOLS_FILE           - name of file with symbols of interest\n"
    "  -1 EXECUTABLE             - name of executable to get symbols from\n"
    "  -e EXE_EXTENSION          - extension of the executables to analyze\n"
    "  -c COVERAGEFILE_EXTENSION - extension of the coverage files to analyze\n"
    "  -g GCNOS_LIST             - name of file with list of *.gcno files\n"
    "  -p PROJECT_NAME           - name of the project\n"
    "  -C ConfigurationFileName  - name of configuration file\n"
    "  -O Output_Directory       - name of output directory (default=."
    "\n",
    progname,
    progname
  );
}

#define PrintableString(_s) \
       ((!(_s)) ? "NOT SET" : (_s))

/*
 *  Configuration File Support
 */
#include "ConfigFile.h"
Configuration::FileReader *CoverageConfiguration;
Configuration::Options_t Options[] = {
  { "explanations",         NULL },
  { "format",               NULL },
  { "symbolsFile",          NULL },
  { "outputDirectory",      NULL },
  { "executableExtension",  NULL },
  { "coverageExtension",    NULL },
  { "gcnosFile",            NULL },
  { "target",               NULL },
  { "verbose",              NULL },
  { "projectName",          NULL },
  { NULL,                   NULL }
};

bool isTrue(const char *value)
{
  if ( !value )                  return false;
  if ( !strcmp(value, "true") )  return true;
  if ( !strcmp(value, "TRUE") )  return true;
  if ( !strcmp(value, "yes") )   return true;
  if ( !strcmp(value, "YES") )   return true;
  return false;
}

#define GET_BOOL(_opt, _val) \
  if (isTrue(CoverageConfiguration->getOption(_opt))) \
    _val = true;

#define GET_STRING(_opt, _val) \
  do { \
    const char *_t; \
    _t = CoverageConfiguration->getOption(_opt); \
    if ( _t ) _val = _t; \
  } while(0)
  

void check_configuration(void)
{
  GET_BOOL( "verbose", Verbose );

  GET_STRING( "format",               format );
  GET_STRING( "target",               target );
  GET_STRING( "explanations",         explanations );
  GET_STRING( "symbolsFile",          symbolsFile );
  GET_STRING( "outputDirectory",      outputDirectory );
  GET_STRING( "executableExtension",  executableExtension );
  GET_STRING( "coverageExtension",    coverageFileExtension );
  GET_STRING( "gcnosFile",            gcnosFileName );
  GET_STRING( "projectName",          projectName );

  // Now calculate some values
  if ( coverageFileExtension )
    coverageExtensionLength = strlen( coverageFileExtension );

  if ( executableExtension )
    executableExtensionLength = strlen( executableExtension );

  if ( format )
    coverageFormat = Coverage::CoverageFormatToEnum( format );
}

int main(
  int    argc,
  char** argv
)
{
  std::list<std::string>::iterator               citr;
  std::string                                    coverageFileName;
  std::list<Coverage::ExecutableInfo*>::iterator eitr;
  Coverage::ExecutableInfo*                      executableInfo = NULL;
  int                                            i;
  int                                            opt;
  const char*                                    singleExecutable = NULL;

  CoverageConfiguration = new Configuration::FileReader(Options);
  
  //
  // Process command line options.
  //
  progname = argv[0];

  while ((opt = getopt(argc, argv, "C:1:L:e:c:g:E:f:s:T:O:p:v")) != -1) {
    switch (opt) {
      case 'C': CoverageConfiguration->processFile( optarg ); break;
      case '1': singleExecutable      = optarg; break;
      case 'L': dynamicLibrary        = optarg; break;
      case 'e': executableExtension   = optarg; break;
      case 'c': coverageFileExtension = optarg; break;
      case 'g': gcnosFileName         = optarg; break;
      case 'E': explanations          = optarg; break;
      case 'f': format                = optarg; break;
      case 's': symbolsFile           = optarg; break;
      case 'T': target                = optarg; break;
      case 'O': outputDirectory       = optarg; break;
      case 'v': Verbose               = true;   break;
      case 'p': projectName           = optarg; break;
      default: /* '?' */
        usage();
        exit( -1 );
    }
  }

  // Do not trust any arguments until after this point.
  check_configuration();

  // XXX We need to verify that all of the needed arguments are non-NULL.

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
    }

    else {

      for (i=optind; i < argc; i++) {

        // Ensure that the coverage file is readable.
        if (!FileIsReadable( argv[i] )) {
          fprintf(
            stderr,
            "WARNING: Unable to read coverage file %s\n",
            argv[i]
          );
        }

        else
          coverageFileNames.push_back( argv[i] );
      }

      // If there was at least one coverage file, create the
      // executable information.
      if (!coverageFileNames.empty()) {
        if (dynamicLibrary)
          executableInfo = new Coverage::ExecutableInfo(
            singleExecutable, dynamicLibrary
          );
        else
          executableInfo = new Coverage::ExecutableInfo( singleExecutable );

        executablesToAnalyze.push_back( executableInfo );
      }
    }
  }

  // If not invoked with a single executable, process the remaining
  // arguments as executables and derive the coverage file names.
  else {
    for (i = optind; i < argc; i++) {

      // Ensure that the executable is readable.
      if (!FileIsReadable( argv[i] )) {
        fprintf(
          stderr,
          "WARNING: Unable to read executable %s\n",
          argv[i]
        );
      }

      else {
        coverageFileName = argv[i];
        coverageFileName.replace(
          coverageFileName.length() - executableExtensionLength,
          executableExtensionLength,
          coverageFileExtension
        );

        if (!FileIsReadable( coverageFileName.c_str() )) {
          fprintf(
            stderr,
            "WARNING: Unable to read coverage file %s\n",
            coverageFileName.c_str()
          );
        }

        else {
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
    exit( -1 );
  }

  if (Verbose) {
    if (singleExecutable)
      fprintf(
        stderr,
        "Processing a single executable and multiple coverage files\n"
      );
    else
      fprintf(
        stderr,
        "Processing multiple executable/coverage file pairs\n"
      );
    fprintf( stderr, "Coverage Format : %s\n", format );
    fprintf( stderr, "Target          : %s\n", PrintableString(target) );
    fprintf( stderr, "\n" );
#if 1
    // Process each executable/coverage file pair.
    eitr = executablesToAnalyze.begin();
    for (citr = coverageFileNames.begin();
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
#endif
  }

  //
  // Validate inputs.
  //

  // Target name must be set.
  if (!target) {
    fprintf( stderr, "ERROR: target not specified\n" );
    usage();
    exit(-1);
  }

  // Validate format.
  if (!format) {
    fprintf( stderr, "ERROR: coverage format report not specified\n" );
    usage();
    exit(-1);
  }

  // Validate that we have a symbols of interest file.
  if (!symbolsFile) {
    fprintf( stderr, "ERROR: symbols of interest file not specified\n" );
    usage();
    exit(-1);
  }

  //
  // Create data to support analysis.
  //

  // Create data based on target.
  TargetInfo = Target::TargetFactory( target );

  // Create the set of desired symbols.
  SymbolsToAnalyze = new Coverage::DesiredSymbols();
  SymbolsToAnalyze->load( symbolsFile );
  if (Verbose)
    fprintf(
      stderr,
      "Analyzing %u symbols\n",
      (unsigned int) SymbolsToAnalyze->set.size()
    );

  // Create explanations.
  AllExplanations = new Coverage::Explanations();
  if ( explanations )
    AllExplanations->load( explanations );

  // Create coverage map reader.
  coverageReader = Coverage::CreateCoverageReader(coverageFormat);
  if (!coverageReader) {
    fprintf( stderr, "ERROR: Unable to create coverage file reader\n" );
    exit(-1);
  }

  // Create the objdump processor.
  objdumpProcessor = new Coverage::ObjdumpProcessor();

  // Prepare each executable for analysis.
  for (eitr = executablesToAnalyze.begin();
       eitr != executablesToAnalyze.end();
       eitr++) {

    if (Verbose)
      fprintf(
        stderr,
        "Extracting information from %s\n",
        ((*eitr)->getFileName()).c_str()
      );

    // If a dynamic library was specified, determine the load address.
    if (dynamicLibrary)
      (*eitr)->setLoadAddress(
        objdumpProcessor->determineLoadAddress( *eitr )
      );

    // Load the objdump for the symbols in this executable.
    objdumpProcessor->load( *eitr );
  }

  //
  // Analyze the coverage data.
  //

  // Process each executable/coverage file pair.
  eitr = executablesToAnalyze.begin();
  for (citr = coverageFileNames.begin();
       citr != coverageFileNames.end();
       citr++) {

    if (Verbose)
      fprintf(
        stderr,
        "Processing coverage file %s for executable %s\n",
        (*citr).c_str(),
        ((*eitr)->getFileName()).c_str()
      );

    // Process its coverage file.
    coverageReader->processFile( (*citr).c_str(), *eitr );

    // Merge each symbols coverage map into a unified coverage map.
    (*eitr)->mergeCoverage();

    // DEBUG Print ExecutableInfo content
    //(*eitr)->dumpExecutableInfo();

    if (!singleExecutable)
      eitr++;
  }

  // Do necessary preprocessing of uncovered ranges and branches
  if (Verbose)
    fprintf( stderr, "Preprocess uncovered ranges and branches\n" );
  SymbolsToAnalyze->preprocess();

  //
  // Generate Gcov reports
  //
  if (Verbose)
    fprintf( stderr, "Generating Gcov reports...\n");
  gcnosFile = fopen ( gcnosFileName , "r" );	

  if ( !gcnosFile ) {
    fprintf( stderr, "Unable to open %s\n", gcnosFileName );
  }
  else {
    while ( fscanf( gcnosFile, "%s", inputBuffer ) != EOF) {
      gcovFile = new Gcov::GcovData();
      strcpy( gcnoFileName, inputBuffer );

      if ( Verbose )
	fprintf( stderr, "Processing file: %s\n", gcnoFileName );

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
  if (Verbose)
    fprintf( stderr, "Computing uncovered ranges and branches\n" );
  SymbolsToAnalyze->computeUncovered();

  // Calculate remainder of statistics.
  if (Verbose)
    fprintf( stderr, "Calculate statistics\n" );
  SymbolsToAnalyze->calculateStatistics();

  // Look up the source lines for any uncovered ranges and branches.
  if (Verbose)
    fprintf(
      stderr, "Looking up source lines for uncovered ranges and branches\n"
    );
  SymbolsToAnalyze->findSourceForUncovered();

  //
  // Report the coverage data.
  //
  if (Verbose)
    fprintf(
      stderr, "Generate Reports\n"
    );
  Coverage::GenerateReports();

  // Write explanations that were not found.
  if ( explanations ) {
    std::string notFound;

    notFound = outputDirectory;
    notFound += "/";
    notFound += "ExplanationsNotFound.txt";

    if (Verbose)
      fprintf( stderr, "Writing Not Found Report (%s)\n", notFound.c_str() );
    AllExplanations->writeNotFound( notFound.c_str() );
  }

  return 0;
}
