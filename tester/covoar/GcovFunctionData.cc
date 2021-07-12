/*! @file GcovFunctionData.cc
 *  @brief GcovFunctionData Implementation
 *
 *  This file contains the implementation of the class storing information
 *  about single function.
 */

#include <cstdio>
#include <cstring>
#include <cinttypes>

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
    const std::string&        fcnName,
    Coverage::DesiredSymbols& symbolsToAnalyze
  )
  {
    std::string   symbolName;

    symbolName = fcnName;

    if ( fcnName.length() >= FUNCTION_NAME_LENGTH ) {
      std::cerr << "ERROR: Function name is too long to be correctly stored: "
                << fcnName.length() << std::endl;
      return false;
    }

    functionName = fcnName;

    // Tie function to its coverage map
    symbolInfo = symbolsToAnalyze.find( symbolName );
    if ( symbolInfo != NULL )
      coverageMap = symbolInfo->unifiedCoverageMap;

#if 0
    if ( coverageMap == NULL ) {
      std::cerr << "ERROR: Could not find coverage map for: " << symbolName
                << std::endl;
    } else {
      std::cerr << "SUCCESS: Found coverage map for: " << symbolName
                << std::endl;
    }
#endif

    return true;
  }

  bool GcovFunctionData::setFileName( const std::string& fileName ) {
    if ( fileName.length() >= FILE_NAME_LENGTH ) {
      std::cerr << "ERROR: File name is too long to be correctly stored: "
                << fileName.length() << std::endl;
      return false;
    }

    sourceFileName = fileName;
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
    const uint32_t     id,
    const uint32_t     flags,
    const std::string& sourceFileName
  )
  {
    gcov_block_info block;

    numberOfBlocks++;
    block.id    = id;
    block.flags = flags;
    block.numberOfLines = 0;
    block.counter = 0;
    block.sourceFileName = sourceFileName;
    blocks.push_back(block);
  }

  void GcovFunctionData::printFunctionInfo(
    std::ofstream& textFile,
    uint32_t       function_number
  )
  {
    blocks_iterator_t  currentBlock;
    arcs_iterator_t    currentArc;

    textFile << std::endl << std::endl
             << "=========================="
             << "FUNCTION  " << std::setw( 3 ) << function_number
             << "=========================="
             << std::endl << std::endl
             << "Name:      " << functionName << std::endl
             << "File:      " << sourceFileName << std::endl
             << "Line:      " << firstLineNumber << std::endl
             << "Id:        " << id << std::endl
             << "Checksum:  0x" << std::hex << checksum << std::dec
             << std::endl << std::endl;

    // Print arcs info
    for ( currentArc = arcs.begin(); currentArc != arcs.end(); currentArc++ ) {
      printArcInfo( textFile, currentArc );
    }
    textFile << std::endl;

    // Print blocks info
    for ( currentBlock = blocks.begin();
          currentBlock != blocks.end();
          currentBlock++
    ) {
      printBlockInfo( textFile, currentBlock );
    }
  }

  void GcovFunctionData::printCoverageInfo(
    std::ofstream& textFile,
    uint32_t       function_number
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

      textFile << std::endl << "Instructions (Base address: 0x"
               << std::setfill( '0' ) << std::setw( 8 )
               << std::hex << baseAddress << std::dec << std::setfill( ' ' )
               << ", Size: " << std::setw( 4 ) << baseSize
               << "):" << std::endl << std::endl;

      for ( instruction = symbolInfo->instructions.begin();
            instruction != symbolInfo->instructions.end();
            instruction++
      )
      {
        if ( instruction->isInstruction ) {
          currentAddress = instruction->address - baseAddress;

          textFile << std::left << "0x" << std::setw( 70 )
                   << instruction->line.c_str() << " " << std::right
                   << "| 0x" << std::hex << std::setfill( '0' )
                   << std::setw( 8 ) << currentAddress << " "
                   << std::dec << std::setfill( ' ' )
                   << "*| exec: " << std::setw( 4 )
                   << coverageMap->getWasExecuted( currentAddress )
                   << " | taken/not: " << std::setw( 4 )
                   << coverageMap->getWasTaken( currentAddress )
                   << "/" << std::setw( 4 )
                   << coverageMap->getWasNotTaken( currentAddress )
                   << " ";

          if ( instruction->isBranch )
            textFile << "| Branch ";
          else
            textFile << "         ";

          if ( instruction->isNop )
            textFile << "| NOP(" << std::setw( 3 ) << instruction->nopSize
                     << ") " << std::endl;
          else
            textFile << "           " << std::endl;
        }
      }
    }
  }

  void GcovFunctionData::setBlockFileName(
    const blocks_iterator_t  block,
    const std::string&       fileName
  )
  {
    block->sourceFileName = fileName;
  }

  void GcovFunctionData::addBlockLine(
    const blocks_iterator_t  block,
    const uint32_t           line
  )
  {
    block->lines.push_back(line);
    (block->numberOfLines)++;
  }

  blocks_iterator_t GcovFunctionData::findBlockById( const uint32_t id )
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
      std::cerr << "ERROR: GcovFunctionData::findBlockById() failed"
                << ", no blocks present" << std::endl;
    }
    return blockIterator;
  }

  void GcovFunctionData::printArcInfo(
    std::ofstream&  textFile,
    arcs_iterator_t arc
  )
  {
    textFile << " > ARC " << std::setw( 3 ) << arc->sourceBlock
             << " -> " << arc->destinationBlock << " ";

    textFile << "\tFLAGS: ";
    switch ( arc->flags ) {
      case 0:
        textFile << "( ___________ ____ _______ )";
        break;
      case 1:
        textFile << "( ___________ ____ ON_TREE )";
        break;
      case 2:
        textFile << "( ___________ FAKE _______ )";
        break;
      case 3:
        textFile << "( ___________ FAKE ON_TREE )";
        break;
      case 4:
        textFile << "( FALLTHROUGH ____ _______ )";
        break;
      case 5:
        textFile << "( FALLTHROUGH ____ ON_TREE )";
        break;
      default:
        textFile  << "( =======FLAGS_ERROR====== )";
        std::cerr << " ERROR: Unknown arc flag: 0x"
                  << std::hex << arcs.back().flags << std::endl
                  << std::dec;
        break;
    }

    textFile << "\tTaken: " << std::setw( 5 ) << arc->counter << std::endl;
  }

  void GcovFunctionData::printBlockInfo(
    std::ofstream&    textFile,
    blocks_iterator_t block
  )
  {
    std::list<uint32_t>::iterator  line;

    textFile << " > BLOCK " << std::setw( 3 ) << block->id
             << " from " << block->sourceFileName << std::endl
             << "    -counter: " << std::setw( 5 ) << block->counter << std::endl
             << "    -flags: 0x" << std::hex << block->flags << std::endl
             << "    -lines: ";

    if ( !block->lines.empty() )
      for ( line = block->lines.begin(); line != block->lines.end(); line++ ) {
        textFile << *line << ", ";
      }

    textFile << std::endl;
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

    //std::cerr << "DEBUG: Processing counters for file: " << sourceFileName
    //          << std::endl;
    if ( blocks.empty() || arcs.empty() || coverageMap == NULL || symbolInfo->instructions.empty())     {
      //std::cerr << "DEBUG: sanity check returned false for function: "
      //          << functionName << " from file: " << sourceFileName
      //          << std::endl;
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
      //std::cerr << "ERROR: Failed to process branches for function: "
      //          << functionName << " from file: " << sourceFileName
      //          << std::endl;
      return false;
    };

    // Process the branching arcs
    while ( blockIterator != blocks.end() ) {
      //std::cerr << "DEBUG: Processing branches" << std::endl;
      while ( arcIterator->sourceBlock != blockIterator->id ) {
        if ( arcIterator == arcs.end() ) {
          //std::cerr << "ERROR: Unexpectedly runned out of arcs to analyze"
          //          << std::endl;
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
          std::cerr << "ERROR: Branches missing for function: "
                    << functionName << " from file: " << sourceFileName
                    << std::endl;
          return false;
        }

        //std::cerr << "DEBUG: Found true branching arc "
        //          << std::setw( 3 ) << arcIterator->sourceBlock << " -> "
        //          << std::setw( 3 ) << arcIteratior->destinationBlock
        //          << std::endl;
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
          std::cerr << "ERROR: Unexpectedly runned out of arcs to analyze"
                    << std::endl;
          return false;
        }
        arcIterator++;
        arcIterator2++;
      }

      // If this is the last arc, propagate counter and exit
      if ( arcIterator2 == arcs.end() ) {
        //std::cerr << "DEBUG: Found last arc " << std::setw( 3 )
        //          << arcIterator->sourceBlock << " -> " << std::setw( 3 )
        //          << arcIterator->destinationBlock << std::endl;
        arcIterator->counter = blockIterator->counter;
        blockIterator2 =  blocks.begin();
        while ( arcIterator->destinationBlock != blockIterator2->id)  //TODO: ADD FAILSAFE
          blockIterator2++;
        blockIterator2->counter += arcIterator->counter;
        return true;
      }

      // If this is not a branch, propagate counter and continue
      if ( arcIterator->sourceBlock != arcIterator2->sourceBlock ) {
        //std::cerr << "DEBUG: Found simple arc " << std::setw( 3 )
        //          << arcIterator->sourceBlock << " -> " << std::setw( 3 )
        //          << arcIterator->destinationBlock << std::endl;
        arcIterator->counter = blockIterator->counter;
        blockIterator2 =  blocks.begin();;
        while ( arcIterator->destinationBlock != blockIterator2->id) //TODO: ADD FAILSAFE
          blockIterator2++;
        blockIterator2->counter += arcIterator->counter;
      }

      // If this is  a branch with FAKE arc
      else if ( (arcIterator->sourceBlock == arcIterator2->sourceBlock ) && ( arcIterator2->flags & FAKE_ARC_FLAG ))
      {
        //std::cerr << "DEBUG: Found fake branching arc " << std::setw( 3 )
        //          << arcIterator->sourceBlock << " -> " << std::setw( 3 )
        //          << arcIterator->destinationBlock << std::endl;
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

    // std::cerr << "DEBUG: Processing instructions in search of branches"
    //           << std::endl;
    for (instruction = symbolInfo->instructions.begin(); instruction != symbolInfo->instructions.end(); instruction++)
    {
      if ( instruction->isInstruction) {
        currentAddress = instruction-> address - baseAddress;
        if ( instruction->isBranch ) {
          taken->push_back ( (uint64_t) coverageMap->getWasTaken( currentAddress  ) );
          notTaken->push_back ( (uint64_t) coverageMap->getWasNotTaken( currentAddress ) );

          //std::cerr << "Added branch to list taken/not: " << std::setw( 4 )
          //          << coverageMap->getWasTaken( currentAddress )
          //          << "/" << std::setw( 4 )
          //          << coverageMap->getWasNotTaken( currentAddress )
          //          << std::endl;
        }
      }
    }
    return true;
  }
}
