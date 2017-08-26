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

#include "app_common.h"
#include "ObjdumpProcessor.h"
#include "CoverageMap.h"
#include "ExecutableInfo.h"
#include "SymbolTable.h"
#include "TargetFactory.h"

namespace Coverage {

  void finalizeSymbol(
    ExecutableInfo* const            executableInfo,
    std::string&                     symbolName,
    uint32_t                         lowAddress,
    uint32_t                         highAddress,
    ObjdumpProcessor::objdumpLines_t instructions
  ) {

    CoverageMapBase*                                   aCoverageMap = NULL;
    uint32_t                                           endAddress = highAddress;
    ObjdumpProcessor::objdumpLines_t::iterator         itr, fnop, lnop;
    ObjdumpProcessor::objdumpLines_t::reverse_iterator ritr;
    SymbolInformation*                                 symbolInfo = NULL;
    SymbolTable*                                       theSymbolTable;

    //
    // Remove trailing nop instructions.
    //

    // First find the last instruction.
    for (ritr = instructions.rbegin();
         ritr != instructions.rend();
         ritr++) {
      if (ritr->isInstruction)
        break;
    }

    // If an instruction was found and it is a nop, ...
    if ((ritr != instructions.rend()) && (ritr->isNop)) {

      // save it as the last nop.  Note that we must account for
      // the difference between a forward and a reverse iterator.
      lnop = ritr.base();
      lnop--;
      endAddress -= lnop->nopSize;

      // Now look for the first nop in the sequence of trailing nops.
      fnop = lnop;
      ritr++;
      for (; ritr != instructions.rend(); ritr++) {
        if (ritr->isNop) {
          fnop = ritr.base();
          fnop--;
          endAddress -= fnop->nopSize;
        }
        else
          break;
      }

      // Erase trailing nops.  The erase operation wants the first
      // parameter to point to the first item to erase and the second
      // parameter to point to the item beyond the last item to erase.
      if ( fnop == lnop )
        instructions.erase( fnop );
      else
        instructions.erase( fnop, ++lnop );
    }

    // If there are NOT already saved instructions, save them.
    symbolInfo = SymbolsToAnalyze->find( symbolName );
    if (symbolInfo->instructions.empty()) {
      symbolInfo->sourceFile = executableInfo;
      symbolInfo->baseAddress = lowAddress;
      symbolInfo->instructions = instructions;
    }

    // Add the symbol to this executable's symbol table.
    theSymbolTable = executableInfo->getSymbolTable();
    theSymbolTable->addSymbol(
      symbolName, lowAddress, endAddress - lowAddress + 1
    );

    // Create a coverage map for the symbol.
    aCoverageMap = executableInfo->createCoverageMap(
      executableInfo->getFileName().c_str(), symbolName, lowAddress, endAddress
    );

    if (aCoverageMap) {

      // Mark the start of each instruction in the coverage map.
      for (itr = instructions.begin();
           itr != instructions.end();
           itr++ ) {

        aCoverageMap->setIsStartOfInstruction( itr->address );
      }

      // Create a unified coverage map for the symbol.
      SymbolsToAnalyze->createCoverageMap(
        executableInfo->getFileName().c_str(), symbolName,
        endAddress - lowAddress + 1
      );
    }
  }

  ObjdumpProcessor::ObjdumpProcessor()
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

    // This method should only be call for a dynamic library.
    if (!theExecutable->hasDynamicLibrary())
      return 0;

    std::string dlinfoName = theExecutable->getFileName();
    uint32_t address;
    char inLibName[128];
    std::string Library = theExecutable->getLibraryName();

    dlinfoName += ".dlinfo";
    // Read load address.
    loadAddressFile = fopen( dlinfoName.c_str(), "r" );
    if (!loadAddressFile) {
      fprintf( stderr, METHOD "unable to open %s\n", dlinfoName.c_str() );
      exit( -1 );
    }

    // Process the dlinfo file.
    while ( 1 ) {

      // Get a line.
      cStatus = fgets( inputBuffer, MAX_LINE_LENGTH, loadAddressFile );
      if (cStatus == NULL) {
        fprintf(
          stderr,
          METHOD "library %s not found in %s\n",
          Library.c_str(),
          dlinfoName.c_str()
        );
        fclose( loadAddressFile );
        exit( -1 );
      }
      sscanf( inputBuffer, "%s %x", inLibName, &offset );
      std::string tmp = inLibName;
      if ( tmp.find( Library ) != tmp.npos ) {
        // fprintf( stderr, "%s - 0x%08x\n", inLibName, offset );
        address = offset;
        break;
      }
    }

    fclose( loadAddressFile );
    return address;

    #undef METHOD
  }

  bool ObjdumpProcessor::IsBranch(
    const char *instruction
  )
  {
    if ( !TargetInfo ) {
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::IsBranch - unknown architecture\n"
      );
      assert(0);
      return false;
    }

    return TargetInfo->isBranch( instruction );
  }

  bool ObjdumpProcessor::isBranchLine(
    const char* const line
  )
  {
    if ( !TargetInfo ) {
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::isBranchLine - unknown architecture\n"
      );
      assert(0);
      return false;
    }

    return  TargetInfo->isBranchLine( line );
  }

  bool ObjdumpProcessor::isNop(
    const char* const line,
    int&              size
  )
  {
    if ( !TargetInfo ){
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::isNop - unknown architecture\n"
      );
      assert(0);
      return false;
    }

    return TargetInfo->isNopLine( line, size );
  }

  FILE* ObjdumpProcessor::getFile( std::string fileName )
  {
    char               dumpFile[128];
    FILE*              objdumpFile;
    char               buffer[ 512 ];
    int                status;

    sprintf( dumpFile, "%s.dmp", fileName.c_str() );

    // Generate the objdump.
    if (FileIsNewer( fileName.c_str(), dumpFile )) {
      sprintf(
        buffer,
        "%s -Cda --section=.text --source %s | sed -e \'s/ *$//\' >%s",
        TargetInfo->getObjdump(),
        fileName.c_str(),
        dumpFile
      );

      status = system( buffer );
      if (status) {
        fprintf(
          stderr,
          "ERROR: ObjdumpProcessor::getFile - command (%s) failed with %d\n",
          buffer,
          status
        );
        exit( -1 );
      }
    }

    // Open the objdump file.
    objdumpFile = fopen( dumpFile, "r" );
    if (!objdumpFile) {
      fprintf(
        stderr,
        "ERROR: ObjdumpProcessor::getFile - unable to open %s\n",
        dumpFile
      );
      exit(-1);
    }

    return objdumpFile;
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
    ExecutableInfo* const executableInformation
  )
  {
    char*              cStatus;
    int                items;
    FILE*              objdumpFile;
    uint32_t           offset;
    char               terminator;

    // Obtain the objdump file.
    if (!executableInformation->hasDynamicLibrary())
      objdumpFile = getFile( executableInformation->getFileName() );
    else
      objdumpFile = getFile( executableInformation->getLibraryName() );

    // Process all lines from the objdump file.
    while ( 1 ) {

      // Get the line.
      cStatus = fgets( inputBuffer, MAX_LINE_LENGTH, objdumpFile );
      if (cStatus == NULL) {
        break;
      }
      inputBuffer[ strlen(inputBuffer) - 1] = '\0';

      // See if it is the dump of an instruction.
      items = sscanf(
        inputBuffer,
        "%x%c",
        &offset, &terminator
      );

      // If it looks like an instruction ...
      if ((items == 2) && (terminator == ':')){
        objdumpList.push_back(
          executableInformation->getLoadAddress() + offset
        );
      }
    }
  }

  void ObjdumpProcessor::load(
    ExecutableInfo* const executableInformation
  )
  {
    char*              cStatus;
    std::string        currentSymbol = "";
    uint32_t           endAddress;
    uint32_t           instructionOffset;
    int                items;
    objdumpLine_t      lineInfo;
    FILE*              objdumpFile;
    uint32_t           offset;
    bool               processSymbol = false;
    uint32_t           startAddress = 0;
    char               symbol[ MAX_LINE_LENGTH ];
    char               terminator1;
    char               terminator2;
    objdumpLines_t     theInstructions;

    // Obtain the objdump file.
    if (!executableInformation->hasDynamicLibrary())
      objdumpFile = getFile( executableInformation->getFileName() );
    else
      objdumpFile = getFile( executableInformation->getLibraryName() );

    // Process all lines from the objdump file.
    while ( 1 ) {

      // Get the line.
      cStatus = fgets( inputBuffer, MAX_LINE_LENGTH, objdumpFile );
      if (cStatus == NULL) {

        // If we are currently processing a symbol, finalize it.
        if (processSymbol) {
          finalizeSymbol(
            executableInformation,
            currentSymbol,
            startAddress,
            executableInformation->getLoadAddress() + offset,
            theInstructions
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
        break;
      }

      inputBuffer[ strlen(inputBuffer) - 1] = '\0';

      lineInfo.line          = inputBuffer;
      lineInfo.address       = 0xffffffff;
      lineInfo.isInstruction = false;
      lineInfo.isNop         = false;
      lineInfo.nopSize       = 0;
      lineInfo.isBranch      = false;

      // Look for the start of a symbol's objdump and extract
      // offset and symbol (i.e. offset <symbolname>:).
      items = sscanf(
        inputBuffer,
        "%x <%[^>]>%c",
        &offset, symbol, &terminator1
      );

      // If all items found, we are at the beginning of a symbol's objdump.
      if ((items == 3) && (terminator1 == ':')) {

        endAddress = executableInformation->getLoadAddress() + offset - 1;

        // If we are currently processing a symbol, finalize it.
        if (processSymbol) {
          finalizeSymbol(
            executableInformation,
            currentSymbol,
            startAddress,
            endAddress,
            theInstructions
          );
        }

        // Start processing of a new symbol.
        startAddress = 0;
        currentSymbol = "";
        processSymbol = false;
        theInstructions.clear();

        // See if the new symbol is one that we care about.
        if (SymbolsToAnalyze->isDesired( symbol )) {
          startAddress = executableInformation->getLoadAddress() + offset;
          currentSymbol = symbol;
          processSymbol = true;
          theInstructions.push_back( lineInfo );
        }
      }

      else if (processSymbol) {

        // See if it is the dump of an instruction.
        items = sscanf(
          inputBuffer,
          "%x%c\t%*[^\t]%c",
          &instructionOffset, &terminator1, &terminator2
        );

        // If it looks like an instruction ...
        if ((items == 3) && (terminator1 == ':') && (terminator2 == '\t')) {

          // update the line's information, save it and ...
          lineInfo.address =
           executableInformation->getLoadAddress() + instructionOffset;
          lineInfo.isInstruction = true;
          lineInfo.isNop         = isNop( inputBuffer, lineInfo.nopSize );
          lineInfo.isBranch      = isBranchLine( inputBuffer );
        }

        // Always save the line.
        theInstructions.push_back( lineInfo );
      }
    }
  }
}
