/*! @file GcovData.h
 *  @brief GcovData Specification
 *
 *  This file contains the specification of the GcovGcnoWriter class.
 */

#ifndef __GCOV_DATA_H__
#define __GCOV_DATA_H__

#include <stdint.h>
#include <list>
#include <iostream>
#include "GcovFunctionData.h"
#include "DesiredSymbols.h"

namespace Gcov {

#define GCDA_MAGIC ((uint32_t) 0x67636461 )     /* "gcda" */
#define GCNO_MAGIC ((uint32_t) 0x67636e6f )     /* "gcno" */

/* we are using gcc 4.6 release format, coded as "406R" */
#define GCNO_VERSION ((uint32_t) 0x34303652 )

/* GCOV tags */
#define GCOV_TAG_FUNCTION               ((uint32_t)0x01000000)
#define GCOV_TAG_BLOCKS                 ((uint32_t)0x01410000)
#define GCOV_TAG_ARCS                   ((uint32_t)0x01430000)
#define GCOV_TAG_LINES                  ((uint32_t)0x01450000)
#define GCOV_TAG_COUNTER                ((uint32_t)0x01a10000)
#define GCOV_TAG_OBJECT_SUMMARY         ((uint32_t)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY        ((uint32_t)0xa3000000)


typedef std::list<Gcov::GcovFunctionData>              functions_t;
typedef std::list<Gcov::GcovFunctionData>::iterator    functions_iterator_t;

struct gcov_preamble
{
  uint32_t magic;
  uint32_t version;
  uint32_t timestamp;
};

struct gcov_frame_header
{
  uint32_t tag;
  uint32_t length;
};

struct gcov_statistics
{
  uint32_t checksum;          // checksum
  uint32_t counters;          // number of counters
  uint32_t runs;              // number of runs
  uint64_t sum;               // sum of all couter values
  uint64_t max;               // max value on a single run
  uint64_t sumMax;            // sum of individual runs max values
};

class DesiredSymbols;

  /*! @class GcovData
   *
   *  This is the specification of the GcovData class.
   */
  class GcovData {

  public:

    /*!
     *  This method constructs a GcnoReader instance.
     *
     *  @param[in] symbolsToAnalyze the symbols to be analyzed
     */
    GcovData( Coverage::DesiredSymbols& symbolsToAnalyze );

    /*!
     *  This method destructs a GcnoReader instance.
     */
    virtual ~GcovData();

    /*!
     *  This method reads the *.gcno file
     *
     *  @param[in] fileName name of the file to read
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    bool readGcnoFile( const char* const  fileName );

    /*!
     *  This method writes the *.gcda file. It also produces and stores
     *  gcda and txt file names for future outputs.
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    bool writeGcdaFile ();

    /*!
     *  This method writes all contained information to stdout file
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    bool writeReportFile();

    /*!
     *  This method runs gcov to generate report. This method should
     *  be used only when gcno and gcda files are already generated.
     */
    void writeGcovFile( );

    /*!
     *  This method calculates values of counters for all functions
     */
    bool processCounters( void );

  private:

    uint32_t                            numberOfFunctions;
    gcov_preamble                       gcnoPreamble;
    char                                gcnoFileName[FILE_NAME_LENGTH];
    char                                gcdaFileName[FILE_NAME_LENGTH];
    char                                textFileName[FILE_NAME_LENGTH];
    char                                cFileName[FILE_NAME_LENGTH];
    functions_t                         functions;


    /*!
     *  This method reads a frame from *.gcno file
     *
     *  @param[in] file points to a file to read
     *
     *  @return true if read was succesfull, false otherwise
     */
    bool readFrame(
      FILE*       gcovFile
    );

    /*!
     *  This method reads a string from gcov file
     *
     *  @param[in] buffer stores the string
     *  @param[in] file specifies the name of the file to read
     *
     *  @return Returns length of words read (word = 32bit) or -1 if error ocurred
     */
    int readString(
      char*       buffer,
      FILE*       gcovFile
    );

    /*!
     *  This method reads a frame header from gcov file
     *
     *  @param[in] header stores the header
     *  @param[in] file specifies the name of the file to read
     *
     *  @return Returns length of words read (word = 32bit)
     *  or -1 if error ocurred
     */
    int readFrameHeader(
      gcov_frame_header*  header,
      FILE*               gcovFile
    );

    /*!
     *  This method reads a frame header from gcov file
     *
     *  @param[in] preamble stores the preamble
     *  @param[in] gcovFile specifies the name of the file to read
     *  @param[in] desiredMagic stores the expected magic of a file
     *
     *  @return Returns length of words read (word = 32bit)
     *          or -1 if error ocurred
     */
    int readFilePreamble(
      gcov_preamble*      preamble,
      FILE*               gcovFile,
      const uint32_t      desiredMagic
    );

    /*!
     *  This method reads a function frame from gcov file
     *
     *  @param[in] header passes frame header
     *  @param[in] gcovFile specifies the name of the file to read
     *  @param[in] function stores the expected magic of a file
     *
     *  @return Returns true if operation was succesfull
     */
    bool readFunctionFrame(
      gcov_frame_header   header,
      FILE*               gcovFile,
      GcovFunctionData*   function
    );

    /*!
     *  This method prints info about previously read *.gcno file
     *  to a specified report file
     */
    void printGcnoFileInfo( FILE * textFile );

    /*!
     * This member variable contains the symbols to be analyzed
     */
    Coverage::DesiredSymbols& symbolsToAnalyze_m;
  };
}
#endif
