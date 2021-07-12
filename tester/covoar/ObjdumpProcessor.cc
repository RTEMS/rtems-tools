/*! @file ObjdumpProcessor.cc
 *  @brief ObjdumpProcessor Implementation
 *
 *  This file contains the implementation of the functions supporting
 *  the reading of an objdump output file and adding nops to a
 *  coverage map.
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>

#include "ObjdumpProcessor.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"
#include "SymbolTable.h"
#include "TargetFactory.h"

#include "rld.h"
#include "rld-process.h"

#define MAX_LINE_LENGTH 512

namespace Coverage {

  void finalizeSymbol(
    ExecutableInfo* const            executableInfo,
    std::string&                     symbolName,
    ObjdumpProcessor::objdumpLines_t instructions,
    bool                             verbose,
    DesiredSymbols&                  symbolsToAnalyze
  ) {
    // Find the symbol's coverage map.
    try {
      CoverageMapBase& coverageMap = executableInfo->findCoverageMap(symbolName);

      uint32_t firstInstructionAddress = UINT32_MAX;

      // Find the address of the first instruction.
      for (auto& line : instructions) {
        if (line.isInstruction) {
          firstInstructionAddress = line.address;
          break;
        }
      }

      if (firstInstructionAddress == UINT32_MAX) {
        std::ostringstream what;
        what << "Could not find first instruction address for symbol "
          << symbolName << " in " << executableInfo->getFileName();
        throw rld::error( what, "Coverage::finalizeSymbol" );
      }

      int rangeIndex = -1;
      uint32_t lowAddress = UINT32_MAX;
      do {
        rangeIndex++;
        lowAddress = coverageMap.getLowAddressOfRange(rangeIndex);
      } while (firstInstructionAddress != lowAddress);

      uint32_t sizeWithoutNops = coverageMap.getSizeOfRange(rangeIndex);
      uint32_t size = sizeWithoutNops;
      uint32_t highAddress = lowAddress + size - 1;
      uint32_t computedHighAddress = highAddress;

      // Find the high address as reported by the address of the last NOP
      // instruction. This ensures that NOPs get marked as executed later.
      for (auto instruction = instructions.rbegin();
          instruction != instructions.rend();
          instruction++) {
        if (instruction->isInstruction) {
          if (instruction->isNop) {
            computedHighAddress = instruction->address + instruction->nopSize;
          }
          break;
        }
      }

      if (highAddress != computedHighAddress) {
        std::cerr << "Function's high address differs between DWARF and objdump: "
          << symbolName << " (0x" << std::hex << highAddress << " and 0x"
          << computedHighAddress - 1 << ")" << std::dec << std::endl;
        size = computedHighAddress - lowAddress;
      }

      // If there are NOT already saved instructions, save them.
      SymbolInformation* symbolInfo = symbolsToAnalyze.find( symbolName );
      if (symbolInfo->instructions.empty()) {
        symbolInfo->sourceFile = executableInfo;
        symbolInfo->baseAddress = lowAddress;
        symbolInfo->instructions = instructions;
      }

      // Add the symbol to this executable's symbol table.
      SymbolTable* theSymbolTable = executableInfo->getSymbolTable();
      theSymbolTable->addSymbol(
        symbolName, lowAddress, highAddress - lowAddress + 1
      );

      // Mark the start of each instruction in the coverage map.
      for (auto& instruction : instructions) {
        coverageMap.setIsStartOfInstruction( instruction.address );
      }

      // Create a unified coverage map for the symbol.
      symbolsToAnalyze.createCoverageMap(
        executableInfo->getFileName().c_str(),
        symbolName,
        size,
        sizeWithoutNops,
        verbose
      );
    } catch (const ExecutableInfo::CoverageMapNotFoundError& e) {
      // Allow execution to continue even if a coverage map could not be
      // found.
      std::cerr << "Coverage map not found for symbol " << e.what()
        << std::endl;
    }
  }

  ObjdumpProcessor::ObjdumpProcessor(
    DesiredSymbols&     symbolsToAnalyze,
    std::shared_ptr<Target::TargetBase>& targetInfo
  ): symbolsToAnalyze_m( symbolsToAnalyze ),
     targetInfo_m( targetInfo )
  {
  }

  ObjdumpProcessor::~ObjdumpProcessor()
  {
  }

  uint32_t ObjdumpProcessor::determineLoadAddress(
    ExecutableInfo* theExecutable
  )
  {
    #define METHOD "ERROR: ObjdumpProcessor::determineLoadAddress - "
    FILE*        loadAddressFile = NULL;
    char*        cStatus;
    uint32_t     offset;
    char         inputBuffer[MAX_LINE_LENGTH];

    // This method should only be call for a dynamic library.
    if (!theExecutable->hasDynamicLibrary())
      return 0;

    std::string dlinfoName = theExecutable->getFileName();
    uint32_t address;
    char inLibName[128];
    std::string Library = theExecutable->getLibraryName();

    dlinfoName += ".dlinfo";
    // Read load address.
    loadAddressFile = ::fopen( dlinfoName.c_str(), "r" );
    if (!loadAddressFile) {
      std::ostringstream what;
      what << "Unable to open " << dlinfoName;
      throw rld::error( what, METHOD );
    }

    // Process the dlinfo file.
    while ( 1 ) {

      // Get a line.
      cStatus = ::fgets( inputBuffer, MAX_LINE_LENGTH, loadAddressFile );
      if (cStatus == NULL) {
        ::fclose( loadAddressFile );
        std::ostringstream what;
        what << "library " << Library << " not found in " << dlinfoName;
        throw rld::error( what, METHOD );
      }
      sscanf( inputBuffer, "%s %x", inLibName, &offset );
      std::string tmp = inLibName;
      if ( tmp.find( Library ) != tmp.npos ) {
        // fprintf( stderr, "%s - 0x%08x\n", inLibName, offset );
        address = offset;
        break;
      }
    }

    ::fclose( loadAddressFile );
    return address;

    #undef METHOD
  }

  bool ObjdumpProcessor::IsBranch( const std::string& instruction )
  {
    if ( !targetInfo_m ) {
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::IsBranch - unknown architecture\n"
      );
      assert(0);
      return false;
    }

    return targetInfo_m->isBranch( instruction );
  }

  bool ObjdumpProcessor::isBranchLine(
    const char* const line
  )
  {
    if ( !targetInfo_m ) {
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::isBranchLine - unknown architecture\n"
      );
      assert(0);
      return false;
    }

    return  targetInfo_m->isBranchLine( line );
  }

  bool ObjdumpProcessor::isNop(
    const char* const line,
    int&              size
  )
  {
    if ( !targetInfo_m ){
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::isNop - unknown architecture\n"
      );
      assert(0);
      return false;
    }

    return targetInfo_m->isNopLine( line, size );
  }

  void ObjdumpProcessor::getFile(
    std::string fileName,
    rld::process::tempfile& objdumpFile,
    rld::process::tempfile& err
    )
  {
    rld::process::status        status;
    rld::process::arg_container args = { targetInfo_m->getObjdump(),
                                         "-Cda", "--section=.text", "--source",
                                         fileName };
    try
    {
      status = rld::process::execute( targetInfo_m->getObjdump(),
                                      args, objdumpFile.name(), err.name() );
      if ( (status.type != rld::process::status::normal)
           || (status.code != 0) ) {
        throw rld::error( "Objdump error", "generating objdump" );
      }
    } catch( rld::error& err )
      {
        std::cout << "Error while running " << targetInfo_m->getObjdump()
                  << " on " << fileName << std::endl;
        std::cout << err.what << " in " << err.where << std::endl;
        return;
      }

    objdumpFile.open( true );
  }

  uint32_t ObjdumpProcessor::getAddressAfter( uint32_t address )
  {
    objdumpFile_t::iterator itr;

    itr = find ( objdumpList.begin(), objdumpList.end(), address );
    if (itr == objdumpList.end()) {
      return 0;
    }

    itr++;
    if (itr == objdumpList.end()) {
      return 0;
    }

    return (*itr);

  }

  void ObjdumpProcessor::loadAddressTable (
    ExecutableInfo* const    executableInformation,
    rld::process::tempfile&  objdumpFile,
    rld::process::tempfile&  err
  )
  {
    int          items;
    uint32_t     offset;
    char         terminator;
    std::string  line;

    // Obtain the objdump file.
    if ( !executableInformation->hasDynamicLibrary() )
      getFile( executableInformation->getFileName(), objdumpFile, err );
    else
      getFile( executableInformation->getLibraryName(), objdumpFile, err );

    // Process all lines from the objdump file.
    while ( true ) {

      // Get the line.
      objdumpFile.read_line( line );
      if ( line.empty() ) {
        break;
      }

      // See if it is the dump of an instruction.
      items = sscanf(
        line.c_str(),
        "%x%c",
        &offset, &terminator
      );

      // If it looks like an instruction ...
      if ((items == 2) && (terminator == ':')) {
        objdumpList.push_back(
          executableInformation->getLoadAddress() + offset
        );
      }
    }
  }

  void ObjdumpProcessor::load(
    ExecutableInfo* const    executableInformation,
    rld::process::tempfile&  objdumpFile,
    rld::process::tempfile&  err,
    bool                     verbose
  )
  {
    std::string     currentSymbol = "";
    uint32_t        instructionOffset;
    int             items;
    int             found;
    objdumpLine_t   lineInfo;
    uint32_t        offset;
    bool            processSymbol = false;
    char            symbol[ MAX_LINE_LENGTH ];
    char            terminator1;
    char            terminatorOne;
    char            terminator2;
    objdumpLines_t  theInstructions;
    char            instruction[ MAX_LINE_LENGTH ];
    char            ID[ MAX_LINE_LENGTH ];
    std::string     call = "";
    std::string     jumpTableID = "";
    std::string     line = "";

    // Obtain the objdump file.
    if ( !executableInformation->hasDynamicLibrary() )
      getFile( executableInformation->getFileName(), objdumpFile, err );
    else
      getFile( executableInformation->getLibraryName(), objdumpFile, err );

    while ( true ) {
      // Get the line.
      objdumpFile.read_line( line );
      if ( line.empty() ) {
        // If we are currently processing a symbol, finalize it.
        if (processSymbol) {
          finalizeSymbol(
            executableInformation,
            currentSymbol,
            theInstructions,
            verbose,
            symbolsToAnalyze_m
          );
          fprintf(
            stderr,
            "WARNING: ObjdumpProcessor::load - analysis of symbol %s \n"
            "         may be incorrect.  It was the last symbol in %s\n"
            "         and the length of its last instruction is assumed "
            "         to be one.\n",
            currentSymbol.c_str(),
            executableInformation->getFileName().c_str()
          );
        }
        objdumpFile.close();
        break;
      }

      // Remove any extra line break
      if (line.back() == '\n') {
        line.erase(line.end() - 1);
      }

      lineInfo.line          = line;
      lineInfo.address       = 0xffffffff;
      lineInfo.isInstruction = false;
      lineInfo.isNop         = false;
      lineInfo.nopSize       = 0;
      lineInfo.isBranch      = false;

      instruction[0] = '\0';
      ID[0] = '\0';

      // Look for the start of a symbol's objdump and extract
      // offset and symbol (i.e. offset <symbolname>:).
      items = sscanf(
        line.c_str(),
        "%x <%[^>]>%c",
        &offset, symbol, &terminator1
      );

      // See if it is a jump table.
      found = sscanf(
        line.c_str(),
        "%x%c\t%*[^\t]%c%s %*x %*[^+]%s",
        &instructionOffset, &terminatorOne, &terminator2, instruction, ID
      );
      call = instruction;
      jumpTableID = ID;

      // If all items found, we are at the beginning of a symbol's objdump.
      if ((items == 3) && (terminator1 == ':')) {
        // If we are currently processing a symbol, finalize it.
        if (processSymbol) {
          finalizeSymbol(
            executableInformation,
            currentSymbol,
            theInstructions,
            verbose,
            symbolsToAnalyze_m
          );
        }

        // Start processing of a new symbol.
        currentSymbol = "";
        processSymbol = false;
        theInstructions.clear();

        // Look for a '.' character and strip everything after it.
        // There is a chance that the compiler splits function bodies to improve
        // inlining. If there exists some inlinable function that contains a
        // branch where one path is more expensive and less likely to be taken
        // than the other, inlining only the branch instruction and the less
        // expensive path results in smaller code size while preserving most of
        // the performance improvement.
        // When this happens, the compiler will generate a function with a
        // ".part.n" suffix. For our purposes, this generated function part is
        // equivalent to the original function and should be treated as such.
        char *periodIndex = strstr(symbol, ".");
        if (periodIndex != NULL) {
          *periodIndex = 0;
        }

        // See if the new symbol is one that we care about.
        if (symbolsToAnalyze_m.isDesired( symbol )) {
          currentSymbol = symbol;
          processSymbol = true;
          theInstructions.push_back( lineInfo );
        }
      }
      // If it looks like a jump table, finalize the symbol.
      else if ( (found == 5) && (terminatorOne == ':') && (terminator2 == '\t')
               && (call.find( "call" ) != std::string::npos)
               && (jumpTableID.find( "+0x" ) != std::string::npos)
               && processSymbol )
      {
        // If we are currently processing a symbol, finalize it.
        if ( processSymbol ) {
          finalizeSymbol(
            executableInformation,
            currentSymbol,
            theInstructions,
            verbose,
            symbolsToAnalyze_m
          );
        }
        processSymbol = false;
      }
      else if (processSymbol) {

        // See if it is the dump of an instruction.
        items = sscanf(
          line.c_str(),
          "%x%c\t%*[^\t]%c",
          &instructionOffset, &terminator1, &terminator2
        );

        // If it looks like an instruction ...
        if ((items == 3) && (terminator1 == ':') && (terminator2 == '\t')) {
          // update the line's information, save it and ...
          lineInfo.address =
           executableInformation->getLoadAddress() + instructionOffset;
          lineInfo.isInstruction = true;
          lineInfo.isNop         = isNop( line.c_str(), lineInfo.nopSize );
          lineInfo.isBranch      = isBranchLine( line.c_str() );
        }

        // Always save the line.
        theInstructions.push_back( lineInfo );
      }
    }
  }

  void ObjdumpProcessor::setTargetInfo(
    std::shared_ptr<Target::TargetBase>& targetInfo
  )
  {
    targetInfo_m = targetInfo;
  }
}
