/*! @file CoverageReaderQEMU.h
 *  @brief CoverageReaderQEMU Specification
 *
 *  This file contains the specification of the CoverageReaderQEMU class.
 */

#ifndef __COVERAGE_READER_QEMU_H__
#define __COVERAGE_READER_QEMU_H__

#include <fstream>

#include "CoverageReaderBase.h"
#include "ExecutableInfo.h"

namespace Coverage {

  /*! @class CoverageReaderQEMU
   *
   *  This class implements the functionality which reads a coverage map
   *  file produced by QEMU.  Since the SPARC has 32-bit instructions,
   *  QEMU produces a file with an integer for each 32-bit word.  The
   *  integer will have the least significant bit set if the address
   *  was executed.  QEMU also supports reporting branch information.
   *  Several bits are set to indicate whether a branch was taken and
   *  NOT taken.
@verbatim
TBD
@endverbatim
   */
  class CoverageReaderQEMU : public CoverageReaderBase {

  public:

    /* Inherit documentation from base class. */
    CoverageReaderQEMU();

    /* Inherit documentation from base class. */
    virtual ~CoverageReaderQEMU();

    /* Inherit documentation from base class. */
    void processFile(
      const std::string&    file,
      ExecutableInfo* const executableInformation
    );
  };

}
#endif
