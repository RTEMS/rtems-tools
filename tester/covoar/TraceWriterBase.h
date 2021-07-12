/*! @file TraceWriterBase.h
 *  @brief TraceWriterBase Specification
 *
 *  This file contains the specification of the TraceWriterBase class.
 */

#ifndef __TRACE_WRITER_BASE_H__
#define __TRACE_WRITER_BASE_H__

#include <stdint.h>
#include <string>
#include "TraceReaderBase.h"

namespace Trace {

  /*! @class TraceWriterBase
   *
   *  This is the specification of the TraceWriter base class.
   *  All TraceWriter implementations inherit from this class.
   */
  class TraceWriterBase {

  public:

    /*!
     *  This method constructs a TraceWriterBase instance.
     */
    TraceWriterBase();

    /*!
     *  This method destructs a TraceWriterBase instance.
     */
    virtual ~TraceWriterBase();

    /*!
     *  This method writes the specified @a trace file.
     *
     *  @param[in] file specifies the name of the file to write
     *  @param[in] log structure where the trace data was read into
     *  @param[in] verbose specifies whether to be verbose with output
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
     virtual bool writeFile(
       const std::string&         file,
       Trace::TraceReaderBase    *log,
       bool                       verbose
     ) =  0;

    /*!
     * This member variable points to the target's info
     */
    std::shared_ptr<Target::TargetBase> targetInfo_m;
  };

}
#endif
