/*! @file CoverageWriterSkyeye.h
 *  @brief CoverageWriterSkyeye Specification
 *
 *  This file contains the specification of the CoverageWriterSkyeye class.
 */

#ifndef __COVERAGE_WRITER_Skyeye_H__
#define __COVERAGE_WRITER_Skyeye_H__

#include <string>

#include "CoverageMapBase.h"
#include "CoverageWriterBase.h"

namespace Coverage {

  /*! @class CoverageWriterSkyeye
   *
   *  This class writes a coverage map in Skyeye format.  The format is
   *  documented in CoverageReaderSkyeye.
   */
  class CoverageWriterSkyeye : public CoverageWriterBase {

  public:

    /*!
     *  This method constructs a CoverageWriterSkyeye instance.
     */
    CoverageWriterSkyeye();

    /*!
     *  This method destructs a CoverageWriterSkyeye instance.
     */
    virtual ~CoverageWriterSkyeye();

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
