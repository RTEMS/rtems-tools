/*! @file GcovFunctionData.cc
 *  @brief GcovFunctionData Implementation
 *
 *  This file contains the implementation of the class storing information
 *  about single function.
 */

#include <cstdio>
#include <cstring>
#include <cinttypes>

#include "app_common.h"
#include "GcovFunctionData.h"
#include "ObjdumpProcessor.h"
#include "CoverageMapBase.h"
#include "DesiredSymbols.h"


namespace Gcov {

  GcovFunctionData::GcovFunctionData()
  {
    numberOfArcs = 0;
    numberOfBlocks = 0;
    coverageMap = NULL;
  }

  GcovFunctionData::~GcovFunctionData()
  {
  }

  void GcovFunctionData::setChecksum( const uint32_t chk )
  {
    checksum = chk;
  }

  void GcovFunctionData::setId( const uint32_t idNumber )
  {
    id = idNumber;
  }


  void GcovFunctionData::setFirstLineNumber( const uint32_t lineNo )
  {
    firstLineNumber = lineNo;
  }

  bool GcovFunctionData::setFunctionName(
    const char*               fcnName,
    Coverage::DesiredSymbols& symbolsToAnalyze
  )
  {
    std::string   symbolName;

    symbolName = fcnName;

    if ( strlen(fcnName) >= FUNCTION_NAME_LENGTH ) {
      fprintf(
        stderr,
        "ERROR: Function name is too long to be correctly stored: %u\n",
        (unsigned int) strlen(fcnName)
      );
      return false;
    }

    strcpy (functionName, fcnName);

    // Tie function to its coverage map
    symbolInfo = symbolsToAnalyze.find( symbolName );
    if ( symbolInfo != NULL )
      coverageMap = symbolInfo->unifiedCoverageMap;

#if 0
    if ( coverageMap == NULL) {
      fprintf(
        stderr,
        "ERROR: Could not find coverage map for: %s\n",
        symbolName.c_str()
      );
    } else {
      fprintf(
        stderr,
        "SUCCESS: Hound coverage map for: %s\n",
        symbolName.c_str()
      );
   }
#endif

    return true;
  }

  bool GcovFunctionData::setFileName( const char* fileName ) {
    if ( strlen(fileName) >= FILE_NAME_LENGTH ){
      fprintf(
        stderr,
        "ERROR: File name is too long to be correctly stored: %u\n",
        (unsigned int) strlen(fileName)
      );
      return false;
    }
    strcpy (sourceFileName, fileName);
    return true;
  }

  arcs_t GcovFunctionData::getArcs() const
  {
    return arcs;
  }

  uint32_t GcovFunctionData::getChecksum() const
  {
    return checksum;
  }

  uint32_t GcovFunctionData::getId() const
  {
    return id;
  }

  void GcovFunctionData::getCounters(
    uint64_t* counterValues,
    uint32_t &countersFound,
    uint64_t &countersSum,
    uint64_t &countersMax
  )
  {
    arcs_iterator_t  currentArc;
    int              i;

    countersFound = 0;
    countersSum   = 0;
    countersMax   = 0;

    // Locate relevant counters and copy their values
    i = 0;
    for(
      currentArc = arcs.begin();
      currentArc != arcs.end();
      currentArc++
    )
    {
      if ( currentArc->flags == 0 || currentArc->flags == 2 ||
           currentArc->flags == 4 ) {
        countersFound++;
        countersSum += currentArc->counter;
        counterValues[i] = currentArc->counter;
        if ( countersMax <= currentArc->counter)
          countersMax = currentArc->counter;
        i++;
      }
    }
  }

  blocks_t GcovFunctionData::getBlocks() const
  {
    return blocks;
  }

  void GcovFunctionData::addArc(
    uint32_t  source,
    uint32_t  destination,
    uint32_t  flags
  )
  {
    gcov_arc_info arc;

    numberOfArcs++;
    arc.sourceBlock = source;
    arc.destinationBlock = destination;
    arc.flags = flags;
    arc.counter = 0;
    arcs.push_back(arc);
  }

  void GcovFunctionData::addBlock(
    const uint32_t  id,
    const uint32_t  flags,
    const char *    sourceFileName
  )
  {
    gcov_block_info block;

    numberOfBlocks++;
    block.id    = id;
    block.flags = flags;
    block.numberOfLines = 0;
    block.counter = 0;
    strcpy (block.sourceFileName, sourceFileName);
    blocks.push_back(block);
  }

  void GcovFunctionData::printFunctionInfo(
    FILE * textFile,
    uint32_t function_number
  )
  {
    blocks_iterator_t  currentBlock;
    arcs_iterator_t    currentArc;

    fprintf(
      textFile,
      "\n\n=========================="
      "FUNCTION %3d "
      "==========================\n\n",
      function_number
    );
    fprintf(
      textFile,
      "Name:      %s\n"
      "File:      %s\n"
      "Line:      %u\n"
      "Id:        %u\n"
      "Checksum:  0x%x\n\n",
      functionName,
      sourceFileName,
      firstLineNumber,
      id,
      checksum
    );

    // Print arcs info
    for ( currentArc = arcs.begin(); currentArc != arcs.end(); currentArc++ ) {
      printArcInfo( textFile, currentArc );
    }
    fprintf( textFile, "\n");

    // Print blocks info
    for ( currentBlock = blocks.begin();
          currentBlock != blocks.end();
          currentBlock++
    ) {
      printBlockInfo( textFile, currentBlock );
    }
  }

  void GcovFunctionData::printCoverageInfo(
    FILE     *textFile,
    uint32_t  function_number
  )
  {
    uint32_t        baseAddress = 0;
    uint32_t        baseSize;
    uint32_t        currentAddress;
    std::list<Coverage::objdumpLine_t>::iterator   instruction;

    if ( coverageMap != NULL ) {

      for (instruction = symbolInfo->instructions.begin();
           instruction != symbolInfo->instructions.end();
           instruction++) {
        if( instruction->isInstruction ) {
          baseAddress = instruction->address;
          break;
        }
      }
      baseSize   = coverageMap->getSize();

      fprintf(
        textFile,
        "\nInstructions (Base address: 0x%08x, Size: %4u): \n\n",
        baseAddress,
        baseSize
      );
      for ( instruction = symbolInfo->instructions.begin();
            instruction != symbolInfo->instructions.end();
            instruction++
      )
      {
        if ( instruction->isInstruction ) {
          currentAddress = instruction->address - baseAddress;
          fprintf( textFile, "0x%-70s ", instruction->line.c_str() );
          fprintf( textFile, "| 0x%08x ",   currentAddress );
          fprintf( textFile, "*");
          fprintf( textFile,
                    "| exec: %4u ",
                    coverageMap->getWasExecuted( currentAddress )
          );
          fprintf( textFile, "| taken/not: %4u/%4u ",
                    coverageMap->getWasTaken( currentAddress ),
                    coverageMap->getWasNotTaken( currentAddress )
          );

          if ( instruction->isBranch )
            fprintf( textFile, "| Branch " );
          else
            fprintf( textFile, "         " );

          if ( instruction->isNop )
            fprintf( textFile, "| NOP(%3u) \n", instruction->nopSize );
          else
            fprintf( textFile, "           \n" );
        }
      }
    }
  }

  void GcovFunctionData::setBlockFileName(
    const blocks_iterator_t  block,
    const char               *fileName
  )
  {
    strcpy(block->sourceFileName, fileName);
  }

  void GcovFunctionData::addBlockLine(
    const blocks_iterator_t  block,
    const uint32_t           line
  )
  {
    block->lines.push_back(line);
    (block->numberOfLines)++;
  }

  blocks_iterator_t GcovFunctionData::findBlockById(
    const uint32_t    id
  )
  {
    blocks_iterator_t blockIterator;

    if ( !blocks.empty() ) {
      blockIterator = blocks.begin();
      while ( blockIterator != blocks.end() ){
        if ( blockIterator->id ==  id)
          break;
        blockIterator++;
      }
    } else {
      fprintf(
        stderr,
        "ERROR: GcovFunctionData::findBlockById() failed, no blocks present\n"
      );
    }
    return blockIterator;
  }

  void GcovFunctionData::printArcInfo(
                FILE * textFile, arcs_iterator_t arc
  )
  {
    fprintf(
      textFile,
      " > ARC %3u -> %3u ",
      arc->sourceBlock,
      arc->destinationBlock
    );

    fprintf( textFile, "\tFLAGS: ");
    switch ( arc->flags ){
      case 0:
        fprintf( textFile, "( ___________ ____ _______ )");
        break;
      case 1:
        fprintf( textFile, "( ___________ ____ ON_TREE )");
        break;
      case 2:
        fprintf( textFile, "( ___________ FAKE _______ )");
        break;
      case 3:
        fprintf( textFile, "( ___________ FAKE ON_TREE )");
        break;
      case 4:
        fprintf( textFile, "( FALLTHROUGH ____ _______ )");
        break;
      case 5:
        fprintf( textFile, "( FALLTHROUGH ____ ON_TREE )");
        break;
      default:
        fprintf( textFile, "( =======FLAGS_ERROR====== )");
        fprintf( stderr,
                " ERROR: Unknown arc flag: 0x%x\n",
                arcs.back().flags
        );
        break;
    }
    fprintf( textFile, "\tTaken: %5" PRIu64 "\n", (uint64_t) arc->counter );
  }

  void GcovFunctionData::printBlockInfo(
    FILE * textFile,
    blocks_iterator_t block
  )
  {
    std::list<uint32_t>::iterator  line;

    fprintf(
      textFile,
      " > BLOCK %3u from %s\n"
      "    -counter: %5" PRIu64 "\n"
      "    -flags: 0x%" PRIx32 "\n"
      "    -lines: ",
      block->id,
      block->sourceFileName,
      (uint64_t) block->counter,
      block->flags
    );
    if ( !block->lines.empty( ) )
      for ( line = block->lines.begin() ; line != block->lines.end(); line++ )
        fprintf ( textFile, "%u, ", *line);
    fprintf ( textFile, "\n");
  }

  bool GcovFunctionData::processFunctionCounters( void ) {

    uint32_t               baseAddress = 0;
    uint32_t               currentAddress = 0;
    std::list<Coverage::objdumpLine_t>::iterator  instruction;
    blocks_iterator_t      blockIterator;
    blocks_iterator_t      blockIterator2;
    arcs_iterator_t        arcIterator;
    arcs_iterator_t        arcIterator2;
    std::list<uint64_t>    taken;       // List of taken counts for branches
    std::list<uint64_t>    notTaken;    // List of not taken counts for branches

    //fprintf( stderr, "DEBUG: Processing counters for file: %s\n", sourceFileName  );
    if ( blocks.empty() || arcs.empty() || coverageMap == NULL || symbolInfo->instructions.empty())
    {
      //fprintf( stderr,
      //          "DEBUG: sanity check returned false for function: %s from file: %s\n",
      //          functionName,
      //          sourceFileName
      //);
      return false;
    }

    // Reset iterators and variables
    blockIterator = blocks.begin();
    arcIterator = arcs.begin();
    arcIterator2 = arcIterator;
    arcIterator2++;
    instruction = symbolInfo->instructions.begin();
    baseAddress = coverageMap->getFirstLowAddress();      //symbolInfo->baseAddress;
    currentAddress = baseAddress;

    // Find taken/not taken values for branches
    if ( !processBranches( &taken , &notTaken ) )
    {
      //fprintf( stderr,
      //          "ERROR: Failed to process branches for function: %s from file: %s\n",
      //          functionName,
      //          sourceFileName
      //);
      return false;
    };

    // Process the branching arcs
    while ( blockIterator != blocks.end() ) {
      //fprintf( stderr, "DEBUG: Processing branches\n" );
      while ( arcIterator->sourceBlock != blockIterator->id ) {
        if ( arcIterator == arcs.end() ) {
          //fprintf( stderr, "ERROR: Unexpectedly runned out of arcs to analyze\n" );
          return false;
        }
        arcIterator++;
        arcIterator2++;
      }

      // If no more branches break;
      if ( arcIterator2 == arcs.end() )
        break;

      // If this is a branch without FAKE arcs process it
      if (
        (arcIterator->sourceBlock == arcIterator2->sourceBlock ) &&
        !( arcIterator->flags & FAKE_ARC_FLAG ) &&
        !( arcIterator2->flags & FAKE_ARC_FLAG )
      ) {
        if ( taken.empty() || notTaken.empty() ) {
          fprintf(
            stderr,
            "ERROR: Branchess missing for function: %s from file: %s\n",
            functionName,
            sourceFileName
          );
          return false;
        }
        //fprintf( stderr, "DEBUG: Found true branching arc %3u -> %3u\n", arcIterator->sourceBlock, arcIterator->destinationBlock );
        if ( arcIterator->flags & FALLTHROUGH_ARC_FLAG ) {
          arcIterator->counter = notTaken.front();
          notTaken.pop_front();
          arcIterator2->counter = taken.front();
          taken.pop_front();
        } else {
          arcIterator2->counter = notTaken.front();
          notTaken.pop_front();
          arcIterator->counter = taken.front();
          taken.pop_front();
        }

        blockIterator2 = blocks.begin();
        //TODO: ADD FAILSAFE
        while ( arcIterator->destinationBlock != blockIterator2->id)
          blockIterator2++;
        blockIterator2->counter += arcIterator->counter;

        blockIterator2 = blocks.begin();
        //TODO: ADD FAILSAFE
        while ( arcIterator2->destinationBlock != blockIterator2->id)
            blockIterator2++;
          blockIterator2->counter += arcIterator2->counter;
      }
      blockIterator++;
    }

    // Reset iterators and variables
    blockIterator = blocks.begin();
    arcIterator = arcs.begin();
    arcIterator2 = arcIterator;
    arcIterator2++;

    // Set the first block
    blockIterator->counter = coverageMap->getWasExecuted( currentAddress );

    // Analyze remaining arcs and blocks
    while ( blockIterator != blocks.end() ) {
      while ( arcIterator->sourceBlock != blockIterator->id ) {
        if ( arcIterator == arcs.end() ) {
          fprintf( stderr, "ERROR: Unexpectedly runned out of arcs to analyze\n" );
          return false;
        }
        arcIterator++;
        arcIterator2++;
      }

      // If this is the last arc, propagate counter and exit
      if ( arcIterator2 == arcs.end() ) {
        //fprintf( stderr,
        //        "DEBUG: Found last arc %3u -> %3u\n",
        //        arcIterator->sourceBlock,
        //        arcIterator->destinationBlock
        //);
        arcIterator->counter = blockIterator->counter;
        blockIterator2 =  blocks.begin();
        while ( arcIterator->destinationBlock != blockIterator2->id)  //TODO: ADD FAILSAFE
          blockIterator2++;
        blockIterator2->counter += arcIterator->counter;
        return true;
      }

      // If this is not a branch, propagate counter and continue
      if ( arcIterator->sourceBlock != arcIterator2->sourceBlock ) {
        //fprintf( stderr, "DEBUG: Found simple arc %3u -> %3u\n", arcIterator->sourceBlock, arcIterator->destinationBlock );
        arcIterator->counter = blockIterator->counter;
        blockIterator2 =  blocks.begin();;
        while ( arcIterator->destinationBlock != blockIterator2->id) //TODO: ADD FAILSAFE
          blockIterator2++;
        blockIterator2->counter += arcIterator->counter;
      }

      // If this is  a branch with FAKE arc
      else if ( (arcIterator->sourceBlock == arcIterator2->sourceBlock ) && ( arcIterator2->flags & FAKE_ARC_FLAG ))
      {
        //fprintf( stderr, "DEBUG: Found fake branching arc %3u -> %3u\n", arcIterator->sourceBlock, arcIterator->destinationBlock );
        arcIterator->counter = blockIterator->counter;
        blockIterator2 =  blocks.begin();
        while ( arcIterator->destinationBlock != blockIterator2->id) //TODO: ADD FAILSAFE
          blockIterator2++;
        blockIterator2->counter += arcIterator->counter;
      }

      // If this is a legitimate branch
      blockIterator++;
    }

    return true;
  }

  bool GcovFunctionData::processBranches(
            std::list<uint64_t> * taken ,
            std::list<uint64_t> * notTaken
  )
  {
    uint32_t        baseAddress = 0;
    uint32_t        currentAddress;
    std::list<Coverage::objdumpLine_t>::iterator   instruction;

    if ( coverageMap == NULL )
      return false;

    //baseAddress = coverageMap->getFirstLowAddress();      //symbolInfo->baseAddress;
    for (instruction = symbolInfo->instructions.begin(); instruction != symbolInfo->instructions.end(); instruction++)
      if( instruction->isInstruction ) {
        baseAddress = instruction->address;
        break;
      }

    //fprintf( stderr, "DEBUG: Processing instructions in search of branches\n" );
    for (instruction = symbolInfo->instructions.begin(); instruction != symbolInfo->instructions.end(); instruction++)
    {
      if ( instruction->isInstruction) {
        currentAddress = instruction-> address - baseAddress;
        if ( instruction->isBranch ) {
          taken->push_back ( (uint64_t) coverageMap->getWasTaken( currentAddress  ) );
          notTaken->push_back ( (uint64_t) coverageMap->getWasNotTaken( currentAddress ) );
          //fprintf( stderr,
          //          "Added branch to list taken/not: %4u/%4u\n",
          //          coverageMap->getWasTaken( currentAddress ),
          //          coverageMap->getWasNotTaken( currentAddress )
          //);
        }
      }
    }
    return true;
  }
}
