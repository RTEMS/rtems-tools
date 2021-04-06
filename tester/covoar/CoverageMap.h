/*! @file CoverageMap.h
 *  @brief CoverageMap Specification
 *
 *  This file contains the specification of the CoverageMap class.
 */

#ifndef __COVERAGE_MAP_H__
#define __COVERAGE_MAP_H__

#include "CoverageMapBase.h"

namespace Coverage {

  /*! @class CoverageMap
   *
   *  This class implements a coverage map which supports a single
   *  range of addresses from low to high.
   */
  class CoverageMap : public CoverageMapBase {

  public:

    /*!
     *  This method constructs a CoverageMap instance.
     *
     *  @param[in] low specifies the lowest address of the coverage map.
     *  @param[in] high specifies the highest address of the coverage map.
     */
    CoverageMap(
      const std::string& exefileName,
      uint32_t           low,
      uint32_t           high
    );

    /* Inherit documentation from base class. */
    virtual ~CoverageMap();

  };

}
#endif
