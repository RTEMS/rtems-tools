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

#include "rtems-utils.h"

namespace Gcov {

  GcovData::GcovData( Coverage::DesiredSymbols& symbolsToAnalyze ):
    numberOfFunctions( 0 ),
    symbolsToAnalyze_m( symbolsToAnalyze )
  {
  }

  GcovData::~GcovData()
  {
  }

  bool GcovData::readGcnoFile( const std::string& fileName )
  {
    int           status;
    std::ifstream gcovFile;
    std::string   tempString;
    std::string   tempString2;
    std::string   tempString3;
    size_t        index;

    if ( fileName.length() >= FILE_NAME_LENGTH ) {
      std::cerr << "ERROR: File name is too long to be correctly stored: "
                << fileName.length() << std::endl;
      return false;
    }

    gcnoFileName = fileName;
    gcdaFileName = fileName;
    textFileName = fileName;
    cFileName    = fileName;
    tempString   = gcdaFileName;
    tempString2  = textFileName;
    tempString3  = cFileName;

    index = tempString.find( ".gcno" );
    if ( index == std::string::npos ) {
      std::cerr << "ERROR: incorrect name of *.gcno file" << std::endl;
      return false;
    } else {
      // construct gcda file name
      tempString = tempString.replace( index, strlen( ".gcno" ), ".gcda" );

      // construct report file name
      tempString2 = tempString2.replace( index, strlen( ".gcno" ), ".txt" );

      // construct source file name
      tempString3 = tempString3.replace( index, strlen( ".gcno" ), ".c" );
    }

    // Debug message
    // std::cerr << "Reading file: " << gcnoFileName << std::endl;

    // Open the notes file.
    gcovFile.open( gcnoFileName );
    if ( !gcovFile ) {
      std::cerr << "Unable to open " << gcnoFileName << std::endl;
      return false;
    }

    // Read and validate the gcnoPreamble (magic, version, timestamp) from the file
    status = readFilePreamble( &gcnoPreamble, gcovFile, GCNO_MAGIC );
    if ( status <= 0 ) {
      std::cerr << "Unable to read " << gcnoFileName << std::endl;
      return false;
    }

    //Read all remaining frames from file
    while( readFrame( gcovFile ) ) {}

    return true;
  }


  bool GcovData::writeGcdaFile()
  {
    gcov_preamble         preamble;
    gcov_frame_header     header;
    std::ofstream         gcdaFile;
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
    long int              bytes_before;

    // Debug message
    //std::cerr << "Writing file: " <<  gcdaFileName << std::endl;

    // Lets clear counters sumators
    countersSum      = 0;
    countersMax      = 0;
    countersFoundSum = 0;

    // Open the data file.
    gcdaFile.open( gcdaFileName );
    if ( !gcdaFile ) {
      std::cerr << "Unable to create " << gcdaFileName << std::endl;
      return false;
    }

    //Form preamble
    preamble.magic     = GCDA_MAGIC;
    preamble.version   = gcnoPreamble.version;
    preamble.timestamp = gcnoPreamble.timestamp;

    //Write preamble
    gcdaFile.write( (char *) &preamble , sizeof( preamble ) );
    if ( gcdaFile.fail() ) {
      std::cerr << "Error while writing gcda preamble to a file "
                << gcdaFileName << std::endl;
    }

    //Write function info and counter counts
    for (
      currentFunction = functions.begin();
      currentFunction != functions.end();
      currentFunction++
    ) {
      //Write function announcement frame header (length always equals 2)
      header.tag = GCOV_TAG_FUNCTION;
      header.length = 2;
      gcdaFile.write( (char *) &header, sizeof( header ) );
      if ( gcdaFile.fail() ) {
        std::cerr << "Error while writing function announcement to a file "
                  << gcdaFileName << std::endl;
      }

      //Write function id
      buffer = (*currentFunction).getId();
      gcdaFile.write( (char *) &buffer, sizeof( buffer ) );
      if ( gcdaFile.fail() ) {
        std::cerr << "Error while writing function id to a file "
                  << gcdaFileName << std::endl;
      }

      //Write function checksum
      buffer = (*currentFunction).getChecksum();
      gcdaFile.write( (char *) &buffer, sizeof( buffer ) );
      if ( gcdaFile.fail() ) {
        std::cerr << "Error while writing function checksum to a file "
                  << gcdaFileName << std::endl;
      }

      // Determine how many counters there are
      // and store their counts in buffer
      countersFound = 0;
      (*currentFunction).getCounters(
        llBuffer,
        countersFound,
        countersSum,
        countersMax
      );
      countersFoundSum += countersFound;

      //Write info about counters
      header.tag = GCOV_TAG_COUNTER;
      header.length = countersFound * 2;
      gcdaFile.write( (char *) &header, sizeof( header ) );
      if ( gcdaFile.fail() ) {
        std::cerr << "Error while writing counter header to a file "
                  << gcdaFileName << std::endl;
      }

      bytes_before = gcdaFile.tellp();

      gcdaFile.write( (char *) llBuffer, sizeof( uint64_t ) * countersFound );
      if ( gcdaFile.tellp() - bytes_before != countersFound ) {
        std::cerr << "Error while writing counter data to a file "
                  << gcdaFileName << std::endl;
      }
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
    gcdaFile.write( (char *) &header, sizeof( header ) );
    if ( gcdaFile.fail() ) {
      std::cerr << "Error while writing stats header to a file "
                << gcdaFileName << std::endl;
    }

    gcdaFile.write( (char *) &objectStats, sizeof( objectStats ) );
    if ( gcdaFile.fail() ) {
      std::cerr << "Error while writing object stats to a file "
                << gcdaFileName << std::endl;
    }


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
    gcdaFile.write( (char *) &header, sizeof( header ) );
    if ( gcdaFile.fail() ) {
      std::cerr << "Error while writing stats header to a file "
                << gcdaFileName << std::endl;
    }

    gcdaFile.write( (char *) &programStats, sizeof( programStats ) );
    if ( gcdaFile.fail() ) {
      std::cerr << "Error while writing program stats to a file "
                << gcdaFileName << std::endl;
    }

    return true;
  }

  bool GcovData::readFrame( std::ifstream& gcovFile )
  {
    gcov_frame_header header;
    char              buffer[512];
    char              intBuffer[16384];
    uint32_t          tempBlockId;
    blocks_iterator_t tempBlockIterator;
    int               status;

    status = readFrameHeader( &header, gcovFile );
    if ( status <= 0 ) {
      // Not printing error message because this
      // happenns at the end of each file
      return false;
    }

    switch ( header.tag ) {

      case GCOV_TAG_FUNCTION:

        {
          numberOfFunctions++;
          GcovFunctionData newFunction;

          if ( !readFunctionFrame(header, gcovFile, &newFunction) ) {
            std::cerr << "Error while reading FUNCTION from gcov file..."
                      << std::endl;
            return false;
          }

          functions.push_back( newFunction );
        }

        break;

      case GCOV_TAG_BLOCKS:

        gcovFile.read( intBuffer, header.length );
        if ( gcovFile.gcount() != (int) header.length ) {
          std::cerr << "Error while reading BLOCKS from gcov file..."
                    << std::endl
                    << "Header length is " << header.length
                    << " instead of " << gcovFile.gcount()
                    << std::endl;
          return false;
        }

        for( uint32_t i = 0; i < header.length; i++ ) {
          functions.back().addBlock( i, intBuffer[i], "" );
        }

        break;

      case GCOV_TAG_ARCS:

        gcovFile.read( intBuffer, header.length );
        if ( gcovFile.gcount() != (int) header.length ) {
          return false;
        }

        for ( int i = 1; i < (int) header.length; i += 2 ) {
          functions.back().addArc(intBuffer[0], intBuffer[i], intBuffer[i+1]);
        }

        break;

      case GCOV_TAG_LINES:

        gcovFile.read( intBuffer, 2 );
        if ( gcovFile.gcount() != 2 || intBuffer[1] != 0 ) {
          std::cerr << "Error while reading block id for LINES from gcov "
                    << "file..." << std::endl;
          return false;
        }
        tempBlockId = intBuffer[0];
        header.length -= 2;

        // Find the right block
        tempBlockIterator =functions.back().findBlockById( tempBlockId );

        header.length -= readString( buffer, gcovFile );
        functions.back().setBlockFileName( tempBlockIterator, buffer );

        gcovFile.read( intBuffer, header.length );
        if ( gcovFile.gcount() != (int) header.length ) {
          std::cerr << "Error while reading LINES from gcov file..."
                    << std::endl;
          return false;
        } else {
          for ( int i = 0; i < (int) (header.length - 2); i++ ) {
            functions.back().addBlockLine( tempBlockIterator, intBuffer[i] );
          }
        }

        break;

      default:

        std::cerr << std::endl << std::endl
                  << "ERROR - encountered unknown *.gcno tag : 0x"
                  << std::hex << header.tag << std::dec << std::endl;
        break;
      }

      return true;
  }

  int GcovData::readString( char* buffer, std::ifstream& gcovFile )
  {
    int length;

    gcovFile.read( (char *) &length, sizeof( int ) );
    if ( gcovFile.gcount() != sizeof( int ) ) {
      std::cerr << "ERROR: Unable to read string length from gcov file"
                << std::endl;
      return -1;
    }

    gcovFile.read( buffer, length * 4 );
    if ( gcovFile.gcount() != length * 4 ) {
      std::cerr << "ERROR: Unable to read string from gcov file"
                << std::endl;
      return -1;
    }

    buffer[length * 4] = '\0';

    return length +1;
  }

  int GcovData::readFrameHeader(
    gcov_frame_header* header,
    std::ifstream&     gcovFile
  )
  {
    int length;

    length = sizeof( gcov_frame_header );
    gcovFile.read( (char *) header, length );
    if ( gcovFile.gcount() != length ) {
      std::cerr << "ERROR: Unable to read frame header from gcov file"
                << std::endl;
      return -1;
    }

      return length / 4;
  }

  int GcovData::readFilePreamble(
     gcov_preamble* preamble,
     std::ifstream& gcovFile,
     uint32_t       desiredMagic
  )
  {
    rtems::utils::ostream_guard old_state( std::cerr );

    // Read the gcov preamble and make sure it is the right length and has the
    // magic number
    gcovFile.read( (char *) &preamble, sizeof( gcov_preamble ) );
    if ( gcovFile.gcount() != sizeof( gcov_preamble ) ) {
      std::cerr << "Error while reading file preamble" << std::endl;
      return -1;
    }

    if ( preamble->magic != GCNO_MAGIC ) {
      std::cerr << "File is not a valid *.gcno output (magic: 0x"
                << std::hex << std::setw( 4 ) << preamble->magic
                << ")" << std::endl;
      return -1;
    }

    return sizeof( gcov_preamble ) / 4;
  }

  bool GcovData::readFunctionFrame(
    gcov_frame_header header,
    std::ifstream&    gcovFile,
    GcovFunctionData* function
  )
  {
    char buffer[512];
    char intBuffer[16384];

    gcovFile.read( (char *) &intBuffer, 8 );
    if ( gcovFile.gcount() != 8 ) {
      std::cerr << "ERROR: Unable to read Function ID & checksum" << std::endl;
      return false;
    }

    header.length -= 2;
    function->setId( intBuffer[0] );
    function->setChecksum( intBuffer[1] );

    header.length -= readString( buffer, gcovFile );
    function->setFunctionName( buffer, symbolsToAnalyze_m );
    header.length -= readString( buffer, gcovFile );
    function->setFileName( buffer );
    gcovFile.read( (char*) &intBuffer, 4 * header.length );
    if (gcovFile.gcount() != 4 * header.length ) {
      std::cerr << "ERROR: Unable to read Function starting line number"
                << std::endl;
      return false;
    }

    function->setFirstLineNumber( intBuffer[0] );

    return true;
  }

  bool GcovData::writeReportFile()
  {
    functions_iterator_t currentFunction;
    uint32_t             i = 1;          //iterator
    std::ofstream        textFile;

    // Debug message
    // std::cerr << "Writing file: " << textFileName << std::endl;

    // Open the data file.
    textFile.open( textFileName );
    if ( !textFile.is_open() ) {
      std::cerr << "Unable to create " << textFileName << std::endl;
      return false;
    }

    printGcnoFileInfo( textFile );

    for (
      currentFunction = functions.begin();
      currentFunction != functions.end();
      currentFunction++
    ) {
      (*currentFunction).printFunctionInfo( textFile, i );
      (*currentFunction).printCoverageInfo( textFile, i );
      i++;
    }

    return true;
  }

  void GcovData::printGcnoFileInfo( std::ofstream& textFile )
  {
    textFile << std::endl << "FILE:\t\t\t" << gcnoFileName
             << std::endl << std::hex
             << "magic:\t\t\t" << gcnoPreamble.magic << std::endl
             << "version:\t\t" << gcnoPreamble.version << std::endl
             << "timestamp:\t\t" << gcnoPreamble.timestamp << std::endl
             << std::dec
             << "functions found: \t"
             << std::endl << std::endl << gcnoPreamble.timestamp
             << std::endl << std::endl;
  }

  void GcovData::writeGcovFile()
  {
    //std::cerr << "Attempting to run gcov for: " << cFileName << std::endl;
    std::ostringstream command;

    command << "(  cd " << rld::path::dirname( cFileName )
            << " && gcov " << rld::path::basename( cFileName )
            << " &>> gcov.log)";
    //std::cerr << "> " << command << std::endl;
    system( command.str().c_str() );
  }

  bool GcovData::processCounters()
  {
    functions_iterator_t currentFunction;
    bool                 status = true;

    for (
      currentFunction = functions.begin();
      currentFunction != functions.end();
      currentFunction++
    ) {
      if ( !(*currentFunction).processFunctionCounters() ) {
        status = false;
      }
    }

    return status;
  }
}
