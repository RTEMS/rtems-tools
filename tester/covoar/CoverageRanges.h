/*! @file CoverageRanges.h
 *  @brief CoverageRanges Specification
 *
 *  This file contains the specification of the CoverageRanges class.
 */

#ifndef __COVERAGE_RANGES_H__
#define __COVERAGE_RANGES_H__

#include <stdint.h>
#include <list>
#include <string>

namespace Coverage {

  /*! @class CoverageRanges
   *
   *  This class defines a set of address ranges for which coverage
   *  did not occur.  Each address range can either define a range of
   *  bytes that was not executed or a range of bytes for a branch
   *  instruction that was not completely covered (i.e. taken and NOT
   *  taken).
   */
  class CoverageRanges {

  public:

    /*!
     *  This type defines the reasons to associate with a range.
     */
    typedef enum {
      UNCOVERED_REASON_NOT_EXECUTED,
      UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN,
      UNCOVERED_REASON_BRANCH_NEVER_TAKEN
    } uncoveredReason_t;

    /*!
     *  This type defines the information kept for each range.
     */
    typedef struct {
      /*!
       *  This member contains an identification number for this 
       *  coverage range.
       */
      uint32_t          id;

      /*!
       *  This member contains the low address of this coverage 
       *  range.
       */
      uint32_t          lowAddress;

      /*!
       *  This member contains the source line associated with the 
       *  low address for this coverage range.
       */
      std::string       lowSourceLine;

      /*!
       * This member contains the high address for this coverage range.
       */
      uint32_t          highAddress;

      /*!
       *  This member contains the high source line for this coverage range.
       */
      std::string       highSourceLine;

      /*!
       * This member contains an instruction count for this coverage 
       * address range.
       */
      uint32_t          instructionCount;

      /*!
       *  This member contains the reason that this area was uncovered.
       */
      uncoveredReason_t reason;
    } coverageRange_t;

    /*!
     *  This type contains a list of CoverageRange instances.
     */
    typedef std::list<coverageRange_t> ranges_t;

    /*!
     *  This member contains a list of the CoverageRange instances.
     */
    ranges_t set;

    /*! 
     *  This method constructs a CoverageRanges instance.
     */
    CoverageRanges();

    /*! 
     *  This method destructs a CoverageRanges instance.
     */
    ~CoverageRanges();

    /*!
     *  This method adds a range entry to the set of ranges.
     *
     *  @param[in] lowAddressArg specifies the lowest address of the range
     *  @param[in] highAddressArg specifies the highest address of the range
     *  @param[in] why specifies the reason that the range was added
     *
     */
    void add(
      uint32_t          lowAddressArg,
      uint32_t          highAddressArg,
      uncoveredReason_t why,
      uint32_t          numInstructions
    );
 

    /*!
     *  This method returns the index of a range given the low address.
     *  Upon failure on finding the adress 0 is returned.
     */
    uint32_t getId( uint32_t lowAddress );
 
    protected:


  };

}
#endif
