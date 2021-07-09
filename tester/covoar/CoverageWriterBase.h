/*! @file CoverageWriterBase.h
 *  @brief CoverageWriterBase Specification
 *
 *  This file contains the specification of the CoverageWriterBase class.
 */

#ifndef __COVERAGE_WRITER_BASE_H__
#define __COVERAGE_WRITER_BASE_H__

#include <stdint.h>

#include <string>

#include "CoverageMapBase.h"

namespace Coverage {

  /*! @class CoverageWriterBase
   *
   *  This is the specification of the CoverageWriter base class.
   *  All CoverageWriter implementations inherit from this class.
   */
  class CoverageWriterBase {

  public:

    /*!
     *  This method constructs a CoverageWriterBase instance.
     */
    CoverageWriterBase();

    /*!
     *  This method destructs a CoverageWriterBase instance.
     */
    virtual ~CoverageWriterBase();

    /*!
     *  This method writes the @a coverage map for the specified
     *  address range and writes it to @file.
     *
     *  @param[in] file specifies the name of the file to write
     *  @param[in] coverage specifies the coverage map to output
     *  @param[in] lowAddress specifies the lowest address in the
     *             coverage map to process
     *  @param[in] highAddress specifies the highest address in the
     *             coverage map to process
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    virtual void writeFile(
      const std::string& file,
      CoverageMapBase*  coverage,
      uint32_t          lowAddress,
      uint32_t          highAddress
    ) = 0;
  };

}
#endif
