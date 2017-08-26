/*! @file TraceReaderLogQEMU.h
 *  @brief TraceReaderLogQEMU Specification
 *
 *  This file contains the specification of the TraceReaderLogQEMU class.
 */

#ifndef __TRACE_READER_LOG_QEMU_H__
#define __TRACE_READER_LOG_QEMU_H__

#include "TraceReaderBase.h"

#include "rld-process.h"

namespace Trace {

  /*! @class TraceReaderLogQEMU
   *
   *  This is the specification of the TraceReader base class.
   *  All TraceReader implementations inherit from this class.
   */
  class TraceReaderLogQEMU: public TraceReaderBase {

  public:


    /*! 
     *  This method constructs a TraceReaderLogQEMU instance.
     */
    TraceReaderLogQEMU();

    /*! 
     *  This method destructs a TraceReaderLogQEMU instance.
     */
    virtual ~TraceReaderLogQEMU();

    /*!
     *  This method processes the coverage information from the input
     *  @a file and adds it to the specified @a executableInformation.
     *
     *  @param[in] file is the coverage file to process
     *  @param[in] executableInformation is the information for an
     *             associated executable
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    virtual bool processFile(
      const char* const     file
    );
  };

}
#endif
