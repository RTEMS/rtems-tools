/*
 *  TODO: use strings instead of cstrings for reliability and saving memory
 *  TODO: use global buffers
 *
 */

/*! @file GcovData.cc
 *  @brief GcovData Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  reading *.gcno and writing *.gcda files for gcov support
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <sys/stat.h>

//#include "app_common.h"
#include "GcovData.h"
//#include "ExecutableInfo.h"
//#include "CoverageMap.h"
//#include "qemu-traces.h"


namespace Gcov {

  GcovData::GcovData( Coverage::DesiredSymbols& symbolsToAnalyze ):
    numberOfFunctions( 0 ),
    symbolsToAnalyze_m( symbolsToAnalyze )
  {
  }

  GcovData::~GcovData()
  {
  }

  bool GcovData::readGcnoFile( const char* const  fileName )
  {
    int        status;
    FILE*      gcovFile;
    char*      tempString;
    char*      tempString2;
    char*      tempString3;

    if ( strlen(fileName) >= FILE_NAME_LENGTH ){
      fprintf(
        stderr,
        "ERROR: File name is too long to be correctly stored: %u\n",
        (unsigned int) strlen(fileName)
      );
      return false;
    }
    strcpy( gcnoFileName, fileName );
    strcpy( gcdaFileName, fileName );
    strcpy( textFileName, fileName );
    strcpy( cFileName, fileName );
    tempString = strstr( gcdaFileName,".gcno" );
    tempString2 = strstr( textFileName,".gcno" );
    tempString3 = strstr( cFileName,".gcno" );

    if ( (tempString == NULL) && (tempString2 == NULL) ){
      fprintf(stderr, "ERROR: incorrect name of *.gcno file\n");
    }
    else
    {
      strcpy( tempString, ".gcda");             // construct gcda file name
      strcpy( tempString2, ".txt");             // construct report file name
      strcpy( tempString3, ".c");               // construct source file name
    }

    // Debug message
    // fprintf( stderr, "Readning file: %s\n",  gcnoFileName);

    // Open the notes file.
    gcovFile = fopen( gcnoFileName, "r" );
    if ( !gcovFile ) {
      fprintf( stderr, "Unable to open %s\n", gcnoFileName );
      return false;
    }

    // Read and validate the gcnoPreamble (magic, version, timestamp) from the file
    status = readFilePreamble( &gcnoPreamble, gcovFile, GCNO_MAGIC );
    if ( status <= 0 ){
      fprintf( stderr, "Unable to read %s\n", gcnoFileName );
      fclose( gcovFile );
      return false;
    }

    //Read all remaining frames from file
    while( readFrame(gcovFile) ){}

    fclose( gcovFile );
    return true;
  }


  bool GcovData::writeGcdaFile ()
  {
    gcov_preamble         preamble;
    gcov_frame_header     header;
    FILE*                 gcdaFile;
    functions_iterator_t  currentFunction;
    arcs_iterator_t       currentArc;
    uint32_t              buffer;
    uint32_t              countersFound;
    uint32_t              countersFoundSum;
    uint64_t              countersSum;
    uint64_t              countersMax;
    uint64_t              llBuffer[4096];    // TODO: Use common buffer
    gcov_statistics       objectStats;
    gcov_statistics       programStats;
    size_t                status;

    // Debug message
    // fprintf( stderr, "Writing file: %s\n",  gcdaFileName);

    // Lets clear counters sumators
    countersSum     = 0;
    countersMax     = 0;
    countersFoundSum   = 0;

    // Open the data file.
    gcdaFile = fopen( gcdaFileName, "w" );
    if ( !gcdaFile ) {
      fprintf( stderr, "Unable to create %s\n", gcdaFileName );
      return false;
    }

    //Form preamble
    preamble.magic  = GCDA_MAGIC;
    preamble.version  = gcnoPreamble.version;
    preamble.timestamp  = gcnoPreamble.timestamp;

    //Write preamble
    status = fwrite (&preamble , sizeof( preamble ), 1 , gcdaFile );
    if ( status != 1 )
      fprintf( stderr, "Error while writing gcda preamble to a file %s\n", gcdaFileName );

    //Write function info and counter counts
    for (
      currentFunction = functions.begin();
      currentFunction != functions.end();
      currentFunction++
    )
    {
      //Write function announcement frame header (length always equals 2)
      header.tag = GCOV_TAG_FUNCTION;
      header.length = 2;
      status = fwrite (&header, sizeof(header), 1, gcdaFile );
      if ( status != 1 )
        fprintf( stderr, "Error while writing function announcement to a file %s\n", gcdaFileName );

      //Write function id
      buffer = (*currentFunction).getId();
      status = fwrite (&buffer, sizeof( buffer ), 1, gcdaFile );
      if ( status != 1 )
        fprintf( stderr, "Error while writing function id to a file %s\n", gcdaFileName );

      //Write function checksum
      buffer = (*currentFunction).getChecksum();
      status = fwrite (&buffer, sizeof( buffer ), 1, gcdaFile );
      if ( status != 1 )
        fprintf( stderr, "Error while writing function checksum to a file %s\n", gcdaFileName );

      // Determine how many counters there are
      // and store their counts in buffer
      countersFound = 0;
      (*currentFunction).getCounters( llBuffer, countersFound, countersSum, countersMax );
      countersFoundSum += countersFound;

      //Write info about counters
      header.tag = GCOV_TAG_COUNTER;
      header.length = countersFound * 2;
      status = fwrite (&header, sizeof( header ), 1, gcdaFile );
      if ( status != 1 )
        fprintf( stderr, "Error while writing counter header to a file %s\n", gcdaFileName );

      status = fwrite (llBuffer, sizeof( uint64_t ), countersFound , gcdaFile );
      if ( status != countersFound )
        fprintf( stderr, "Error while writing counter data to a file %s\n", gcdaFileName );
    }

    // Prepare frame with object file statistics
    header.tag = GCOV_TAG_OBJECT_SUMMARY;
    header.length = 9;
    objectStats.checksum = 0;      // TODO: have no idea hov to calculates it :)
    objectStats.counters = countersFoundSum;
    objectStats.runs = 1;        // We are lying for now, we have no means of figuring this out
    objectStats.sum = countersSum;    // Sum of all counters
    objectStats.max = countersMax;    // max value for counter on last run, we have no clue
    objectStats.sumMax = countersMax;    // we have no clue

    // Write data
    status = fwrite (&header, sizeof( header ), 1, gcdaFile );
    if ( status != 1 )
      fprintf( stderr, "Error while writing stats header to a file %s\n", gcdaFileName );
    status = fwrite (&objectStats, sizeof( objectStats ), 1, gcdaFile );
    if ( status != 1 )
      fprintf( stderr, "Error while writing object stats to a file %s\n", gcdaFileName );


    // Prepare frame with program statistics
    header.tag = GCOV_TAG_PROGRAM_SUMMARY;
    header.length = 9;
    programStats.checksum = 0;      // TODO: have no idea hov to calculate it :)
    programStats.counters = countersFoundSum;
    programStats.runs = 1;      // We are lying for now, we have no clue
    programStats.sum = countersSum;    // Sum of all counters
    programStats.max = countersMax;    // max value for counter on last run, we have no clue
    programStats.sumMax = countersMax;    // we have no clue

    // Write data
    status = fwrite (&header, sizeof( header ), 1, gcdaFile );
    if ( status != 1 )
      fprintf( stderr, "Error while writing stats header to a file %s\n", gcdaFileName );
    status = fwrite (&programStats, sizeof( programStats ), 1, gcdaFile );
    if ( status != 1 )
      fprintf( stderr, "Error while writing program stats to a file %s\n", gcdaFileName );

    fclose( gcdaFile );

    return true;
  }

  bool GcovData::readFrame(
         FILE*         gcovFile
  )
  {
    gcov_frame_header   header;
    char                buffer[512];
    uint32_t            intBuffer[4096];
    uint32_t            tempBlockId;
    blocks_iterator_t   tempBlockIterator;
    int                 status;

    status = readFrameHeader( &header, gcovFile);

    if ( status <= 0 ) {
      // Not printing error message because this
      // happenns at the end of each file
      return false;
    }

    switch (header.tag){

      case GCOV_TAG_FUNCTION:

        {
          numberOfFunctions++;
          GcovFunctionData newFunction;

          if ( !readFunctionFrame(header, gcovFile, &newFunction) ){
            fprintf( stderr, "Error while reading FUNCTION from gcov file...\n" );
            return false;
          }

          functions.push_back(newFunction);
        }

        break;

      case GCOV_TAG_BLOCKS:

        status = fread( &intBuffer, 4, header.length, gcovFile );
        if ( status != (int) header.length){
          fprintf(
            stderr, "Error while reading BLOCKS from gcov file...\n"
            "Header lenght is %u instead of %u\n",
            header.length,
            status
          );
          return false;
        }

        for( uint32_t i = 0; i < header.length; i++ )
          functions.back().addBlock(i, intBuffer[i], "");

        break;

      case GCOV_TAG_ARCS:

        status = fread( &intBuffer, 4, header.length, gcovFile );
        if (status != (int) header.length){
          return false;
        }

        for ( int i = 1; i < (int) header.length; i += 2 )
          functions.back().addArc(intBuffer[0], intBuffer[i], intBuffer[i+1]);

        break;

      case GCOV_TAG_LINES:

        status = fread( &intBuffer, 4, 2, gcovFile );
        if (status != 2 || intBuffer[1] != 0){
          fprintf(
            stderr,
            "Error while reading block id for LINES from gcov file..."
          );
          return false;
        }
        tempBlockId = intBuffer[0];
        header.length -= 2;

        // Find the right block
        tempBlockIterator =functions.back().findBlockById(tempBlockId);

        header.length -= readString(buffer, gcovFile);
        functions.back().setBlockFileName( tempBlockIterator, buffer );

        status = fread( &intBuffer, 4, header.length, gcovFile );
        if (status != (int) header.length){
          fprintf( stderr, "Error while reading LINES from gcov file..." );
          return false;
        }

        else
          for (int i = 0; i < (int) (header.length - 2); i++)
            functions.back().addBlockLine( tempBlockIterator, intBuffer[i] );

        break;

      default:

        fprintf( stderr, "\n\nERROR - encountered unknown *.gcno tag : 0x%x\n", header.tag );
        break;
      }

      return true;
  }

  int GcovData::readString(
    char*     buffer,   //TODO: use global buffer here
    FILE*     gcovFile
  )
  {
    int          status;
    int          length;

    status = fread( &length, sizeof(int), 1, gcovFile );
    if (status != 1){
      fprintf( stderr, "ERROR: Unable to read string length from gcov file\n" );
      return -1;
    }

    status = fread( buffer, length * 4 , 1, gcovFile );
    if (status != 1){
      fprintf( stderr, "ERROR: Unable to read string from gcov file\n" );
      return -1;
    }

    buffer[length * 4] = '\0';

    return length +1;
  }

  int GcovData::readFrameHeader(
    gcov_frame_header*   header,
    FILE*             gcovFile
  )
  {
    int          status;
    int          length;

    length = sizeof(gcov_frame_header);
    status = fread( header, length, 1, gcovFile );
    if (status != 1){
      //fprintf( stderr, "ERROR: Unable to read frame header from gcov file\n" );
      return -1;
    }

      return length / 4;
  }

  int GcovData::readFilePreamble(
     gcov_preamble*       preamble,
     FILE*             gcovFile,
     uint32_t        desiredMagic
  )
  {
    int          status;
    int          length;

    length = sizeof( gcov_preamble );
    status = fread( preamble, sizeof( gcov_preamble), 1, gcovFile );
    if (status <= 0) {
      fprintf( stderr, "Error while reading file preamble\n" );
      return -1;
    }

    if ( preamble->magic != GCNO_MAGIC ) {
      fprintf( stderr, "File is not a valid *.gcno output (magic: 0x%4x)\n", preamble->magic );
      return -1;
    }

    return length / 4;
  }

  bool GcovData::readFunctionFrame(
    gcov_frame_header   header,
    FILE*           gcovFile,
    GcovFunctionData*  function
  )
  {
    char         buffer[512];    //TODO: use common buffers
    uint32_t     intBuffer[4096];
    int          status;

    status = fread( &intBuffer, 8, 1, gcovFile );
    if (status != 1){
      fprintf( stderr, "ERROR: Unable to read Function ID & checksum\n" );
      return false;
    }
    header.length -= 2;
    function->setId( intBuffer[0] );
    function->setChecksum( intBuffer[1] );

    header.length -= readString( buffer, gcovFile );
    function->setFunctionName( buffer, symbolsToAnalyze_m );
    header.length -= readString( buffer, gcovFile );
    function->setFileName( buffer );
    status = fread( &intBuffer, 4, header.length, gcovFile );
    if (status <= 0){
    fprintf( stderr, "ERROR: Unable to read Function starting line number\n" );
      return false;
    }
    function->setFirstLineNumber( intBuffer[0] );

    return true;
  }

  bool GcovData::writeReportFile()
  {
    functions_iterator_t   currentFunction;
    uint32_t               i = 1;          //iterator
    FILE*                  textFile;

    // Debug message
    // fprintf( stderr, "Writing file: %s\n",  textFileName);

    // Open the data file.
    textFile = fopen( textFileName, "w" );
    if ( !textFile ) {
      fprintf( stderr, "Unable to create %s\n", textFileName );
      return false;
    }

    printGcnoFileInfo( textFile );

    for (
      currentFunction = functions.begin();
      currentFunction != functions.end();
      currentFunction++
    )
    {
      (*currentFunction).printFunctionInfo( textFile, i );
      (*currentFunction).printCoverageInfo( textFile, i );
      i++;
    }

    fclose ( textFile );
    return true;
  }

  void GcovData::printGcnoFileInfo( FILE * textFile )
  {
    fprintf(
      textFile,
      "\nFILE:\t\t\t%s\n"
      "magic:\t\t\t%x\n"
      "version:\t\t%x\n"
      "timestamp:\t\t%x\n"
      "functions found: \t%u\n\n",
      gcnoFileName,
      gcnoPreamble.magic,
      gcnoPreamble.version,
      gcnoPreamble.timestamp,
      numberOfFunctions
    );
  }

  void GcovData::writeGcovFile( )
  {
    //std::cerr << "Attempting to run gcov for: " << cFileName << std::endl;
    std::ostringstream command;
    command << "( cd " << rld::path::dirname (cFileName)
            << " && gcov " << rld::path::basename (cFileName)
            << " &>> gcov.log)";
    //std::cerr << "> " << command << std::endl;
    system( command.str ().c_str () );
  }

  bool GcovData::processCounters(  )
  {
    functions_iterator_t  currentFunction;
    bool                  status = true;

    for (
      currentFunction = functions.begin();
      currentFunction != functions.end();
      currentFunction++
    )
    {
      if ( !(*currentFunction).processFunctionCounters(  ) )
        status = false;
    }

    return status;
  }
}
