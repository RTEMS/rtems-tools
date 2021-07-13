/*! @file ReportsText.h
 *  @brief Reports Text Format Write Specification
 *
 *  This file contains the specification of the Reports methods.  This
 *  collection of methods is used to generate the various reports of
 *  the analysis results.
 */

#ifndef __REPORTSTEXT_H__
#define __REPORTSTEXT_H__

#include <stdint.h>
#include "ReportsBase.h"

namespace Coverage {

/*!
 *   This class contains all methods and data necessary to
 *   produce all text style reports.
 */
class ReportsText: public ReportsBase {

  public:
    ReportsText( time_t timestamp, const std::string& symbolSetName );
    virtual ~ReportsText();

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
};

}

#endif
