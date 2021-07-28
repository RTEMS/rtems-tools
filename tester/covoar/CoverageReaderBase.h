/*! @file CoverageReaderBase.h
 *  @brief CoverageReaderBase Specification
 *
 *  This file contains the specification of the CoverageReaderBase class.
 */

#ifndef __COVERAGE_READER_BASE_H__
#define __COVERAGE_READER_BASE_H__

#include "ExecutableInfo.h"

namespace Coverage {

  /*! @class CoverageReaderBase
   *
   *  This is the specification of the CoverageReader base class.
   *  All CoverageReader implementations inherit from this class.
   */
  class CoverageReaderBase {

  public:

    /*!
     *  This method constructs a CoverageReaderBase instance.
     */
    CoverageReaderBase();

    /*!
     *  This method destructs a CoverageReaderBase instance.
     */
    virtual ~CoverageReaderBase();

    /*!
     *  This method processes the coverage information from the input
     *  @a file and adds it to the specified @a executableInformation.
     *
     *  @param[in] file is the coverage file to process
     *  @param[in] executableInformation is the information for an
     *             associated executable
     */
    virtual void processFile(
      const char* const     file,
      ExecutableInfo* const executableInformation
    ) = 0;

  /*!
   *  This method retrieves the branchInfoAvailable_m variable
   */
  bool getBranchInfoAvailable() const;

  /*!
   * This member variable tells whether the branch info is available.
   */
  bool branchInfoAvailable_m = false;
  };

}
#endif
