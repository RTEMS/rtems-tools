/*! @file TraceReaderBase.h
 *  @brief TraceReaderBase Specification
 *
 *  This file contains the specification of the TraceReaderBase class.
 */

#ifndef __TRACE_READER_BASE_H__
#define __TRACE_READER_BASE_H__

#include "TraceList.h"
#include "ObjdumpProcessor.h"

namespace Trace {

  /*! @class TraceReaderBase
   *
   *  This is the specification of the TraceReader base class.
   *  All TraceReader implementations inherit from this class.
   */
  class TraceReaderBase {

  public:

    /*!
     *  Trace list that is filled by processFile.
     */
    TraceList Trace;


    /*!
     *  This method constructs a TraceReaderBase instance.
     */
    TraceReaderBase();

    /*!
     *  This method destructs a TraceReaderBase instance.
     */
    virtual ~TraceReaderBase();

    /*!
     *  This method processes the coverage information from the input
     *  @a file and adds it to the specified @a executableInformation.
     *
     *  @param[in] file is the coverage file to process
     *  @param[in] objdumpProcessor the processor for the object dump
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    virtual bool processFile(
      const char* const           file,
      Coverage::ObjdumpProcessor& objdumpProcessor
    ) = 0;
  };

}
#endif
