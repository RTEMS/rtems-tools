/*! @file CoverageWriterTSIM.h
 *  @brief CoverageWriterTSIM Specification
 *
 *  This file contains the specification of the CoverageWriterTSIM class.
 */

#ifndef __COVERAGE_WRITER_TSIM_H__
#define __COVERAGE_WRITER_TSIM_H__

#include <string>

#include "CoverageMapBase.h"
#include "CoverageWriterBase.h"

namespace Coverage {

  /*! @class CoverageWriterTSIM
   *
   *  This class writes a coverage map in TSIM format.  The format is
   *  documented in CoverageReaderTSIM.
   */
  class CoverageWriterTSIM : public CoverageWriterBase {

  public:

    /*!
     *  This method constructs a CoverageWriterTSIM instance.
     */
    CoverageWriterTSIM();

    /*!
     *  This method destructs a CoverageWriterTSIM instance.
     */
    virtual ~CoverageWriterTSIM();

    /* Inherit documentation from base class. */
    void writeFile(
      const std::string& file,
      CoverageMapBase*  coverage,
      uint32_t          lowAddress,
      uint32_t          highAddress
    );
  };

}
#endif
