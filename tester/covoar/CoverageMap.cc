/*! @file CoverageMap.cc
 *  @brief CoverageMap Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  a CoverageMap.  Currently, it adds no functionality to CoverageMapBase.
 */

#include "CoverageMap.h"

namespace Coverage {

  CoverageMap::CoverageMap(
    const std::string& exefileName,
    uint32_t           low,
    uint32_t           high
  ) : CoverageMapBase( exefileName, low, high )
  {
  }

  CoverageMap::~CoverageMap()
  {
  }

}
