/*! @file ReportsHtml.h
 *  @brief Reports in HTML Specification
 *
 *  This file contains the specification of the Reports methods.  This
 *  collection of methods is used to generate the various reports of
 *  the analysis results.
 */

#ifndef __REPORTSHTML_H__
#define __REPORTSHTML_H__

#include <string>
#include <fstream>

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
    ReportsHtml( time_t timestamp, const std::string& symbolSetName );
   ~ReportsHtml();

   /*!
    *  This method produces an index file.
    *
    *  @param[in] fileName identifies the file name.
    */
   void WriteIndex( const std::string& fileName ) override;

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
    *  @param[in] fileName identifies the size report file name
    */
   void WriteSizeReport( const std::string& fileName );

  protected:

    /*!
     *  This variable tracks the annotated state at the time the
     *  last line was output.  This allows the text formating to change
     *  based upon the type of lines being put out: source code or assembly
     *  object dump line....
     */
    AnnotatedLineState_t lastState_m;

    /* Inherit documentation from base class. */
    virtual void  OpenAnnotatedFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    virtual void OpenBranchFile(
      const std::string& fileName,
      bool               hasBranches,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    virtual void OpenCoverageFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    void OpenNoRangeFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    virtual void OpenSizeFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    virtual void OpenSymbolSummaryFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    virtual void CloseAnnotatedFile( std::ofstream& aFile );

    /* Inherit documentation from base class. */
    virtual void CloseBranchFile(
      std::ofstream& aFile,
      bool           hasBranches
    );

    /* Inherit documentation from base class. */
    virtual void CloseCoverageFile( std::ofstream& aFile );

    /* Inherit documentation from base class. */
    void CloseNoRangeFile( std::ofstream& aFile );

    /* Inherit documentation from base class. */
    virtual void CloseSizeFile( std::ofstream& aFile );

    /* Inherit documentation from base class. */
    virtual void CloseSymbolSummaryFile( std::ofstream& aFile );

    /* Inherit documentation from base class. */
    virtual void PutAnnotatedLine(
      std::ofstream&       aFile,
      AnnotatedLineState_t state,
      const std::string&   line,
      uint32_t             id
    );

    /* Inherit documentation from base class. */
     virtual void AnnotatedStart( std::ofstream& aFile );

    /* Inherit documentation from base class. */
     virtual void AnnotatedEnd( std::ofstream& aFile );

    /* Inherit documentation from base class. */
    virtual bool PutNoBranchInfo( std::ofstream& report );

    /* Inherit documentation from base class. */
    virtual bool PutBranchEntry(
      std::ofstream&                         report,
      unsigned int                           number,
      const std::string&                     symbolName,
      const SymbolInformation&               symbolInfo,
      const CoverageRanges::coverageRange_t& range
    );

    /* Inherit documentation from base class. */
    virtual void putCoverageNoRange(
      std::ofstream&     report,
      std::ofstream&     noRangeFile,
      unsigned int       number,
      const std::string& symbol
    );

    /* Inherit documentation from base class. */
    virtual bool PutCoverageLine(
      std::ofstream&                         report,
      unsigned int                           number,
      const std::string&                     symbolName,
      const SymbolInformation&               symbolInfo,
      const CoverageRanges::coverageRange_t& range
    );

    /* Inherit documentation from base class. */
    virtual bool PutSizeLine(
      std::ofstream&                         report,
      unsigned int                           number,
      const std::string&                     symbolName,
      const CoverageRanges::coverageRange_t& range
    );

    /* Inherit documentation from base class. */
    virtual bool PutSymbolSummaryLine(
      std::ofstream&           report,
      unsigned int             number,
      const std::string&       symbolName,
      const SymbolInformation& symbolInfo
    );

    /* Inherit documentation from base class. */
    virtual void OpenFile(
      const std::string& fileName,
      std::ofstream&     aFile
    );

    /* Inherit documentation from base class. */
    virtual bool WriteExplanationFile(
      const std::string&           fileName,
      const Coverage::Explanation* explanation
    );
  };

}

#endif
