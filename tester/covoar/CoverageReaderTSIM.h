/*! @file CoverageReaderTSIM.h
 *  @brief CoverageReaderTSIM Specification
 *
 *  This file contains the specification of the CoverageReaderTSIM class.
 */

#ifndef __COVERAGE_READER_TSIM_H__
#define __COVERAGE_READER_TSIM_H__

#include <string>

#include "CoverageReaderBase.h"
#include "ExecutableInfo.h"

namespace Coverage {

  /*! @class CoverageReaderTSIM
   *
   *  This class implements the functionality which reads a coverage map
   *  file produced by TSIM.  Since the SPARC has 32-bit instructions,
   *  TSIM produces a file with an integer for each 32-bit word.  The
   *  integer will have the least significant bit if the address
   *  was executed.
@verbatim
40000000 : 1 0 0 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 0 0 0 0 0 0 0 0 0 0 0 1
40000080 : 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
@endverbatim
   */
  class CoverageReaderTSIM : public CoverageReaderBase {

  public:

    /* Inherit documentation from base class. */
    CoverageReaderTSIM();

    /* Inherit documentation from base class. */
    virtual ~CoverageReaderTSIM();

    /* Inherit documentation from base class. */
    void processFile(
      const std::string&    file,
      ExecutableInfo* const executableInformation
    );
  };

}
#endif
