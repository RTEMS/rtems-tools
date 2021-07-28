#include <stdio.h>
#include <string.h>

#include <iomanip>

#include "ReportsText.h"
#include "app_common.h"
#include "CoverageRanges.h"
#include "DesiredSymbols.h"
#include "Explanations.h"
#include "ObjdumpProcessor.h"

#include <rtems-utils.h>

typedef rtems::utils::ostream_guard ostream_guard;

namespace Coverage {

ReportsText::ReportsText(
  time_t                  timestamp,
  const std::string&      symbolSetName,
  Coverage::Explanations& allExplanations,
  const std::string&      projectName,
  const std::string&      outputDirectory,
  const DesiredSymbols&   symbolsToAnalyze
): ReportsBase(
     timestamp,
     symbolSetName,
     allExplanations,
     projectName,
     outputDirectory,
     symbolsToAnalyze
   )
{
  reportExtension_m = ".txt";
}

ReportsText::~ReportsText()
{
}

void ReportsText::AnnotatedStart( std::ofstream& aFile )
{
  aFile << "========================================"
        << "=======================================" << std::endl;
}

void ReportsText::AnnotatedEnd( std::ofstream& aFile )
{
}

void ReportsText::PutAnnotatedLine(
  std::ofstream&       aFile,
  AnnotatedLineState_t state,
  const std::string&   line,
  uint32_t             id
)
{
  aFile << line << std::endl;
}

bool ReportsText::PutNoBranchInfo( std::ofstream& report )
{
  if (
    BranchInfoAvailable &&
    symbolsToAnalyze_m.getNumberBranchesFound( symbolSetName_m ) != 0
  ) {
    report << "All branch paths taken." << std::endl;
  } else {
    report << "No branch information found." << std::endl;
  }

  return true;
}


bool ReportsText::PutBranchEntry(
  std::ofstream&                         report,
  unsigned int                           number,
  const std::string&                     symbolName,
  const SymbolInformation&               symbolInfo,
  const CoverageRanges::coverageRange_t& range
)
{
  const Coverage::Explanation* explanation;

  // Add an entry to the report
  report << "============================================" << std::endl
         << "Symbol        : " << symbolName
         << std::hex << " (0x" << symbolInfo.baseAddress << ")" << std::endl
         << "Line          : " << range.lowSourceLine
         << " (0x" << range.lowAddress << ")" << std::endl
         << "Size in Bytes : " << range.highAddress - range.lowAddress + 1
         << std::dec << std::endl;

  if (
    range.reason ==
    Coverage::CoverageRanges::UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN
  ) {
    report << "Reason        : ALWAYS TAKEN"
           << std::endl << std::endl;
  } else if (
    range.reason ==
    Coverage::CoverageRanges::UNCOVERED_REASON_BRANCH_NEVER_TAKEN
  ) {
    report << "Reason        : NEVER TAKEN"
           << std::endl << std::endl;
  }

  // See if an explanation is available
  explanation = allExplanations_m.lookupExplanation( range.lowSourceLine );

  if ( !explanation ) {
    report << "Classification: NONE" << std::endl << std::endl
           << "Explanation:" << std::endl
           << "No Explanation" << std::endl;
  } else {
    report << "Classification: " << explanation->classification
           << std::endl << std::endl
           << "Explanation:" << std::endl;

    for ( unsigned int i=0; i < explanation->explanation.size(); i++ ) {
      report << explanation->explanation[i] << std::endl;
    }
  }

  report << "============================================" << std::endl;

  return true;
}

void ReportsText::putCoverageNoRange(
  std::ofstream&     report,
  std::ofstream&     noRangeFile,
  unsigned int       number,
  const std::string& symbol
)
{
  report << "============================================" << std::endl
         << "Symbol        : " << symbol << std::endl << std::endl
         << "          *** NEVER REFERENCED ***" << std::endl << std::endl
         << "This symbol was never referenced by an analyzed executable."
         << std::endl
         << "Therefore there is no size or disassembly for this symbol."
         << std::endl
         << "This could be due to symbol misspelling or lack of a test for"
         << std::endl
         << "this symbol." << std::endl
         << "============================================" << std::endl;

  noRangeFile << symbol << std::endl;
}

bool ReportsText::PutCoverageLine(
  std::ofstream&                         report,
  unsigned int                           number,
  const std::string&                     symbolName,
  const SymbolInformation&               symbolInfo,
  const CoverageRanges::coverageRange_t& range
)
{
  const Coverage::Explanation* explanation;

  ostream_guard oldState( report );

  report << "============================================" << std::endl
         << "Index                : " << range.id << std::endl
         << "Symbol               : " << symbolName
         << std::hex << " (0x" << symbolInfo.baseAddress << ")" << std::endl
         << "Starting Line        : " << range.lowSourceLine
         << " (0x" << range.lowAddress << ")" << std::endl
         << "Ending Line          : " << range.highSourceLine
         << " (0x" << range.highAddress << ")" << std::endl
         << std::dec
         << "Size in Bytes        : "
         << range.highAddress - range.lowAddress + 1 << std::endl
         << "Size in Instructions : " << range.instructionCount
         << std::endl << std::endl;

  explanation = allExplanations_m.lookupExplanation( range.lowSourceLine );

  if ( !explanation ) {
    report << "Classification: NONE" << std::endl << std::endl
           << "Explanation:" << std::endl
           << "No Explanation" << std::endl;
  } else {
    report << "Classification: " << explanation->classification << std::endl
           << std::endl
           << "Explanation:" << std::endl;

    for ( unsigned int i=0; i < explanation->explanation.size(); i++) {
      report << explanation->explanation[i] << std::endl;
    }
  }

  report << "============================================" << std::endl;

  return true;
}

bool  ReportsText::PutSizeLine(
  std::ofstream&                         report,
  unsigned int                           number,
  const std::string&                     symbolName,
  const CoverageRanges::coverageRange_t& range
)
{
  report << range.highAddress - range.lowAddress + 1 << '\t'
         << symbolName << '\t'
         << range.lowSourceLine << std::endl;

  return true;
}

bool  ReportsText::PutSymbolSummaryLine(
  std::ofstream&           report,
  unsigned int             number,
  const std::string&       symbolName,
  const SymbolInformation& symbolInfo
)
{
  float uncoveredBytes;
  float uncoveredInstructions;

  if ( symbolInfo.stats.sizeInBytes == 0 ) {
    report << "============================================" << std::endl
           << "Symbol                            : " << symbolName << std::endl
           << "          *** NEVER REFERENCED ***"
           << std::endl << std::endl
           << "This symbol was never referenced by an analyzed executable."
           << std::endl
           << "Therefore there is no size or disassembly for this symbol."
           << std::endl
           << "This could be due to symbol misspelling or lack of a test for"
           << std::endl
           << "this symbol." << std::endl
           << "============================================" << std::endl;
  } else {
    if ( symbolInfo.stats.sizeInInstructions == 0 ) {
      uncoveredInstructions = 0;
    } else {
      uncoveredInstructions =
        ( symbolInfo.stats.uncoveredInstructions * 100.0 ) /
        symbolInfo.stats.sizeInInstructions;
    }

    if ( symbolInfo.stats.sizeInBytes == 0 ) {
      uncoveredBytes = 0;
    } else {
      uncoveredBytes = ( symbolInfo.stats.uncoveredBytes * 100.0 ) /
                       symbolInfo.stats.sizeInBytes;
    }

    report << "============================================" << std::endl
           << "Symbol                            : "
           << symbolName << std::endl
           << "Total Size in Bytes               : "
           << symbolInfo.stats.sizeInBytes << std::endl
           << "Total Size in Instructions        : "
           << symbolInfo.stats.sizeInInstructions << std::endl
           << "Total number Branches             : "
           << symbolInfo.stats.branchesNotExecuted +
              symbolInfo.stats.branchesExecuted
           << std::endl
           << "Total Always Taken                : "
           << symbolInfo.stats.branchesAlwaysTaken << std::endl
           << "Total Never Taken                 : "
           << symbolInfo.stats.branchesNeverTaken << std::endl
           << std::fixed << std::setprecision( 2 )
           << "Percentage Uncovered Instructions : "
           << uncoveredInstructions << std::endl
           << "Percentage Uncovered Bytes        : "
           << uncoveredBytes << std::endl;

  report << "============================================" << std::endl;
  }

  return true;
}

}
