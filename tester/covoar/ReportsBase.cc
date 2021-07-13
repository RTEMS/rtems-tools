#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iomanip>
#include <sstream>

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

ReportsBase::ReportsBase( time_t timestamp, const std::string& symbolSetName ):
  reportExtension_m( "" ),
  symbolSetName_m( symbolSetName ),
  timestamp_m( timestamp )
{
}

ReportsBase::~ReportsBase()
{
}

void ReportsBase::OpenFile(
  const std::string& fileName,
  const std::string& symbolSetName,
  std::ofstream&     aFile
)
{
  int         sc;
  std::string file;

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
  sc = mkdir( symbolSetOutputDirectory.c_str(), 0755 );
#endif
  if ( ( sc == -1 ) && ( errno != EEXIST ) ) {
    throw rld::error(
      "Unable to create output directory",
      "ReportsBase::OpenFile"
    );
    return;
  }

  file = symbolSetOutputDirectory;
  rld::path::path_join( file, fileName, file );

  // Open the file.
  aFile.open( file );
  if ( !aFile.is_open() ) {
    std::cerr << "Unable to open " << file << std::endl;
  }
}

void ReportsBase::WriteIndex( const std::string& fileName )
{
}

void ReportsBase::OpenAnnotatedFile(
  const std::string& fileName,
  std::ofstream&     aFile
)
{
  OpenFile(fileName, symbolSetName_m, aFile);
}

void ReportsBase::OpenBranchFile(
  const std::string& fileName,
  bool               hasBranches,
  std::ofstream&     aFile
)
{
  OpenFile(fileName, symbolSetName_m, aFile);
}

void ReportsBase::OpenCoverageFile(
  const std::string& fileName,
  std::ofstream&     aFile
)
{
  OpenFile(fileName, symbolSetName_m, aFile);
}

void ReportsBase::OpenNoRangeFile(
  const std::string& fileName,
  std::ofstream&     aFile
)
{
  OpenFile(fileName, symbolSetName_m, aFile);
}


void ReportsBase::OpenSizeFile(
  const std::string& fileName,
  std::ofstream&     aFile
)
{
  OpenFile(fileName, symbolSetName_m, aFile);
}

void ReportsBase::OpenSymbolSummaryFile(
  const std::string& fileName,
  std::ofstream&     aFile
)
{
  OpenFile(fileName, symbolSetName_m, aFile);
}

void ReportsBase::CloseFile( std::ofstream& aFile )
{
  aFile.close();
}

void ReportsBase::CloseAnnotatedFile( std::ofstream& aFile )
{
  CloseFile( aFile );
}

void ReportsBase::CloseBranchFile( std::ofstream& aFile, bool hasBranches )
{
  CloseFile( aFile );
}

void  ReportsBase::CloseCoverageFile( std::ofstream& aFile )
{
  CloseFile( aFile );
}

void  ReportsBase::CloseNoRangeFile( std::ofstream& aFile )
{
  CloseFile( aFile );
}

void  ReportsBase::CloseSizeFile( std::ofstream& aFile )
{
  CloseFile( aFile );
}

void  ReportsBase::CloseSymbolSummaryFile( std::ofstream& aFile )
{
  CloseFile( aFile );
}

std::string expand_tabs( const std::string& in ) {
  std::string expanded = "";
  int i = 0;

  for ( char c : in ) {
    if ( c == '\t' ) {
      int num_tabs = 4 - ( i % 4 );
      expanded.append( num_tabs, ' ' );
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
void ReportsBase::WriteAnnotatedReport( const std::string& fileName )
{
  std::ofstream              aFile;
  Coverage::CoverageRanges*  theBranches;
  Coverage::CoverageRanges*  theRanges;
  Coverage::CoverageMapBase* theCoverageMap = NULL;
  uint32_t                   bAddress = 0;
  AnnotatedLineState_t       state;

  OpenAnnotatedFile( fileName, aFile );
  if ( !aFile.is_open() ) {
    throw rld::error(
      "Unable to open " + fileName,
      "ReportsBase::WriteAnnotatedReport"
    );
    return;
  }

  // Process uncovered branches for each symbol.
  const std::vector<std::string>& symbols =
    SymbolsToAnalyze->getSymbolsForSet( symbolSetName_m );

  for ( const auto& symbol : symbols ) {
    const SymbolInformation& info =
      SymbolsToAnalyze->allSymbols().at( symbol );

    // If uncoveredRanges and uncoveredBranches don't exist, then the
    // symbol was never referenced by any executable.  Just skip it.
    if (
      ( info.uncoveredRanges == NULL ) &&
      ( info.uncoveredBranches == NULL )
    ) {
      continue;
    }

    // If uncoveredRanges and uncoveredBranches are empty, then everything
    // must have been covered for this symbol.  Just skip it.
    if (
      ( info.uncoveredRanges->set.empty() ) &&
      ( info.uncoveredBranches->set.empty() )
    ) {
      continue;
    }

    theCoverageMap = info.unifiedCoverageMap;
    bAddress       = info.baseAddress;
    theRanges      = info.uncoveredRanges;
    theBranches    = info.uncoveredBranches;

    // Add annotations to each line where necessary
    AnnotatedStart( aFile );
    for ( const auto& instruction : info.instructions ) {
      uint32_t          id = 0;
      std::string       annotation = "";
      std::string       line;
      const std::size_t LINE_LENGTH = 150;
      std::string       textLine = "";
      std::stringstream ss;

      state = A_SOURCE;

      if ( instruction.isInstruction ) {
        if ( !theCoverageMap->wasExecuted( instruction.address - bAddress ) ) {
          annotation = "<== NOT EXECUTED";
          state = A_NEVER_EXECUTED;
          id = theRanges->getId( instruction.address );
        } else if (
          theCoverageMap->isBranch( instruction.address - bAddress )
        ) {
          id = theBranches->getId( instruction.address );
          if (
          theCoverageMap->wasAlwaysTaken( instruction.address - bAddress )
        ){
            annotation = "<== ALWAYS TAKEN";
            state = A_BRANCH_TAKEN;
          } else if (
            theCoverageMap->wasNeverTaken( instruction.address - bAddress )
          ) {
            annotation = "<== NEVER TAKEN";
            state = A_BRANCH_NOT_TAKEN;
          }
        } else {
          state = A_EXECUTED;
        }
      }

      std::string textLineWithoutTabs = expand_tabs( instruction.line );

      ss << std::left << std::setw( 90 )
         << textLineWithoutTabs.c_str();

      textLine = ss.str().substr( 0, LINE_LENGTH );

      line = textLine + annotation;

      PutAnnotatedLine( aFile, state, line, id );
    }

    AnnotatedEnd( aFile );
  }

  CloseAnnotatedFile( aFile );
}

/*
 *  Write branch report
 */
void ReportsBase::WriteBranchReport( const std::string& fileName )
{
  std::ofstream             report;
  Coverage::CoverageRanges* theBranches;
  unsigned int              count;
  bool                      hasBranches = true;

  if (
    ( SymbolsToAnalyze->getNumberBranchesFound( symbolSetName_m ) == 0 ) ||
    ( BranchInfoAvailable == false )
  ) {
     hasBranches = false;
  }

  // Open the branch report file
  OpenBranchFile( fileName, hasBranches, report );
  if ( !report.is_open() ) {
    return;
  }

  // If no branches were found then branch coverage is not supported
  if (
    ( SymbolsToAnalyze->getNumberBranchesFound( symbolSetName_m ) != 0 ) &&
    ( BranchInfoAvailable == true )
  ) {
    // Process uncovered branches for each symbol in the set.
    const std::vector<std::string>& symbols =
      SymbolsToAnalyze->getSymbolsForSet( symbolSetName_m );

    count = 0;
    for ( const auto& symbol : symbols ) {
      const SymbolInformation& info =
        SymbolsToAnalyze->allSymbols().at( symbol );

      theBranches = info.uncoveredBranches;

      if ( theBranches && !theBranches->set.empty() ) {
        for ( const auto& range : theBranches->set ) {
          count++;
          PutBranchEntry( report, count, symbol, info, range );
        }
      }
    }
  }

  CloseBranchFile( report, hasBranches );
}

/*
 *  Write coverage report
 */
void ReportsBase::WriteCoverageReport( const std::string& fileName )
{
  std::ofstream             report;
  Coverage::CoverageRanges* theRanges;
  unsigned int              count;
  std::ofstream             NoRangeFile;
  std::string               NoRangeName;

  // Open special file that captures NoRange informaiton
  NoRangeName = "no_range_";
  NoRangeName += fileName;
  OpenNoRangeFile( NoRangeName, NoRangeFile );
  if ( !NoRangeFile.is_open() ) {
    return;
  }

  // Open the coverage report file.
  OpenCoverageFile( fileName, report );
  if ( !report.is_open() ) {
    return;
  }

  // Process uncovered ranges for each symbol.
  const std::vector<std::string>& symbols =
    SymbolsToAnalyze->getSymbolsForSet( symbolSetName_m );

  count = 0;
  for ( const auto& symbol : symbols ) {
    const SymbolInformation& info =
      SymbolsToAnalyze->allSymbols().at( symbol );

    theRanges = info.uncoveredRanges;

    // If uncoveredRanges doesn't exist, then the symbol was never
    // referenced by any executable.  There may be a problem with the
    // desired symbols list or with the executables so put something
    // in the report.
    if ( theRanges == NULL ) {
      putCoverageNoRange( report, NoRangeFile, count, symbol );
      count++;
    }  else if ( !theRanges->set.empty() ) {
      for ( const auto& range : theRanges->set ) {
        PutCoverageLine( report, count, symbol, info, range );
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
void ReportsBase::WriteSizeReport( const std::string& fileName )
{
  std::ofstream             report;
  Coverage::CoverageRanges* theRanges;
  unsigned int              count;

  // Open the report file.
  OpenSizeFile( fileName, report );
  if ( !report.is_open() ) {
    return;
  }

  // Process uncovered ranges for each symbol.
  const std::vector<std::string>& symbols =
    SymbolsToAnalyze->getSymbolsForSet( symbolSetName_m );

  count = 0;
  for ( const auto& symbol : symbols ) {
    const SymbolInformation& info =
      SymbolsToAnalyze->allSymbols().at( symbol );

    theRanges = info.uncoveredRanges;

    if ( theRanges && !theRanges->set.empty() ) {
      for ( const auto& range : theRanges->set ) {
        PutSizeLine( report, count, symbol, range );
        count++;
      }
    }
  }

  CloseSizeFile( report );
}

void ReportsBase::WriteSymbolSummaryReport( const std::string& fileName )
{
  std::ofstream report;
  unsigned int  count;

  // Open the report file.
  OpenSymbolSummaryFile( fileName , report );
  if ( !report.is_open() ) {
    return;
  }

  // Process each symbol.
  const std::vector<std::string>& symbols =
    SymbolsToAnalyze->getSymbolsForSet( symbolSetName_m );

  count = 0;
  for ( const auto& symbol : symbols ) {
    const SymbolInformation& info =
      SymbolsToAnalyze->allSymbols().at( symbol );

    PutSymbolSummaryLine( report, count, symbol, info );
    count++;
  }

  CloseSymbolSummaryFile( report );
}

void  ReportsBase::WriteSummaryReport(
  const std::string& fileName,
  const std::string& symbolSetName
)
{
    // Calculate coverage statistics and output results.
  uint32_t                   a;
  uint32_t                   endAddress;
  uint32_t                   notExecuted = 0;
  double                     percentage;
  double                     percentageBranches;
  Coverage::CoverageMapBase* theCoverageMap;
  uint32_t                   totalBytes = 0;
  std::ofstream              report;

  // Open the report file.
  OpenFile( fileName, symbolSetName, report );
  if ( !report.is_open() ) {
    return;
  }

  // Look at each symbol.
  const std::vector<std::string>& symbols =
    SymbolsToAnalyze->getSymbolsForSet( symbolSetName );

  for ( const auto& symbol : symbols ) {
    SymbolInformation info = SymbolsToAnalyze->allSymbols().at( symbol );

    // If the symbol's unified coverage map exists, scan through it
    // and count bytes.
    theCoverageMap = info.unifiedCoverageMap;
    if ( theCoverageMap ) {

      endAddress = info.stats.sizeInBytes - 1;

      for ( a = 0; a <= endAddress; a++ ) {
        totalBytes++;
        if ( !theCoverageMap->wasExecuted( a ) )
          notExecuted++;
      }
    }
  }

  if ( totalBytes == 0 ) {
    percentage = 0;
  } else {
    percentage = 100.0 * (double) notExecuted / totalBytes;
  }

  percentageBranches = (double) (
    SymbolsToAnalyze->getNumberBranchesAlwaysTaken( symbolSetName ) +
    SymbolsToAnalyze->getNumberBranchesNeverTaken( symbolSetName ) +
  ( SymbolsToAnalyze->getNumberBranchesNotExecuted( symbolSetName ) * 2 )
  );
  percentageBranches /=
    (double) SymbolsToAnalyze->getNumberBranchesFound( symbolSetName ) * 2;
  percentageBranches *= 100.0;

  report << "Bytes Analyzed                   : " << totalBytes << std::endl
         << "Bytes Not Executed               : " << notExecuted << std::endl
         << "Percentage Executed              : "
         << std::fixed << std::setprecision( 2 ) << std::setw( 5 )
         << 100.0 - percentage << std::endl
         << "Percentage Not Executed          : " << percentage << std::endl
         << "Unreferenced Symbols             : "
         << SymbolsToAnalyze->getNumberUnreferencedSymbols( symbolSetName )
         << std::endl << "Uncovered ranges found           : "
         << SymbolsToAnalyze->getNumberUncoveredRanges( symbolSetName )
         << std::endl << std::endl;

  if (
    ( SymbolsToAnalyze->getNumberBranchesFound( symbolSetName ) == 0 ) ||
    ( BranchInfoAvailable == false )
  ) {
    report << "No branch information available" << std::endl;
  } else {
    report << "Total conditional branches found : "
           << SymbolsToAnalyze->getNumberBranchesFound( symbolSetName )
           << std::endl << "Total branch paths found         : "
           << SymbolsToAnalyze->getNumberBranchesFound( symbolSetName ) * 2
           << std::endl << "Uncovered branch paths found     : "
           << SymbolsToAnalyze->getNumberBranchesAlwaysTaken( symbolSetName ) +
              SymbolsToAnalyze->getNumberBranchesNeverTaken( symbolSetName ) +
            ( SymbolsToAnalyze->getNumberBranchesNotExecuted( symbolSetName ) * 2 )
           << std::endl << "   "
           << SymbolsToAnalyze->getNumberBranchesAlwaysTaken( symbolSetName )
           << " branches always taken" << std::endl << "   "
           << SymbolsToAnalyze->getNumberBranchesNeverTaken( symbolSetName )
           << " branches never taken" << std::endl << "   "
           << SymbolsToAnalyze->getNumberBranchesNotExecuted( symbolSetName ) * 2
           << " branch paths not executed" << std::endl
           << "Percentage branch paths covered  : "
           << std::fixed << std::setprecision( 2 ) << std::setw( 4 )
           << 100.0 - percentageBranches << std::endl;

  }

  CloseFile( report );
}

void GenerateReports( const std::string& symbolSetName )
{
  typedef std::list<ReportsBase *> reportList_t;

  reportList_t           reportList;
  reportList_t::iterator ritr;
  std::string            reportName;
  ReportsBase*           reports;
  time_t                 timestamp;


  timestamp = time( NULL ); /* get current cal time */
  reports = new ReportsText( timestamp, symbolSetName );
  reportList.push_back( reports );
  reports = new ReportsHtml( timestamp, symbolSetName );
  reportList.push_back( reports );

  for ( ritr = reportList.begin(); ritr != reportList.end(); ritr++ ) {
    reports = *ritr;

    reportName = "index" + reports->ReportExtension();
    if ( Verbose ) {
      std::cerr << "Generate " << reportName << std::endl;
    }
    reports->WriteIndex( reportName );

    reportName = "annotated" + reports->ReportExtension();
    if ( Verbose ) {
      std::cerr << "Generate " << reportName << std::endl;
    }
    reports->WriteAnnotatedReport( reportName );

    reportName = "branch" + reports->ReportExtension();
    if ( Verbose ) {
      std::cerr << "Generate " << reportName << std::endl;
    }
    reports->WriteBranchReport( reportName );

    reportName = "uncovered" + reports->ReportExtension();
    if ( Verbose ) {
      std::cerr << "Generate " << reportName << std::endl;
    }
    reports->WriteCoverageReport( reportName );

    reportName = "sizes" + reports->ReportExtension();
    if ( Verbose ) {
      std::cerr << "Generate " << reportName << std::endl;
    }
    reports->WriteSizeReport( reportName );

    reportName = "symbolSummary" + reports->ReportExtension();
    if ( Verbose ) {
      std::cerr << "Generate " << reportName << std::endl;
    }
    reports->WriteSymbolSummaryReport( reportName );
  }

  for ( ritr = reportList.begin(); ritr != reportList.end(); ritr++ ) {
    reports = *ritr;
    delete reports;
  }

  ReportsBase::WriteSummaryReport( "summary.txt", symbolSetName );
}

}
