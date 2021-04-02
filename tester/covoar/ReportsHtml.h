/*! @file ReportsHtml.h
 *  @brief Reports in HTML Specification
 *
 *  This file contains the specification of the Reports methods.  This
 *  collection of methods is used to generate the various reports of
 *  the analysis results.
 */

#ifndef __REPORTSHTML_H__
#define __REPORTSHTML_H__

#include <stdint.h>
#include "ReportsBase.h"
#include "Explanations.h"

namespace Coverage {

/*!
 *   This class contains all methods and data necessary to
 *   do all of the HTML style reports.
 */
class ReportsHtml: public ReportsBase {

  public:
    ReportsHtml( time_t timestamp, std::string symbolSetName );
   ~ReportsHtml();

   /*!
    *  This method produces an index file.
    *
    *  @param[in] fileName identifies the file name.
    */
   void WriteIndex(
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
    *  @param[in] fileName identifies the size report file name
    */
   void WriteSizeReport(
     const char* const fileName
   );

  protected:

    /*!
     *  This variable tracks the annotated state at the time the 
     *  last line was output.  This allows the text formating to change
     *  based upon the type of lines being put out: source code or assembly
     *  object dump line....
     */
    AnnotatedLineState_t lastState_m;

    /* Inherit documentation from base class. */ 
    virtual FILE* OpenAnnotatedFile(
      const char* const fileName
    );

    /* Inherit documentation from base class. */ 
    virtual FILE* OpenBranchFile(
      const char* const fileName,
      bool              hasBranches
    );

    /* Inherit documentation from base class. */ 
    virtual FILE* OpenCoverageFile(
      const char* const fileName
    );

    /* Inherit documentation from base class. */ 
    FILE* OpenNoRangeFile(
      const char* const fileName
    );

    /* Inherit documentation from base class. */ 
    virtual FILE* OpenSizeFile(
      const char* const fileName
    );

    /* Inherit documentation from base class. */ 
    virtual FILE* OpenSymbolSummaryFile(
      const char* const fileName
    );

    /* Inherit documentation from base class. */ 
    virtual void CloseAnnotatedFile(
      FILE*  aFile
    );

    /* Inherit documentation from base class. */ 
    virtual void CloseBranchFile(
      FILE*  aFile,
      bool   hasBranches
    );

    /* Inherit documentation from base class. */ 
    virtual void CloseCoverageFile(
      FILE*  aFile
    );

    /* Inherit documentation from base class. */ 
    void CloseNoRangeFile(
      FILE*  aFile
    );

    /* Inherit documentation from base class. */ 
    virtual void CloseSizeFile(
      FILE*  aFile
    );

    /* Inherit documentation from base class. */ 
    virtual void CloseSymbolSummaryFile(
      FILE*  aFile
    );

    /* Inherit documentation from base class. */ 
    virtual void PutAnnotatedLine( 
      FILE*                aFile, 
      AnnotatedLineState_t state, 
      std::string          line, 
      uint32_t             id 
    );

    /* Inherit documentation from base class. */ 
     virtual void AnnotatedStart(
      FILE*                aFile
    );
 
    /* Inherit documentation from base class. */ 
     virtual void AnnotatedEnd(
      FILE*                aFile
    );
 
    /* Inherit documentation from base class. */ 
    virtual bool PutNoBranchInfo(
      FILE* report
    );

    /* Inherit documentation from base class. */ 
    virtual bool PutBranchEntry(
      FILE*                                            report,
      unsigned int                                     number,
      const std::string&                               symbolName,
      const SymbolInformation&                         symbolInfo,
      Coverage::CoverageRanges::ranges_t::iterator     rangePtr
    );

    /* Inherit documentation from base class. */ 
    virtual void putCoverageNoRange(
      FILE*        report,
      FILE*        noRangeFile,
      unsigned int number,
      std::string  symbol
    );

    /* Inherit documentation from base class. */ 
    virtual bool PutCoverageLine(
      FILE*                                           report,
      unsigned int                                    number,
      const std::string&                              symbolName,
      const SymbolInformation&                        symbolInfo,
      Coverage::CoverageRanges::ranges_t::iterator    ritr
    );

    /* Inherit documentation from base class. */ 
    virtual bool PutSizeLine(
      FILE*                                           report,
      unsigned int                                    number,
      const std::string&                              symbolName,
      Coverage::CoverageRanges::ranges_t::iterator    range
    );

    /* Inherit documentation from base class. */ 
    virtual bool PutSymbolSummaryLine(
      FILE*                                           report,
      unsigned int                                    number,
      const std::string&                              symbolName,
      const SymbolInformation&                        symbolInfo
    );

    /* Inherit documentation from base class. */ 
    virtual FILE* OpenFile(
      const char* const fileName
    );

    /* Inherit documentation from base class. */ 
    virtual bool WriteExplationFile(
      const char*                  fileName,
      const Coverage::Explanation* explanation
    );
  };

}

#endif
