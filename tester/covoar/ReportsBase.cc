#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ReportsBase.h"
#include "app_common.h"
#include "CoverageRanges.h"
#include "DesiredSymbols.h"
#include "Explanations.h"
#include "ObjdumpProcessor.h"

#include "ReportsText.h"
#include "ReportsHtml.h"

#ifdef _WIN32
#include <direct.h>
#endif

namespace Coverage {

ReportsBase::ReportsBase( time_t timestamp, std::string symbolSetName ):
  reportExtension_m(""),
  symbolSetName_m(symbolSetName),
  timestamp_m( timestamp )
{
}

ReportsBase::~ReportsBase()
{
}

FILE* ReportsBase::OpenFile(
  const char* const fileName,
  const char* const symbolSetName
)
{
  int          sc;
  FILE        *aFile;
  std::string  file;

  std::string symbolSetOutputDirectory;
  rld::path::path_join(
    outputDirectory,
    symbolSetName,
    symbolSetOutputDirectory
  );

  // Create the output directory if it does not already exist
#ifdef _WIN32
  sc = _mkdir( symbolSetOutputDirectory );
#else
  sc = mkdir( symbolSetOutputDirectory.c_str(),0755 );
#endif
  if ( (sc == -1) && (errno != EEXIST) ) {
    fprintf(
      stderr,
      "Unable to create output directory %s\n",
      symbolSetOutputDirectory.c_str()
    );
    return NULL;
  }

  file = symbolSetOutputDirectory;
  rld::path::path_join(file, fileName, file);

  // Open the file.
  aFile = fopen( file.c_str(), "w" );
  if ( !aFile ) {
    fprintf( stderr, "Unable to open %s\n", file.c_str() );
  }
  return aFile;
}

void ReportsBase::WriteIndex(
  const char* const fileName
)
{
}

FILE* ReportsBase::OpenAnnotatedFile(
  const char* const fileName
)
{
  return OpenFile(fileName, symbolSetName_m.c_str());
}

FILE* ReportsBase::OpenBranchFile(
  const char* const fileName,
  bool              hasBranches
)
{
  return OpenFile(fileName, symbolSetName_m.c_str());
}

FILE* ReportsBase::OpenCoverageFile(
  const char* const fileName
)
{
  return OpenFile(fileName, symbolSetName_m.c_str());
}

FILE* ReportsBase::OpenNoRangeFile(
  const char* const fileName
)
{
  return OpenFile(fileName, symbolSetName_m.c_str());
}


FILE* ReportsBase::OpenSizeFile(
  const char* const fileName
)
{
  return OpenFile(fileName, symbolSetName_m.c_str());
}

FILE* ReportsBase::OpenSymbolSummaryFile(
  const char* const fileName
)
{
  return OpenFile(fileName, symbolSetName_m.c_str());
}

void ReportsBase::CloseFile(
  FILE*  aFile
)
{
  fclose( aFile );
}

void ReportsBase::CloseAnnotatedFile(
  FILE*  aFile
)
{
  CloseFile( aFile );
}

void ReportsBase::CloseBranchFile(
  FILE*  aFile,
  bool   hasBranches
)
{
  CloseFile( aFile );
}

void  ReportsBase::CloseCoverageFile(
  FILE*  aFile
)
{
  CloseFile( aFile );
}

void  ReportsBase::CloseNoRangeFile(
  FILE*  aFile
)
{
  CloseFile( aFile );
}

void  ReportsBase::CloseSizeFile(
  FILE*  aFile
)
{
  CloseFile( aFile );
}

void  ReportsBase::CloseSymbolSummaryFile(
  FILE*  aFile
)
{
  CloseFile( aFile );
}

std::string expand_tabs(const std::string& in) {
  std::string expanded = "";
  int i = 0;

  for (char c : in) {
    if (c == '\t') {
      int num_tabs = 4 - (i % 4);
      expanded.append(num_tabs, ' ');
      i += num_tabs;
    } else {
      expanded += c;
      i++;
    }
  }

  return expanded;
}

/*
 *  Write annotated report
 */
void ReportsBase::WriteAnnotatedReport(
  const char* const fileName
) {
  FILE*                                                          aFile = NULL;
  Coverage::CoverageRanges*                                      theBranches;
  Coverage::CoverageRanges*                                      theRanges;
  Coverage::CoverageMapBase*                                     theCoverageMap = NULL;
  uint32_t                                                       bAddress = 0;
  AnnotatedLineState_t                                           state;
  const std::list<Coverage::ObjdumpProcessor::objdumpLine_t>*    theInstructions;
  std::list<
    Coverage::ObjdumpProcessor::objdumpLine_t>::const_iterator   itr;

  aFile = OpenAnnotatedFile(fileName);
  if (!aFile)
    return;

  // Process uncovered branches for each symbol.
  const std::vector<std::string>& symbols = SymbolsToAnalyze->getSymbolsForSet(symbolSetName_m);

  for (const auto& symbol : symbols) {
    const SymbolInformation& info = SymbolsToAnalyze->allSymbols().at(symbol);

    // If uncoveredRanges and uncoveredBranches don't exist, then the
    // symbol was never referenced by any executable.  Just skip it.
    if ((info.uncoveredRanges == NULL) &&
        (info.uncoveredBranches == NULL))
      continue;

    // If uncoveredRanges and uncoveredBranches are empty, then everything
    // must have been covered for this symbol.  Just skip it.
    if ((info.uncoveredRanges->set.empty()) &&
        (info.uncoveredBranches->set.empty()))
      continue;

    theCoverageMap = info.unifiedCoverageMap;
    bAddress = info.baseAddress;
    theInstructions = &(info.instructions);
    theRanges = info.uncoveredRanges;
    theBranches = info.uncoveredBranches;

    // Add annotations to each line where necessary
    AnnotatedStart( aFile );
    for (itr = theInstructions->begin();
         itr != theInstructions->end();
         itr++ ) {

      uint32_t           id = 0;
      std::string        annotation = "";
      std::string        line;
      const std::size_t  LINE_LENGTH = 150;
      char               textLine[LINE_LENGTH];

      state = A_SOURCE;

      if ( itr->isInstruction ) {
        if (!theCoverageMap->wasExecuted( itr->address - bAddress )){
          annotation = "<== NOT EXECUTED";
          state = A_NEVER_EXECUTED;
          id = theRanges->getId( itr->address );
        } else if (theCoverageMap->isBranch( itr->address - bAddress )) {
          id = theBranches->getId( itr->address );
          if (theCoverageMap->wasAlwaysTaken( itr->address - bAddress )){
            annotation = "<== ALWAYS TAKEN";
            state = A_BRANCH_TAKEN;
          } else if (theCoverageMap->wasNeverTaken( itr->address - bAddress )){
            annotation = "<== NEVER TAKEN";
            state = A_BRANCH_NOT_TAKEN;
          }
        } else {
          state = A_EXECUTED;
        }
      }

      std::string textLineWithoutTabs = expand_tabs(itr->line);
      snprintf( textLine, LINE_LENGTH, "%-90s", textLineWithoutTabs.c_str() );
      line = textLine + annotation;

      PutAnnotatedLine( aFile, state, line, id);
    }

    AnnotatedEnd( aFile );
  }

  CloseAnnotatedFile( aFile );
}

/*
 *  Write branch report
 */
void ReportsBase::WriteBranchReport(
  const char* const fileName
) {
  FILE*                                           report = NULL;
  Coverage::CoverageRanges::ranges_t::iterator    ritr;
  Coverage::CoverageRanges*                       theBranches;
  unsigned int                                    count;
  bool                                            hasBranches = true;

  if ((SymbolsToAnalyze->getNumberBranchesFound(symbolSetName_m) == 0) ||
      (BranchInfoAvailable == false) )
     hasBranches = false;

  // Open the branch report file
  report = OpenBranchFile( fileName, hasBranches );
  if (!report)
    return;

  // If no branches were found then branch coverage is not supported
  if ((SymbolsToAnalyze->getNumberBranchesFound(symbolSetName_m) != 0) &&
      (BranchInfoAvailable == true) ) {
    // Process uncovered branches for each symbol in the set.
    const std::vector<std::string>& symbols = SymbolsToAnalyze->getSymbolsForSet(symbolSetName_m);

    count = 0;
    for (const auto& symbol : symbols) {
      const SymbolInformation& info = SymbolsToAnalyze->allSymbols().at(symbol);

      theBranches = info.uncoveredBranches;

      if (theBranches && !theBranches->set.empty()) {

        for (ritr =  theBranches->set.begin() ;
             ritr != theBranches->set.end() ;
             ritr++ ) {
          count++;
          PutBranchEntry( report, count, symbol, info, ritr );
        }
      }
    }
  }

  CloseBranchFile( report, hasBranches );
}

/*
 *  Write coverage report
 */
void ReportsBase::WriteCoverageReport(
  const char* const fileName
)
{
  FILE*                                           report;
  Coverage::CoverageRanges::ranges_t::iterator    ritr;
  Coverage::CoverageRanges*                       theRanges;
  unsigned int                                    count;
  FILE*                                           NoRangeFile;
  std::string                                     NoRangeName;

  // Open special file that captures NoRange informaiton
  NoRangeName = "no_range_";
  NoRangeName +=  fileName;
  NoRangeFile = OpenNoRangeFile ( NoRangeName.c_str() );
  if (!NoRangeFile) {
    return;
  }

  // Open the coverage report file.
  report = OpenCoverageFile( fileName );
  if ( !report ) {
    return;
  }

  // Process uncovered ranges for each symbol.
  const std::vector<std::string>& symbols = SymbolsToAnalyze->getSymbolsForSet(symbolSetName_m);

  count = 0;
  for (const auto& symbol : symbols) {
    const SymbolInformation& info = SymbolsToAnalyze->allSymbols().at(symbol);

    theRanges = info.uncoveredRanges;

    // If uncoveredRanges doesn't exist, then the symbol was never
    // referenced by any executable.  There may be a problem with the
    // desired symbols list or with the executables so put something
    // in the report.
    if (theRanges == NULL) {
      putCoverageNoRange( report, NoRangeFile, count, symbol );
      count++;
    }  else if (!theRanges->set.empty()) {

      for (ritr =  theRanges->set.begin() ;
           ritr != theRanges->set.end() ;
           ritr++ ) {
        PutCoverageLine( report, count, symbol, info, ritr );
        count++;
      }
    }
  }

  CloseNoRangeFile( NoRangeFile );
  CloseCoverageFile( report );

}

/*
 * Write size report
 */
void ReportsBase::WriteSizeReport(
  const char* const fileName
)
{
  FILE*                                           report;
  Coverage::CoverageRanges::ranges_t::iterator    ritr;
  Coverage::CoverageRanges*                       theRanges;
  unsigned int                                    count;

  // Open the report file.
  report = OpenSizeFile( fileName );
  if ( !report ) {
    return;
  }

  // Process uncovered ranges for each symbol.
  const std::vector<std::string>& symbols = SymbolsToAnalyze->getSymbolsForSet(symbolSetName_m);

  count = 0;
  for (const auto& symbol : symbols) {
    const SymbolInformation& info = SymbolsToAnalyze->allSymbols().at(symbol);

    theRanges = info.uncoveredRanges;

    if (theRanges && !theRanges->set.empty()) {

      for (ritr =  theRanges->set.begin() ;
           ritr != theRanges->set.end() ;
           ritr++ ) {
        PutSizeLine( report, count, symbol, ritr );
        count++;
      }
    }
  }

  CloseSizeFile( report );
}

void ReportsBase::WriteSymbolSummaryReport(
  const char* const fileName
)
{
  FILE*                                           report;
  unsigned int                                    count;

  // Open the report file.
  report = OpenSymbolSummaryFile( fileName );
  if ( !report ) {
    return;
  }

  // Process each symbol.
  const std::vector<std::string>& symbols = SymbolsToAnalyze->getSymbolsForSet(symbolSetName_m);

  count = 0;
  for (const auto& symbol : symbols) {
    const SymbolInformation& info = SymbolsToAnalyze->allSymbols().at(symbol);

    PutSymbolSummaryLine( report, count, symbol, info );
    count++;
  }

  CloseSymbolSummaryFile( report );
}

void  ReportsBase::WriteSummaryReport(
  const char* const fileName,
  const char* const symbolSetName
)
{
    // Calculate coverage statistics and output results.
  uint32_t                                        a;
  uint32_t                                        endAddress;
  uint32_t                                        notExecuted = 0;
  double                                          percentage;
  double                                          percentageBranches;
  Coverage::CoverageMapBase*                      theCoverageMap;
  uint32_t                                        totalBytes = 0;
  FILE*                                           report;

  // Open the report file.
  report = OpenFile( fileName, symbolSetName );
  if ( !report ) {
    return;
  }

  // Look at each symbol.
  const std::vector<std::string>& symbols = SymbolsToAnalyze->getSymbolsForSet(symbolSetName);

  for (const auto& symbol : symbols) {
    SymbolInformation info = SymbolsToAnalyze->allSymbols().at(symbol);

    // If the symbol's unified coverage map exists, scan through it
    // and count bytes.
    theCoverageMap = info.unifiedCoverageMap;
    if (theCoverageMap) {

      endAddress = info.stats.sizeInBytes - 1;

      for (a = 0; a <= endAddress; a++) {
        totalBytes++;
        if (!theCoverageMap->wasExecuted( a ))
          notExecuted++;
      }
    }
  }

  percentage = (double) notExecuted;
  percentage /= (double) totalBytes;
  percentage *= 100.0;

  percentageBranches = (double) (
    SymbolsToAnalyze->getNumberBranchesAlwaysTaken(symbolSetName) +
      SymbolsToAnalyze->getNumberBranchesNeverTaken(symbolSetName) +
      (SymbolsToAnalyze->getNumberBranchesNotExecuted(symbolSetName) * 2)
  );
  percentageBranches /=
    (double) SymbolsToAnalyze->getNumberBranchesFound(symbolSetName) * 2;
  percentageBranches *= 100.0;

  fprintf( report, "Bytes Analyzed                   : %d\n", totalBytes );
  fprintf( report, "Bytes Not Executed               : %d\n", notExecuted );
  fprintf( report, "Percentage Executed              : %5.4g\n", 100.0 - percentage  );
  fprintf( report, "Percentage Not Executed          : %5.4g\n", percentage  );
  fprintf(
    report,
    "Unreferenced Symbols             : %d\n",
    SymbolsToAnalyze->getNumberUnreferencedSymbols(symbolSetName)
  );
  fprintf(
    report,
    "Uncovered ranges found           : %d\n\n",
    SymbolsToAnalyze->getNumberUncoveredRanges(symbolSetName)
  );
  if ((SymbolsToAnalyze->getNumberBranchesFound(symbolSetName) == 0) ||
      (BranchInfoAvailable == false) ) {
    fprintf( report, "No branch information available\n" );
  } else {
    fprintf(
      report,
      "Total conditional branches found : %d\n",
      SymbolsToAnalyze->getNumberBranchesFound(symbolSetName)
    );
    fprintf(
      report,
      "Total branch paths found         : %d\n",
      SymbolsToAnalyze->getNumberBranchesFound(symbolSetName) * 2
    );
    fprintf(
      report,
      "Uncovered branch paths found     : %d\n",
      SymbolsToAnalyze->getNumberBranchesAlwaysTaken(symbolSetName) +
       SymbolsToAnalyze->getNumberBranchesNeverTaken(symbolSetName) +
       (SymbolsToAnalyze->getNumberBranchesNotExecuted(symbolSetName) * 2)
    );
    fprintf(
      report,
      "   %d branches always taken\n",
      SymbolsToAnalyze->getNumberBranchesAlwaysTaken(symbolSetName)
    );
    fprintf(
      report,
      "   %d branches never taken\n",
      SymbolsToAnalyze->getNumberBranchesNeverTaken(symbolSetName)
    );
    fprintf(
      report,
      "   %d branch paths not executed\n",
      SymbolsToAnalyze->getNumberBranchesNotExecuted(symbolSetName) * 2
    );
    fprintf(
      report,
      "Percentage branch paths covered  : %4.4g\n",
      100.0 - percentageBranches
    );
  }
}

void GenerateReports(const std::string& symbolSetName)
{
  typedef std::list<ReportsBase *> reportList_t;

  reportList_t           reportList;
  reportList_t::iterator ritr;
  std::string            reportName;
  ReportsBase*           reports;

  time_t timestamp;


  timestamp = time(NULL); /* get current cal time */
  reports = new ReportsText(timestamp, symbolSetName);
  reportList.push_back(reports);
  reports = new ReportsHtml(timestamp, symbolSetName);
  reportList.push_back(reports);

  for (ritr = reportList.begin(); ritr != reportList.end(); ritr++ ) {
    reports = *ritr;

    reportName = "index" + reports->ReportExtension();
    if (Verbose)
      fprintf(
        stderr, "Generate %s\n", reportName.c_str()
      );
    reports->WriteIndex( reportName.c_str() );

    reportName = "annotated" + reports->ReportExtension();
    if (Verbose)
      fprintf(
        stderr, "Generate %s\n", reportName.c_str()
      );
    reports->WriteAnnotatedReport( reportName.c_str() );

    reportName = "branch" + reports->ReportExtension();
    if (Verbose)
      fprintf(
        stderr, "Generate %s\n", reportName.c_str()
      );
    reports->WriteBranchReport(reportName.c_str() );

    reportName = "uncovered" + reports->ReportExtension();
    if (Verbose)
      fprintf(
        stderr, "Generate %s\n", reportName.c_str()
      );
    reports->WriteCoverageReport(reportName.c_str() );

    reportName = "sizes" + reports->ReportExtension();
    if (Verbose)
      fprintf(
        stderr, "Generate %s\n", reportName.c_str()
      );
    reports->WriteSizeReport(reportName.c_str() );

    reportName = "symbolSummary" + reports->ReportExtension();
    if (Verbose)
      fprintf(
        stderr, "Generate %s\n", reportName.c_str()
      );
    reports->WriteSymbolSummaryReport(reportName.c_str() );
  }

  for (ritr = reportList.begin(); ritr != reportList.end(); ritr++ ) {
    reports = *ritr;
    delete reports;
  }

  ReportsBase::WriteSummaryReport( "summary.txt", symbolSetName.c_str() );
}

}
