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
#include <time.h>
#include "DesiredSymbols.h"

namespace Coverage {

/*!
 *   This class contains the base information to create a report 
 *   set.  The report set may be text based, html based or some
 *   other format to be defined at a future time.
 */
class ReportsBase {

  public:
    ReportsBase( time_t timestamp );
    virtual ~ReportsBase();

    /*!
     *  This method produces an index of the reports generated.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual void WriteIndex(
      const char* const fileName
    );

    /*!
     *  This method produces an annotated assembly listing report containing
     *  the disassembly of each symbol that was not completely covered.
     *
     *  @param[in] fileName identifies the annotated report file name
     */
    void WriteAnnotatedReport(
      const char* const fileName
    );

    /*!
     *  This method produces a report that contains information about each
     *  uncovered branch statement.
     *
     *  @param[in] fileName identifies the branch report file name
     */
    void WriteBranchReport(
      const char* const fileName
    );

    /*!
     *  This method produces a report that contains information about each
     *  uncovered range of bytes.
     *
     *  @param[in] fileName identifies the coverage report file name
     */
    void WriteCoverageReport(
      const char* const fileName
    );

    /*!
     *  This method produces a summary report that lists each uncovered
     *  range of bytes.
     *
     *  @param[in] fileName identifies the report file name
     */
    void WriteSizeReport(
      const char* const fileName
    );

    /*!
     *  This method produces a summary report that lists information on
     *  each symbol which did not achieve 100% coverage
     *
     *  @param[in] fileName identifies the report file name
     */
    void WriteSymbolSummaryReport(
      const char* const fileName
    );

    /*!
     *  This method produces a sumary report for the overall test run.
     */
    static void  WriteSummaryReport(
      const char* const fileName
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
     *  This member variable contains the timestamp for the report.
     */
    time_t timestamp_m;

    /*!
     *  This method Opens a report file and verifies that it opened
     *  correctly.  Upon failure NULL is returned.
     *
     *  @param[in] fileName identifies the report file name
     */
     static FILE* OpenFile(
      const char* const fileName
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual FILE* OpenAnnotatedFile(
      const char* const fileName
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     *  @param[in] hasBranches indicates if there are branches to report
     */
    virtual FILE* OpenBranchFile(
      const char* const fileName,
      bool              hasBranches
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual FILE* OpenCoverageFile(
      const char* const fileName
    );
    
    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appends any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual FILE* OpenNoRangeFile(
      const char* const fileName
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual FILE* OpenSizeFile(
      const char* const fileName
    );

    /*!
     *  This method opens a report file and verifies that it opened.
     *  Then appedns any necessary header information onto the file.
     *
     *  @param[in] fileName identifies the report file name
     */
    virtual FILE* OpenSymbolSummaryFile(
      const char* const fileName
    );

    /*!
     *  This method Closes a report file. 
     *
     *  @param[in] aFile identifies the report file name
     */
    static void CloseFile(
      FILE*  aFile
    );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the report file name
     */
    virtual void CloseAnnotatedFile(
      FILE*  aFile
    );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the report file name
     *  @param[in] hasBranches indicates if there are branches to report
     */
    virtual void CloseBranchFile(
      FILE*  aFile,
      bool   hasBranches
    );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the report file name
     */
    virtual void CloseCoverageFile(
      FILE*  aFile
    );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the report file name
     */
    void  CloseNoRangeFile(
      FILE*  aFile
    );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the report file name
     */
    virtual void CloseSizeFile(
      FILE*  aFile
    );

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] aFile identifies the report file name
     */
    virtual void CloseSymbolSummaryFile(
      FILE*  aFile
    );

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
      FILE*                aFile, 
      AnnotatedLineState_t state, 
      std::string          line,
      uint32_t             id 
    )=0;

    /*!
     *  This method puts any necessary header information in
     *  front of an annotated section.
     *
     *  @param[in] aFile identifies the report file name
     */
     virtual void AnnotatedStart(
      FILE*                aFile
    )=0;
 
    /*!
     *  This method puts any necessary footer information in
     *  front of an annotated section.
     *
     *  @param[in] aFile identifies the report file name
     */
     virtual void AnnotatedEnd(
      FILE*                aFile
    )=0;
 

    /*!
     *  This method puts any necessary footer information into
     *  the report then closes the file.
     *
     *  @param[in] report identifies the report file name
     */
    virtual bool PutNoBranchInfo(
      FILE* report
    ) = 0;

    /*!
     *  This method puts a branch entry into the branch report. 
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbolPtr is a pointer to the symbol information
     *  @param[in] rangePtr is a pointer to the range information.
     */
    virtual bool PutBranchEntry(
      FILE*                                            report,
      unsigned int                                     number,
      Coverage::DesiredSymbols::symbolSet_t::iterator  symbolPtr,
      Coverage::CoverageRanges::ranges_t::iterator     rangePtr
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
      FILE*        report,
      FILE*        noRangeFile,
      unsigned int number,
      std::string  symbol
    )=0;

    /*!
     *  This method puts a line in the coverage report.
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] ditr is a iterator to the symbol information
     *  @param[in] ritr is a iterator to the range information.
     */
    virtual bool PutCoverageLine(
      FILE*                                           report,
      unsigned int                                    number,
      Coverage::DesiredSymbols::symbolSet_t::iterator ditr,
      Coverage::CoverageRanges::ranges_t::iterator    ritr
    )=0;

    /*!
     *  This method method puts a line into the size report.
     *
     *  @param[in] report identifies the size report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbol is a pointer to the symbol information
     *  @param[in] range is a iterator to the range information.
     */
    virtual bool PutSizeLine(
      FILE*                                           report,
      unsigned int                                    number,
      Coverage::DesiredSymbols::symbolSet_t::iterator symbol,
      Coverage::CoverageRanges::ranges_t::iterator    range
    )=0;

    /*!
     *  This method method puts a line into the symbol summary report.
     *
     *  @param[in] report identifies the report file name
     *  @param[in] number identifies the line number.
     *  @param[in] symbol is a pointer to the symbol information
     */
    virtual bool PutSymbolSummaryLine(
      FILE*                                           report,
      unsigned int                                    number,
      Coverage::DesiredSymbols::symbolSet_t::iterator symbol
    )=0;
};

/*!
 *  This method iterates over all report set types and generates
 *  all reports.
 */
void GenerateReports();

}

#endif
