#include <stdio.h>
#include <string.h>

#include "ReportsText.h"
#include "app_common.h"
#include "CoverageRanges.h"
#include "DesiredSymbols.h"
#include "Explanations.h"
#include "ObjdumpProcessor.h"


namespace Coverage {

ReportsText::ReportsText( time_t timestamp, std::string symbolSetName ):
  ReportsBase( timestamp, symbolSetName )
{
  reportExtension_m = ".txt";
}

ReportsText::~ReportsText()
{
}

void ReportsText::AnnotatedStart(
  FILE*                aFile
)
{
  fprintf(
    aFile,
    "========================================"
    "=======================================\n"
  );
}

void ReportsText::AnnotatedEnd(
  FILE*                aFile
)
{
}

void ReportsText::PutAnnotatedLine(
  FILE*                aFile,
  AnnotatedLineState_t state,
  std::string          line,
  uint32_t             id
)
{
  fprintf( aFile, "%s\n", line.c_str());
}

bool ReportsText::PutNoBranchInfo(
  FILE*           report
)
{
  if ( BranchInfoAvailable &&
    SymbolsToAnalyze->getNumberBranchesFound(symbolSetName_m) != 0 )
    fprintf( report, "All branch paths taken.\n" );
  else
    fprintf( report, "No branch information found.\n" );
  return true;
}


bool ReportsText::PutBranchEntry(
  FILE*                                            report,
  unsigned int                                     number,
  const std::string&                               symbolName,
  const SymbolInformation&                         symbolInfo,
  Coverage::CoverageRanges::ranges_t::iterator     rangePtr
)
{
  const Coverage::Explanation* explanation;

  // Add an entry to the report
  fprintf(
    report,
    "============================================\n"
    "Symbol        : %s (0x%x)\n"
    "Line          : %s (0x%x)\n"
    "Size in Bytes : %d\n",
    symbolName.c_str(),
    symbolInfo.baseAddress,
    rangePtr->lowSourceLine.c_str(),
    rangePtr->lowAddress,
    rangePtr->highAddress - rangePtr->lowAddress + 1
  );

  if (rangePtr->reason ==
    Coverage::CoverageRanges::UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN)
    fprintf(
      report, "Reason        : %s\n\n", "ALWAYS TAKEN"
    );
  else if (rangePtr->reason ==
    Coverage::CoverageRanges::UNCOVERED_REASON_BRANCH_NEVER_TAKEN)
    fprintf( report, "Reason        : %s\n\n", "NEVER TAKEN" );

  // See if an explanation is available
  explanation = AllExplanations->lookupExplanation( rangePtr->lowSourceLine );

  if ( !explanation ) {
    fprintf(
      report,
      "Classification: NONE\n"
      "\n"
      "Explanation:\n"
      "No Explanation\n"
    );
  } else {
    fprintf(
      report,
      "Classification: %s\n"
      "\n"
      "Explanation:\n",
      explanation->classification.c_str()
    );

    for ( unsigned int i=0 ;
          i < explanation->explanation.size();
          i++) {
      fprintf(
        report,
        "%s\n",
        explanation->explanation[i].c_str()
      );
    }
  }

  fprintf(
    report, "============================================\n"
  );

  return true;
}

void ReportsText::putCoverageNoRange(
  FILE*         report,
  FILE*         noRangeFile,
  unsigned int  number,
  std::string   symbol
)
{
      fprintf(
        report,
        "============================================\n"
        "Symbol        : %s\n\n"
        "          *** NEVER REFERENCED ***\n\n"
        "This symbol was never referenced by an analyzed executable.\n"
        "Therefore there is no size or disassembly for this symbol.\n"
        "This could be due to symbol misspelling or lack of a test for\n"
        "this symbol.\n"
        "============================================\n",
        symbol.c_str()
      );
      fprintf( noRangeFile, "%s\n", symbol.c_str() );
}

bool ReportsText::PutCoverageLine(
  FILE*                                           report,
  unsigned int                                    number,
  const std::string&                              symbolName,
  const SymbolInformation&                        symbolInfo,
  Coverage::CoverageRanges::ranges_t::iterator    ritr
)
{
  const Coverage::Explanation*   explanation;

  fprintf(
    report,
    "============================================\n"
    "Index                : %d\n"
    "Symbol               : %s (0x%x)\n"
    "Starting Line        : %s (0x%x)\n"
    "Ending Line          : %s (0x%x)\n"
    "Size in Bytes        : %d\n"
    "Size in Instructions : %d\n\n",
    ritr->id,
    symbolName.c_str(),
    symbolInfo.baseAddress,
    ritr->lowSourceLine.c_str(),
    ritr->lowAddress,
    ritr->highSourceLine.c_str(),
    ritr->highAddress,
    ritr->highAddress - ritr->lowAddress + 1,
    ritr->instructionCount
  );

  explanation = AllExplanations->lookupExplanation( ritr->lowSourceLine );

  if ( !explanation ) {
    fprintf(
      report,
      "Classification: NONE\n"
      "\n"
      "Explanation:\n"
      "No Explanation\n"
    );
  } else {
    fprintf(
      report,
      "Classification: %s\n"
      "\n"
      "Explanation:\n",
      explanation->classification.c_str()
    );

    for ( unsigned int i=0; i < explanation->explanation.size(); i++) {
      fprintf( report,"%s\n", explanation->explanation[i].c_str() );
    }
  }

  fprintf(report, "============================================\n");
  return true;
}

bool  ReportsText::PutSizeLine(
  FILE*                                           report,
  unsigned int                                    number,
  const std::string&                              symbolName,
  Coverage::CoverageRanges::ranges_t::iterator    range
)
{
  fprintf(
    report,
    "%d\t%s\t%s\n",
    range->highAddress - range->lowAddress + 1,
    symbolName.c_str(),
    range->lowSourceLine.c_str()
  );
  return true;
}

bool  ReportsText::PutSymbolSummaryLine(
  FILE*                                           report,
  unsigned int                                    number,
  const std::string&                              symbolName,
  const SymbolInformation&                        symbolInfo
)
{
  float uncoveredBytes;
  float uncoveredInstructions;

  if (symbolInfo.stats.sizeInBytes == 0) {
    fprintf(
      report,
      "============================================\n"
      "Symbol                            : %s\n"
      "          *** NEVER REFERENCED ***\n\n"
      "This symbol was never referenced by an analyzed executable.\n"
      "Therefore there is no size or disassembly for this symbol.\n"
      "This could be due to symbol misspelling or lack of a test for\n"
      "this symbol.\n",
      symbolName.c_str()
    );
  } else {
    if ( symbolInfo.stats.sizeInInstructions == 0 )
      uncoveredInstructions = 0;
    else
      uncoveredInstructions = (symbolInfo.stats.uncoveredInstructions*100.0)/
                              symbolInfo.stats.sizeInInstructions;

    if ( symbolInfo.stats.sizeInBytes == 0 )
      uncoveredBytes = 0;
    else
      uncoveredBytes = (symbolInfo.stats.uncoveredBytes*100.0)/
                       symbolInfo.stats.sizeInBytes;

    fprintf(
      report,
      "============================================\n"
      "Symbol                            : %s\n"
      "Total Size in Bytes               : %d\n"
      "Total Size in Instructions        : %d\n"
      "Total number Branches             : %d\n"
      "Total Always Taken                : %d\n"
      "Total Never Taken                 : %d\n"
      "Percentage Uncovered Instructions : %.2f\n"
      "Percentage Uncovered Bytes        : %.2f\n",
      symbolName.c_str(),
      symbolInfo.stats.sizeInBytes,
      symbolInfo.stats.sizeInInstructions,
      symbolInfo.stats.branchesNotExecuted +  symbolInfo.stats.branchesExecuted,
      symbolInfo.stats.branchesAlwaysTaken,
      symbolInfo.stats.branchesNeverTaken,
      uncoveredInstructions,
      uncoveredBytes
    );
  }

  fprintf(report, "============================================\n");
  return true;
}

}
