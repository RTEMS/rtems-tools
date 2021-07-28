/*! @file CoverageReaderBase.cc
 *  @brief CoverageReaderBase Implementation
 *
 *  This file contains the implementation of the CoverageReader base class.
 *  All CoverageReader implementations inherit from this.
 */

#include "CoverageReaderBase.h"

namespace Coverage {

  CoverageReaderBase::CoverageReaderBase()
  {
  }

  CoverageReaderBase::~CoverageReaderBase()
  {
  }

  bool CoverageReaderBase::getBranchInfoAvailable() const
  {
    return branchInfoAvailable_m;
  }
}
