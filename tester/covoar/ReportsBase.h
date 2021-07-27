/*! @file ReportsBase.h
 *  @brief Reports Base Class Specification
 *
 *  This file contains the specification of the Reports methods.  This
 *  collection of methods is used to generate the various reports of
 *  the analysis results.
 */

#ifndef __REPORTSBASE_H__
#define __REPORTSBASE_H__

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <time.h>
#include "DesiredSymbols.h"
#include "Explanations.h"

namespace Coverage {

/*!
 *   This class contains the base information to create a report
 *   set.  The report set may be text based, html based or some
 *   other format to be defined at a future time.
 */
class ReportsBase {

  public:
    ReportsBase(
      time_t                  timestamp,
      const std::string&      symbolSetName,
      Coverage::Explanations& allExplanations,
      const std::string&      projectName
    );
    virtual ~ReportsBase();

    /*!
     *  This method produces an index of the reports generated.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual void WriteIndex( const std::string& fileName );

    /*!
     *  This method produces an annotated assembly listing report containing
     *  the disassembly of each symbol that was not completely covered.
     *
     *  @param[in] fileName identifies the annotated report file name
     */
    void WriteAnnotatedReport( const std::string& fileName );

    /*!
     *  This method produces a report that contains information about each
     *  uncovered branch statement.
     *
     *  @param[in] fileName identifies the branch report file name
     */
    void WriteBranchReport( const std::string& fileName );

    /*!
     *  This method produces a report that contains information about each
     *  uncovered range of bytes.
     *
     *  @param[in] fileName identifies the coverage report file name
     */
    void WriteCoverageReport( const std::string& fileName );

    /*!
     *  This method produces a summary report that lists each uncovered
     *  range of bytes.
     *
     *  @param[in] fileName identifies the report file name
     */
    void WriteSizeReport( const std::string& fileName );

    /*!
     *  This method produces a summary report that lists information on
     *  each symbol which did not achieve 100% coverage
     *
     *  @param[in] fileName identifies the report file name
     */
    void WriteSymbolSummaryReport( const std::string& fileName );

    /*!
     *  This method produces a sumary report for the overall test run.
     */
    static void  WriteSummaryReport(
      const std::string& fileName,
      const std::string& symbolSetName
    );

    /*!
     *  This method returns the unique extension for the Report
     *  type.  If the extension is ".txt" files will be
     *  named "annotated.txt", "branch.txt" ......
     */
    std::string ReportExtension() { return reportExtension_m; }

  protected:

    /*!
     * This type is used to track a state during the annotated output.
     */
    typedef enum {
      A_SOURCE,
      A_EXECUTED,
      A_NEVER_EXECUTED,
      A_BRANCH_TAKEN,
      A_BRANCH_NOT_TAKEN
    } AnnotatedLineState_t;

    /*!
     *  This member variable contains the extension used for all reports.
     */
    std::string reportExtension_m;

    /*!
     *  This member variable contains the name of the symbol set for the report.
     */
    std::string symbolSetName_m;

    /*!
     *  This member variable contains the timestamp for the report.
     */
    time_t timestamp_m;

    /*!
     *  This member variable contains the explanations to report on.
     */
    Coverage::Explanations& allExplanations_m;

    /*!
     *  This variable stores the name of the project.
     */
    std::string projectName_m;

    /*!
     *  This method Opens a report file and verifies that it opened
     *  correctly.  Upon failure NULL is returned.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] symbolSetName identifies the name of the report's symbol set
     *  @param[in] aFile identifies the file to open
     */
    static void OpenFile(
      const std::string& fileName,
      const std::string& symbolSetName,
      std::ofstream&     aFile
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] aFile identifies the file to open
     */
    virtual void OpenAnnotatedFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] hasBranches indicates if there are branches to report
     *  @param[in] aFile identifies the file to open
     */
    virtual void OpenBranchFile(
      const std::string& fileName,
      bool               hasBranches,
      std::ofstream&     aFile
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] aFile identifies the file to open
     */
    virtual void OpenCoverageFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appends any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] aFile identifies the file to open
     */
    virtual void OpenNoRangeFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] aFile identifies the file to open
     */
    virtual void OpenSizeFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] aFile identifies the file to open
     */
    virtual void OpenSymbolSummaryFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /*!
     *  This method Closes a report file.
     *
     *  @param[in] aFile identifies the file to close
     */
    static void CloseFile( std::ofstream& aFile );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the file to close
     */
    virtual void CloseAnnotatedFile( std::ofstream& aFile );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the file to close
     *  @param[in] hasBranches indicates if there are branches to report
     */
    virtual void CloseBranchFile( std::ofstream& aFile, bool hasBranches );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the file to close
     */
    virtual void CloseCoverageFile( std::ofstream& aFile );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the file to close
     */
    void  CloseNoRangeFile( std::ofstream& aFile );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the file to close
     */
    virtual void CloseSizeFile( std::ofstream& aFile );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the file to close
     */
    virtual void CloseSymbolSummaryFile( std::ofstream& aFile );

    /*!
     *  This method puts any necessary a line of annotated
     *  data into the file.
     *
     *  @param[in] aFile identifies the report file name
     *  @param[in] state identifies the state machine state
     *  @param[in] line identifies the string to print
     *  @param[in] id identifies the branch or range id.
     */
    virtual void PutAnnotatedLine(
      std::ofstream&       aFile,
      AnnotatedLineState_t state,
      const std::string&   line,
      uint32_t             id
    )=0;

    /*!
     *  This method puts any necessary header information in
     *  front of an annotated section.
     *
     *  @param[in] aFile identifies the report file name
     */
     virtual void AnnotatedStart( std::ofstream& aFile )=0;

    /*!
     *  This method puts any necessary footer information in
     *  front of an annotated section.
     *
     *  @param[in] aFile identifies the report file name
     */
     virtual void AnnotatedEnd( std::ofstream& aFile )=0;


    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] report identifies the report file name
     */
    virtual bool PutNoBranchInfo( std::ofstream& report ) = 0;

    /*!
     *  This method puts a branch entry into the branch report.
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbolName is the symbol's name.
     *  @param[in] symbolInfo is the symbol's information.
     *  @param[in] range is the range information.
     */
    virtual bool PutBranchEntry(
      std::ofstream&                         report,
      unsigned int                           number,
      const std::string&                     symbolName,
      const SymbolInformation&               symbolInfo,
      const CoverageRanges::coverageRange_t& range
    )=0;

    /*!
     *  This method reports when no range is available for
     *  a symbol in the coverage report.
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbol is a pointer to the symbol information
     */
    virtual void putCoverageNoRange(
      std::ofstream&     report,
      std::ofstream&     noRangeFile,
      unsigned int       number,
      const std::string& symbol
    )=0;

    /*!
     *  This method puts a line in the coverage report.
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbolName is the symbol's name.
     *  @param[in] symbolInfo is the symbol's information.
     *  @param[in] range is the range information.
     */
    virtual bool PutCoverageLine(
      std::ofstream&                         report,
      unsigned int                           number,
      const std::string&                     symbolName,
      const SymbolInformation&               symbolInfo,
      const CoverageRanges::coverageRange_t& range
    )=0;

    /*!
     *  This method method puts a line into the size report.
     *
     *  @param[in] report identifies the size report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbolName is the symbol's name.
     *  @param[in] range is the range information.
     */
    virtual bool PutSizeLine(
      std::ofstream&                         report,
      unsigned int                           number,
      const std::string&                     symbolName,
      const CoverageRanges::coverageRange_t& range
    )=0;

    /*!
     *  This method method puts a line into the symbol summary report.
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbolName is the symbol's name.
     *  @param[in] symbolInfo is the symbol's information.
     */
    virtual bool PutSymbolSummaryLine(
      std::ofstream&           report,
      unsigned int             number,
      const std::string&       symbolName,
      const SymbolInformation& symbolInfo
    )=0;
};

/*!
 *  This method iterates over all report set types and generates
 *  all reports.
 *
 *  @param[in] symbolSetName is the name of the symbol set to report on.
 *  @param[in] allExplanations is the explanations to report on.
 *  @param[in] verbose specifies whether to be verbose with output
 *  @param[in] projectName specifies the name of the project
 */
void GenerateReports(
  const std::string&      symbolSetName,
  Coverage::Explanations& allExplanations,
  bool                    verbose,
  const std::string&      projectName
);

}

#endif
