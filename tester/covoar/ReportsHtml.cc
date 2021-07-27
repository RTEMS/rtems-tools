#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <iomanip>

#include <rld.h>
#include <rtems-utils.h>

#include "ReportsHtml.h"
#include "app_common.h"
#include "CoverageRanges.h"
#include "DesiredSymbols.h"
#include "ObjdumpProcessor.h"

typedef rtems::utils::ostream_guard ostream_guard;

#if 0
#define TABLE_HEADER_CLASS \
  " table-autopage:10 table-page-number:pagenum table-page-count:pages "
#define TABLE_FOOTER \
   "<tfoot>\n" \
   "<tr>\n" \
   "<td class=\"table-page:previous\" " \
      "style=\"cursor:pointer;\">&lt; &lt; Previous</td>\n" \
   "<td colspan=\"4\" style=\"text-align:center;\">Page " \
      "<span id=\"pagenum\"></span>&nbsp;of <span id=\"pages\"></span></td>\n" \
   "<td class=\"table-page:next\" " \
     "style=\"cursor:pointer;\">Next &gt; &gt;</td>\n" \
   "</tr>\n" \
   "</tfoot>\n"
#else
#define TABLE_HEADER_CLASS ""
#define TABLE_FOOTER ""
#endif

namespace Coverage {

  ReportsHtml::ReportsHtml(
    time_t                  timestamp,
    const std::string&      symbolSetName,
    Coverage::Explanations& allExplanations
  ): ReportsBase( timestamp, symbolSetName, allExplanations ),
     lastState_m( A_SOURCE )
  {
    reportExtension_m = ".html";
  }

  ReportsHtml::~ReportsHtml()
  {
  }

  void ReportsHtml::WriteIndex( const std::string& fileName )
  {
    std::ofstream aFile;
    #define PRINT_ITEM( _t, _n ) \
       aFile << "<li>" \
             << _t << " (<a href=\"" \
             << _n << ".html\">html</a> or <a href=\"" \
             << _n << ".txt\">text</a>)</li>" << std::endl;
    #define PRINT_TEXT_ITEM( _t, _n ) \
       aFile << "<li>" \
             << _t << " (<a href=\"" \
             << _n << "\">text</a>)</li>" << std::endl;


    // Open the file
    OpenFile( fileName, aFile );

    aFile << "<title>Index</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "Coverage Analysis Reports</div>" << std::endl
          << "<div class =\"datetime\">"
          <<  asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<ul>" << std::endl;

    PRINT_TEXT_ITEM( "Summary",                     "summary.txt" );
    PRINT_ITEM(      "Coverage Report",             "uncovered" );
    PRINT_ITEM(      "Branch Report",               "branch" );
    PRINT_ITEM(      "Annotated Assembly",          "annotated" );
    PRINT_ITEM(      "Symbol Summary",              "symbolSummary" );
    PRINT_ITEM(      "Uncovered Range Size Report", "sizes" );
    PRINT_TEXT_ITEM( "Explanations Not Found",      "ExplanationsNotFound.txt" );

    aFile << "</ul>" << std::endl
          << "<!-- INSERT PROJECT SPECIFIC ITEMS HERE -->" << std::endl
          << "</html>" << std::endl;

    CloseFile( aFile );

    #undef PRINT_ITEM
    #undef PRINT_TEXT_ITEM
  }

  void ReportsHtml::OpenFile(
    const std::string& fileName,
    std::ofstream&     aFile
  )
  {
    // Open the file
    ReportsBase::OpenFile( fileName, symbolSetName_m, aFile );

    // Put Header information on the file
    aFile << "<html>" << std::endl
          << "<meta http-equiv=\"Content-Language\" content=\"English\" >"
          << std::endl
          << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=us-ascii\" >"
          << std::endl
          << "<link rel=\"stylesheet\" type=\"text/css\" href=\"../covoar.css\" media=\"screen\" >"
          << std::endl
          << "<script type=\"text/javascript\" src=\"../table.js\"></script>"
          << std::endl;
  }

  void ReportsHtml::OpenAnnotatedFile(
    const std::string& fileName,
    std::ofstream&     aFile
  )
  {
    // Open the file
    OpenFile( fileName, aFile );

    aFile << "<title>Annotated Report</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "Annotated Report</div>" << std::endl
          << "<div class =\"datetime\">"
          << asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<body>" << std::endl
          << "<pre class=\"code\">" << std::endl;
  }

  void ReportsHtml::OpenBranchFile(
    const std::string& fileName,
    bool               hasBranches,
    std::ofstream&     aFile
  )
  {
    // Open the file
    OpenFile( fileName, aFile );

    // Put header information into the file
    aFile << "<title>Branch Report</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "Branch Report</div>" << std::endl
          << "<div class =\"datetime\">"
          << asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<body>" << std::endl
          << "<table class=\"covoar table-autosort:0 table-autofilter table-stripeclass:covoar-tr-odd"
          << TABLE_HEADER_CLASS << "\">" << std::endl
          << "<thead>" << std::endl
          << "<tr>" << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Symbol</th>" << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Line</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">File</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"left\">Size <br>Bytes</th>"
          << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Reason</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">Taken</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">Not Taken</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">Classification</th>"
          << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Explanation</th>"
          << std::endl
          << "</tr>" << std::endl
          << "</thead>" << std::endl
          << "<tbody>" << std::endl;
  }

  void  ReportsHtml::OpenCoverageFile(
    const std::string& fileName,
    std::ofstream&     aFile
  )
  {
    // Open the file
    OpenFile( fileName, aFile );

    // Put header information into the file
    aFile << "<title>Coverage Report</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "Coverage Report</div>" << std::endl
          << "<div class =\"datetime\">"
          << asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<body>" << std::endl
          << "<table class=\"covoar table-autosort:0 table-autofilter table-stripeclass:covoar-tr-odd"
          << TABLE_HEADER_CLASS << "\">" << std::endl
          << "<thead>" << std::endl
          << "<tr>" << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Symbol</th>"
          << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Range</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">File</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"left\">Size <br>Bytes</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"left\">Size <br>Instructions</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">Classification</th>"
          << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Explanation</th>"
          << std::endl
          << "</tr>" << std::endl
          << "</thead>" << std::endl
          << "<tbody>" << std::endl;
  }

  void ReportsHtml::OpenNoRangeFile(
    const std::string& fileName,
    std::ofstream&     aFile
  )
  {
    // Open the file
    OpenFile( fileName, aFile );

    // Put header information into the file
    aFile << "<title> Report</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "No Range Report</div>" << std::endl
          << "<div class =\"datetime\">"
          << asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<body>" << std::endl
          << "<table class=\"covoar table-autosort:0 table-autofilter table-stripeclass:covoar-tr-odd"
          << TABLE_HEADER_CLASS << "\">" << std::endl
          << "<thead>" << std::endl
          << "<tr>" << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Symbol</th>"
          << std::endl
          << "</tr>" << std::endl
          << "</thead>" << std::endl
          << "<tbody>" << std::endl;
   }



  void ReportsHtml::OpenSizeFile(
    const std::string& fileName,
    std::ofstream&     aFile
  )
  {
    // Open the file
    OpenFile( fileName, aFile );

    // Put header information into the file
    aFile << "<title>Uncovered Range Size Report</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "Uncovered Range Size Report</div>" << std::endl
          << "<div class =\"datetime\">"
          << asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<body>" << std::endl
          << "<table class=\"covoar table-autosort:0 table-autofilter table-stripeclass:covoar-tr-odd"
          << TABLE_HEADER_CLASS << "\">" << std::endl
          << "<thead>" << std::endl
          << "<tr>" << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"left\">Size</th>"
          << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Symbol</th>"
          << std::endl
          << "<th class=\"table-sortable:default\" align=\"left\">Line</th>"
          << std::endl
          << "<th class=\"table-filterable table-sortable:default\" align=\"left\">File</th>"
          << std::endl
          << "</tr>" << std::endl
          << "</thead>" << std::endl
          << "<tbody>" << std::endl;
  }

  void ReportsHtml::OpenSymbolSummaryFile(
    const std::string& fileName,
    std::ofstream&     aFile
  )
  {
    // Open the file
    OpenFile( fileName, aFile );

    // Put header information into the file
    aFile << "<title>Symbol Summary Report</title>" << std::endl
          << "<div class=\"heading-title\">";

    if ( projectName ) {
      aFile << projectName << "<br>";
    }

    aFile << "Symbol Summary Report</div>" << std::endl
          << "<div class =\"datetime\">"
          << asctime( localtime( &timestamp_m ) ) << "</div>" << std::endl
          << "<body>" << std::endl
          << "<table class=\"covoar table-autosort:0 table-autofilter table-stripeclass:covoar-tr-odd"
          << TABLE_HEADER_CLASS << "\">" << std::endl
          << "<thead>" << std::endl
          << "<tr>" << std::endl
          << "<th class=\"table-sortable:default\" align=\"center\">Symbol</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">Total<br>Size<br>Bytes</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">Total<br>Size<br>Instr</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">#<br>Ranges</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">Uncovered<br>Size<br>Bytes</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">Uncovered<br>Size<br>Instr</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">#<br>Branches</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">#<br>Always<br>Taken</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">#<br>Never<br>Taken</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">Percent<br>Uncovered<br>Instructions</th>"
          << std::endl
          << "<th class=\"table-sortable:numeric\" align=\"center\">Percent<br>Uncovered<br>Bytes</th>"
          << std::endl
          << "</tr>" << std::endl
          << "</thead>" << std::endl
          << "<tbody>" << std::endl;
  }

  void ReportsHtml::AnnotatedStart( std::ofstream& aFile )
  {
    aFile << "<hr>" << std::endl;
  }

  void ReportsHtml::AnnotatedEnd( std::ofstream& aFile )
  {
  }

  void ReportsHtml::PutAnnotatedLine(
    std::ofstream&       aFile,
    AnnotatedLineState_t state,
    const std::string&   line,
    uint32_t             id
  )
  {
    std::string stateText;
    std::string number;

    number = std::to_string( id );

    // Set the stateText based upon the current state.
    switch ( state ) {
      case A_SOURCE:
        stateText = "</pre>\n<pre class=\"code\">\n";
        break;
      case A_EXECUTED:
        stateText = "</pre>\n<pre class=\"codeExecuted\">\n";
        break;
      case A_NEVER_EXECUTED:
        stateText = "</pre>\n";
        stateText += "<a name=\"range";
        stateText += number;
        stateText += "\"></a><pre class=\"codeNotExecuted\">\n";
        break;
      case A_BRANCH_TAKEN:
        stateText = "</pre>\n";
        stateText += "<a name=\"range";
        stateText += number;
        stateText += "\"></a><pre class=\"codeAlwaysTaken\">\n";
        break;
      case A_BRANCH_NOT_TAKEN:
        stateText = "</pre>\n";
        stateText += "<a name=\"range";
        stateText += number;
        stateText += "\"></a><pre class=\"codeNeverTaken\">\n";
        break;
      default:
        throw rld::error( "Unknown state", "ReportsHtml::PutAnnotatedLine" );
        break;
    }

    // If the state has not changed there is no need to change the text block
    // format.  If it has changed close out the old format and open up the
    // new format.
    if ( state != lastState_m ) {
      aFile << stateText;
      lastState_m = state;
    }

    // For all the characters in the line replace html reserved special
    // characters and output the line. Note that for a /pre block this
    // is only a '<' symbol.
    for ( unsigned int i=0; i<line.size(); i++ ) {
      if ( line[i] == '<' ) {
        aFile << "&lt;";
      } else {
        aFile << line[i];
      }
    }
    aFile << std::endl;
  }

  bool ReportsHtml::PutNoBranchInfo( std::ofstream& report )
  {
    if (
      BranchInfoAvailable &&
      SymbolsToAnalyze->getNumberBranchesFound( symbolSetName_m ) != 0
    ) {
      report << "All branch paths taken." << std::endl;
    } else {
      report << "No branch information found." << std::endl;
    }

    return true;
  }

  bool ReportsHtml::PutBranchEntry(
    std::ofstream&                         report,
    unsigned int                           count,
    const std::string&                     symbolName,
    const SymbolInformation&               symbolInfo,
    const CoverageRanges::coverageRange_t& range
  )
  {
    const Coverage::Explanation* explanation;
    std::string                  temp;
    int                          i;
    uint32_t                     bAddress = 0;
    uint32_t                     lowAddress = 0;
    Coverage::CoverageMapBase*   theCoverageMap = NULL;

    // Mark the background color different for odd and even lines.
    if ( ( count % 2 ) != 0 ) {
      report << "<tr class=\"covoar-tr-odd\">\n";
    } else {
      report << "<tr>" << std::endl;
    }

    // symbol
    report << "<td class=\"covoar-td\" align=\"center\">"
           << symbolName << "</td>" << std::endl;

    // line
    report << "<td class=\"covoar-td\" align=\"center\"><a href =\"annotated.html#range"
           << range.id << "\">"
           << range.lowSourceLine << "</td>" << std::endl;

    // File
    i = range.lowSourceLine.find( ":" );
    temp = range.lowSourceLine.substr( 0, i );
    report << "<td class=\"covoar-td\" align=\"center\">"
           << temp << "</td>" << std::endl;

    // Size in bytes
    report << "<td class=\"covoar-td\" align=\"center\">"
           << range.highAddress - range.lowAddress + 1 << "</td>" << std::endl;

    // Reason Branch was uncovered
    if (
      range.reason ==
      Coverage::CoverageRanges::UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN
    ) {
      report << "<td class=\"covoar-td\" align=\"center\">Always Taken</td>"
             << std::endl;
    } else if (
      range.reason ==
      Coverage::CoverageRanges::UNCOVERED_REASON_BRANCH_NEVER_TAKEN
    ) {
      report << "<td class=\"covoar-td\" align=\"center\">Never Taken</td>"
             << std::endl;
    }

    // Taken / Not taken counts
    lowAddress     = range.lowAddress;
    bAddress       = symbolInfo.baseAddress;
    theCoverageMap = symbolInfo.unifiedCoverageMap;

    report << "<td class=\"covoar-td\" align=\"center\">"
           << theCoverageMap->getWasTaken( lowAddress - bAddress )
           << "</td>" << std::endl
           << "<td class=\"covoar-td\" align=\"center\">"
           << theCoverageMap->getWasNotTaken( lowAddress - bAddress )
           << "</td>" << std::endl;

    // See if an explanation is available and write the Classification and
    // the Explination Columns.
    explanation = allExplanations_m.lookupExplanation( range.lowSourceLine );
    if ( !explanation ) {
      // Write Classificationditr->second.baseAddress
      report << "<td class=\"covoar-td\" align=\"center\">NONE</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">No Explanation</td>"
             << std::endl;
    } else {
      std::stringstream explanationFile( "explanation" );
      explanationFile << range.id << ".html";

      report << "<td class=\"covoar-td\" align=\"center\">"
             << explanation->classification << "</td>" << std::endl
             << "<td class=\"covoar-td\" align=\"center\">"
             << "<a href=\"" << explanationFile.str()
             << "\">Explanation</a></td>" << std::endl;

      WriteExplanationFile( explanationFile.str(), explanation );
    }

    report << "</tr>" << std::endl;

    return true;
  }

  bool ReportsHtml::WriteExplanationFile(
    const std::string&           fileName,
    const Coverage::Explanation* explanation
  )
  {
    std::ofstream report;

    OpenFile( fileName, report );

    for ( unsigned int i=0 ; i < explanation->explanation.size(); i++ ) {
      report << explanation->explanation[i] << std::endl;
    }
    CloseFile( report );
    return true;
  }

  void ReportsHtml::putCoverageNoRange(
    std::ofstream&     report,
    std::ofstream&     noRangeFile,
    unsigned int       count,
    const std::string& symbol
  )
  {
    Coverage::Explanation explanation;

    explanation.explanation.push_back(
      "<html><p>\n"
      "This symbol was never referenced by an analyzed executable.  "
      "Therefore there is no size or disassembly for this symbol.  "
      "This could be due to symbol misspelling or lack of a test for "
      "this symbol."
      "</p></html>\n"
    );

    // Mark the background color different for odd and even lines.
    if ( ( count % 2 ) != 0 ) {
      report << "<tr class=\"covoar-tr-odd\">" << std::endl;
      noRangeFile << "<tr class=\"covoar-tr-odd\">" << std::endl;
    } else {
      report << "<tr>" << std::endl;
      noRangeFile << "<tr>" << std::endl;
    }

    // symbol
    report << "<td class=\"covoar-td\" align=\"center\">"
           << symbol << "</td>" << std::endl;
    noRangeFile << "<td class=\"covoar-td\" align=\"center\">"
                << symbol << "</td>" << std::endl;

    // starting line
    report << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
           << std::endl;

    // file
    report << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
           << std::endl;

     // Size in bytes
    report << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
           << std::endl;

    // Size in instructions
    report << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
           << std::endl;

    // See if an explanation is available
    report << "<td class=\"covoar-td\" align=\"center\">Unknown</td>"
           << std::endl
           << "<td class=\"covoar-td\" align=\"center\">"
           << "<a href=\"NotReferenced.html\">No data</a></td>"
           << std::endl;

    WriteExplanationFile( "NotReferenced.html", &explanation );

    report << "</tr>" << std::endl;
    noRangeFile << "</tr>" << std::endl;
  }

  bool ReportsHtml::PutCoverageLine(
    std::ofstream&                         report,
    unsigned int                           count,
    const std::string&                     symbolName,
    const SymbolInformation&               symbolInfo,
    const CoverageRanges::coverageRange_t& range
  )
  {
    const Coverage::Explanation* explanation;
    std::string                  temp;
    int                          i;

    // Mark the background color different for odd and even lines.
    if ( ( count % 2) != 0 ) {
      report << "<tr class=\"covoar-tr-odd\">" << std::endl;
    } else {
      report << "<tr>" << std::endl;
    }

    // symbol
    report << "<td class=\"covoar-td\" align=\"center\">"
           << symbolName << "</td>" << std::endl;

    // Range
    report << "<td class=\"covoar-td\" align=\"center\"><a href =\"annotated.html#range"
           << range.id << "\">"
           << range.lowSourceLine << " <br>"
           << range.highSourceLine << "</td>" << std::endl;

    // File
    i = range.lowSourceLine.find( ":" );
    temp = range.lowSourceLine.substr( 0, i );

    report << "<td class=\"covoar-td\" align=\"center\">"
           << temp << "</td>" << std::endl;

    // Size in bytes
    report << "<td class=\"covoar-td\" align=\"center\">"
           << range.highAddress - range.lowAddress + 1 << "</td>" << std::endl;

    // Size in instructions
    report << "<td class=\"covoar-td\" align=\"center\">"
           << range.instructionCount << "</td>" << std::endl;

    // See if an explanation is available
    explanation = allExplanations_m.lookupExplanation( range.lowSourceLine );
    if ( !explanation ) {
      report << "<td class=\"covoar-td\" align=\"center\">NONE</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">No Explanation</td>"
             << std::endl;
    } else {
      std::stringstream explanationFile( "explanation" );

      explanationFile << range.id << ".html";

      report << "<td class=\"covoar-td\" align=\"center\">"
             << explanation->classification << "</td>" << std::endl
             << "<td class=\"covoar-td\" align=\"center\">"
             << "<a href=\""
             << explanationFile.str() << "\">Explanation</a></td>"
             << std::endl;

      WriteExplanationFile( explanationFile.str(), explanation );
    }

    report << "</tr>" << std::endl;

    return true;
  }

  bool  ReportsHtml::PutSizeLine(
    std::ofstream&                         report,
    unsigned int                           count,
    const std::string&                     symbolName,
    const CoverageRanges::coverageRange_t& range
  )
  {
    std::string temp;
    int         i;

    // Mark the background color different for odd and even lines.
    if ( ( count % 2 ) != 0 ) {
      report << "<tr class=\"covoar-tr-odd\">" << std::endl;
    } else {
      report << "<tr>" << std::endl;
    }

    // size
    report << "<td class=\"covoar-td\" align=\"center\">"
           << range.highAddress - range.lowAddress + 1 << "</td>" << std::endl;

    // symbol
    report << "<td class=\"covoar-td\" align=\"center\">"
           << symbolName << "</td>" << std::endl;

    // line
    report << "<td class=\"covoar-td\" align=\"center\"><a href =\"annotated.html#range"
           << range.id << "\">"
           << range.lowSourceLine << "</td>" << std::endl;

    // File
    i = range.lowSourceLine.find( ":" );
    temp =  range.lowSourceLine.substr( 0, i );
    report << "<td class=\"covoar-td\" align=\"center\">"
           << temp << "</td>" << std::endl
           << "</tr>" << std::endl;

    return true;
  }

  bool  ReportsHtml::PutSymbolSummaryLine(
    std::ofstream&           report,
    unsigned int             count,
    const std::string&       symbolName,
    const SymbolInformation& symbolInfo
  )
  {
    ostream_guard old_state( report );

    // Mark the background color different for odd and even lines.
    if ( ( count % 2 ) != 0 ) {
      report << "<tr class=\"covoar-tr-odd\">" << std::endl;
    } else {
      report << "<tr>" << std::endl;
    }

    // symbol
    report << "<td class=\"covoar-td\" align=\"center\">"
           << symbolName << "</td>" << std::endl;

    if ( symbolInfo.stats.sizeInBytes == 0 ) {
      // The symbol has never been seen. Write "unknown" for all columns.
      report << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl
             << "<td class=\"covoar-td\" align=\"center\">unknown</td>"
             << std::endl;
    } else {
      // Total Size in Bytes
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.sizeInBytes << "</td>" << std::endl;

      // Total Size in Instructions
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.sizeInInstructions << "</td>" << std::endl;

      // Total Uncovered Ranges
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.uncoveredRanges << "</td>" << std::endl;

      // Uncovered Size in Bytes
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.uncoveredBytes << "</td>" << std::endl;

      // Uncovered Size in Instructions
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.uncoveredInstructions << "</td>" << std::endl;

      // Total number of branches
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.branchesNotExecuted +
                symbolInfo.stats.branchesExecuted
             << "</td>" << std::endl;

      // Total Always Taken
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.branchesAlwaysTaken << "</td>" << std::endl;

      // Total Never Taken
      report << "<td class=\"covoar-td\" align=\"center\">"
             << symbolInfo.stats.branchesNeverTaken << "</td>" << std::endl;

      // % Uncovered Instructions
      if ( symbolInfo.stats.sizeInInstructions == 0 ) {
        report << "<td class=\"covoar-td\" align=\"center\">100.00</td>"
               << std::endl;
      } else {
        report << "<td class=\"covoar-td\" align=\"center\">"
               << std::fixed << std::setprecision( 2 )
               << ( symbolInfo.stats.uncoveredInstructions * 100.0 ) /
                    symbolInfo.stats.sizeInInstructions
               << "</td>" << std::endl;
      }

      // % Uncovered Bytes
      if ( symbolInfo.stats.sizeInBytes == 0 ) {
        report << "<td class=\"covoar-td\" align=\"center\">100.00</td>"
               << std::endl;
      } else {
        report << "<td class=\"covoar-td\" align=\"center\">"
               << ( symbolInfo.stats.uncoveredBytes * 100.0 ) /
                    symbolInfo.stats.sizeInBytes
               << "</td>" << std::endl;
      }
    }

    report << "</tr>" << std::endl;

    return true;
  }

  void ReportsHtml::CloseAnnotatedFile( std::ofstream& aFile )
  {
    aFile << "</pre>"  << std::endl
          << "</body>" << std::endl
          << "</html>" << std::endl;

    CloseFile( aFile );
  }

  void ReportsHtml::CloseBranchFile( std::ofstream& aFile, bool hasBranches )
  {
    aFile << TABLE_FOOTER
          << "</tbody>" << std::endl
          << "</table>" << std::endl;

    CloseFile( aFile );
  }

  void ReportsHtml::CloseCoverageFile( std::ofstream& aFile )
  {
    aFile << TABLE_FOOTER
          << "</tbody>" << std::endl
          << "</table>" << std::endl
          << "</pre>"   << std::endl
          << "</body>"  << std::endl
          << "</html>";

    CloseFile( aFile );
  }

  void ReportsHtml::CloseNoRangeFile( std::ofstream& aFile )
  {
    aFile << TABLE_FOOTER
          << "</tbody>" << std::endl
          << "</table>" << std::endl
          << "</pre>"   << std::endl
          << "</body>"  << std::endl
          << "</html>";

    CloseFile( aFile );
  }


  void ReportsHtml::CloseSizeFile( std::ofstream& aFile )
  {
    aFile << TABLE_FOOTER
          << "</tbody>" << std::endl
          << "</table>" << std::endl
          << "</pre>"   << std::endl
          << "</body>"  << std::endl
          << "</html>";

    CloseFile( aFile );
  }

  void ReportsHtml::CloseSymbolSummaryFile( std::ofstream& aFile )
  {
    aFile << TABLE_FOOTER
          << "</tbody>" << std::endl
          << "</table>" << std::endl
          << "</pre>"   << std::endl
          << "</body>"  << std::endl
          << "</html>";

     CloseFile( aFile );
  }

}
