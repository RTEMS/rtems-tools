/*! @file CoverageFactory.h
 *  @brief CoverageFactory Specification
 *
 *  This file contains the specification of the CoverageFactory methods.
 *  This collection of methods is used to create CoverageReader and/or
 *  CoverageWriter instances for a particular coverage file format.
 */

#ifndef __COVERAGE_FACTORY_H__
#define __COVERAGE_FACTORY_H__

#include "CoverageReaderBase.h"
#include "CoverageWriterBase.h"

namespace Coverage {

  /*!
   *  This type defines the coverage file formats that are supported.
   */
  typedef enum {
    COVERAGE_FORMAT_QEMU,
    COVERAGE_FORMAT_RTEMS,
    COVERAGE_FORMAT_SKYEYE,
    COVERAGE_FORMAT_TSIM
  } CoverageFormats_t;

  /*!
   *  This method returns the coverage file format that corresponds
   *  to the specified string.
   *
   *  @param[in] format is a string specifying the coverage file format
   *
   *  @return Returns a coverage file format.
   */
  CoverageFormats_t CoverageFormatToEnum(
    const std::string& format
  );

  /*!
   *  This method returns an instance of a Coverage File Reader class
   *  that corresponds to the specified coverage file format.
   *
   *  @param[in] format specifies the coverage file format
   *
   *  @return Returns a Coverage File Reader class instance.
   */
  CoverageReaderBase* CreateCoverageReader(
    CoverageFormats_t format
  );

  /*!
   *  This method returns an instance of a Coverage File Writer class
   *  that corresponds to the specified coverage file format.
   *
   *  @param[in] format specifies the coverage file format
   *
   *  @return Returns a Coverage File Writer class instance.
   */
  CoverageWriterBase* CreateCoverageWriter(
    CoverageFormats_t format
  );
}

#endif
