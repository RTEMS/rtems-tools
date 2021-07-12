/*! @file TraceWriterQEMU.h
 *  @brief TraceWriterQEMU Specification
 *
 *  This file contains the specification of the TraceWriterQEMU class.
 */

#ifndef __TRACE_WRITER_QEMU_H__
#define __TRACE_WRITER_QEMU_H__

#include <stdint.h>
#include "TraceReaderBase.h"
#include "TraceWriterBase.h"

#include "rld-process.h"

namespace Trace {

  /*! @class TraceWriterQEMU
   *
   *  This is the specification of the TraceWriter base class.
   *  All TraceWriter implementations inherit from this class.
   */
  class TraceWriterQEMU: public TraceWriterBase {

  public:

    /*!
     *  This method constructs a TraceWriterQEMU instance.
     */
    TraceWriterQEMU();

    /*!
     *  This method destructs a TraceWriterQEMU instance.
     */
    virtual ~TraceWriterQEMU();

    /*!
     *  This method writes the specified @a trace file.
     *
     *  @param[in] file specifies the name of the file to write
     *  @param[in] log structure where the trace data was read into
     *  @param[in] verbose specifies whether to be verbose with output
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
     bool writeFile(
       const std::string&         file,
       Trace::TraceReaderBase    *log,
       bool                       verbose
     );
  };

}
#endif
