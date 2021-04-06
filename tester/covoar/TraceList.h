/*! @file TraceList.h
 *  @brief TraceList Specification
 *
 *  This file contains the specification of the TraceList class.
 */

#ifndef __TRACE_LIST_H__
#define __TRACE_LIST_H__

#include <stdint.h>
#include <list>
#include <string>

namespace Trace {

  /*! @class TraceList
   *
   *  This class defines XXX
   */
  class TraceList {

  public:

    /*!
     *  This enumberated type defines an exit reason
     *  for the end of a section.
     */
    typedef enum {
      EXIT_REASON_BRANCH_TAKEN,
      EXIT_REASON_BRANCH_NOT_TAKEN,
      EXIT_REASON_OTHER
    } exitReason_t;

    /*!
     *  This type defines the information kept for each range.
     */
    typedef struct {
      /*!
       *  This member variable contains the low address for the
       *  trace range.
       */
      uint32_t          lowAddress;

      /*!
       *  This member variable contains the length of the trace
       *  range.
       */
      uint16_t          length;

      /*!
       *  This member variable contains the reason that this
       *  trace range ended.
       */
      exitReason_t      exitReason;
    } traceRange_t;

    /*!
     *  This member variable contains a list of CoverageRange instances.
     */
    typedef std::list<traceRange_t> ranges_t;

    /*!
     *  This member variable contains a list of coverageRange
     *  instaces.
     */
    ranges_t set;

    /*!
     *  This method constructs a TraceList instance.
     */
    TraceList();

    /*!
     *  This method destructs a TraceList instance.
     */
    ~TraceList();

    /*!
     *  This method adds a range entry to the set of ranges.
     *
     *  @param[in] lowAddressArg specifies the lowest address of the range
     *  @param[in] highAddressArg specifies the highest address of the range
     *  @param[in] why specifies the reason that the range was finished
     *
     */
    void add(
      uint32_t         lowAddressArg,
      uint32_t         highAddressArg,
      exitReason_t     why
    );

    /*!
     *  This method displays the trace information in the variable t.
     */
    void ShowTrace( traceRange_t *t);


    /*!
     *  This method iterates through set displaying the data on the screen.
     */
    void ShowList();

  };

}
#endif
