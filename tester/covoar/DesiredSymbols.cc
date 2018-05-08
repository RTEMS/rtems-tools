/*! @file DesiredSymbols.cc
 *  @brief DesiredSymbols Implementation
 *
 *  This file contains the implementation of the functions
 *  which provide the functionality of the DesiredSymbols.
 */

#ifdef __CYGWIN__
#undef __STRICT_ANSI__
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

#include "rld.h"
#include <rld-config.h>
#include "rld-symbols.h"
#include "rld-files.h"

#include "DesiredSymbols.h"
#include "app_common.h"
#include "CoverageMap.h"
#include "ObjdumpProcessor.h"

namespace Coverage {

  DesiredSymbols::DesiredSymbols()
  {
  }

  DesiredSymbols::~DesiredSymbols()
  {
  }

  bool DesiredSymbols::load(
    const std::string& symbolsSet,
    const std::string& buildTarget,
    const std::string& buildBSP,
    bool               verbose
  )
  {
    rld::files::cache cache;
    bool              r = true;

    //
    // Load the INI file looking for a top level:
    //
    //  [symbols-sets]
    //  sets = A, B, C
    //
    // For each set read the libraries from the configuration file and load.
    //
    //  [A]
    //  libraries = @BUILD-PREFIX@/c/@BSP@/A/libA.a
    //
    //  [B]
    //  libraries = @BUILD-PREFIX@/c/@BSP@/B/libB.a
    //
    try {
      cache.open();

      rld::config::config config;

      if (verbose)
        std::cerr << "Loading symbol sets: " << symbolsSet << std::endl;

      config.load (symbolsSet);

      const rld::config::section& sym_section = config.get_section("symbol-sets");

      rld::strings sets;
      rld::config::parse_items (sym_section, "sets", sets, true);

      for (const std::string set : sets) {
        if (verbose)
          std::cerr << " Symbol set: " << set << std::endl;
        const rld::config::section& set_section = config.get_section(set);
        rld::strings libs;
        rld::config::parse_items (set_section, "libraries", libs, true);
        for (std::string lib : libs) {
          lib = rld::find_replace(lib, "@BUILD-TARGET@", buildTarget);
          lib = rld::find_replace(lib, "@BSP@", buildBSP);
          if (verbose)
            std::cerr << " Loading library: " << lib << std::endl;
          cache.add(lib);
        }
      }

      rld::symbols::table symbols;

      cache.load_symbols (symbols, true);

      for (auto& kv : symbols.globals()) {
        const rld::symbols::symbol& sym = *(kv.second);
        set[sym.name()] = *(new SymbolInformation);
      }
      for (auto& kv : symbols.weaks()) {
        const rld::symbols::symbol& sym = *(kv.second);
        set[sym.name()] = *(new SymbolInformation);
      }

    } catch (rld::error re) {
      std::cerr << "error: "
                << re.where << ": " << re.what
                << std::endl;
      r = false;
    } catch (...) {
      cache.close();
      throw;
    }

    cache.close();

    return r;
  }

  void DesiredSymbols::preprocess( void )
  {
    ObjdumpProcessor::objdumpLines_t::iterator fitr;
    ObjdumpProcessor::objdumpLines_t::iterator n, p;
    DesiredSymbols::symbolSet_t::iterator      sitr;
    CoverageMapBase*                           theCoverageMap;

    // Look at each symbol.
    for (sitr = SymbolsToAnalyze->set.begin();
         sitr != SymbolsToAnalyze->set.end();
         sitr++) {

      // If the unified coverage map does not exist, the symbol was
      // never referenced by any executable.  Just skip it.
      theCoverageMap = sitr->second.unifiedCoverageMap;
      if (!theCoverageMap)
        continue;

      // Mark any branch and NOP instructions.
      for (fitr = sitr->second.instructions.begin();
           fitr != sitr->second.instructions.end();
           fitr++) {
        if (fitr->isBranch) {
           theCoverageMap->setIsBranch(
             fitr->address - sitr->second.baseAddress
           );
        }
        if (fitr->isNop) {
           theCoverageMap->setIsNop(
             fitr->address - sitr->second.baseAddress
           );
        }
      }

    }
  }

  void DesiredSymbols::calculateStatistics( void )
  {
    uint32_t                              a;
    uint32_t                              endAddress;
    DesiredSymbols::symbolSet_t::iterator sitr;
    CoverageMapBase*                      theCoverageMap;

    // Look at each symbol.
    for (sitr = SymbolsToAnalyze->set.begin();
         sitr != SymbolsToAnalyze->set.end();
         sitr++) {

      // If the unified coverage map does not exist, the symbol was
      // never referenced by any executable.  Just skip it.
      theCoverageMap = sitr->second.unifiedCoverageMap;
      if (!theCoverageMap)
        continue;

      // Increment the total sizeInBytes byt the bytes in the symbol
      stats.sizeInBytes += sitr->second.stats.sizeInBytes;

      // Now scan through the coverage map of this symbol.
      endAddress = sitr->second.stats.sizeInBytes - 1;
      a = 0;
      while (a <= endAddress) {

        // If we are at the start of instruction increment
        // instruction type counters as needed.
        if ( theCoverageMap->isStartOfInstruction( a ) ) {

          stats.sizeInInstructions++;
          sitr->second.stats.sizeInInstructions++;

          if (!theCoverageMap->wasExecuted( a ) ) {
            stats.uncoveredInstructions++;
            sitr->second.stats.uncoveredInstructions++;

            if ( theCoverageMap->isBranch( a )) {
              stats.branchesNotExecuted++;
              sitr->second.stats.branchesNotExecuted++;
             }
          } else if (theCoverageMap->isBranch( a )) {
            stats.branchesExecuted++;
            sitr->second.stats.branchesExecuted++;
          }

        }

        if (!theCoverageMap->wasExecuted( a )) {
          stats.uncoveredBytes++;
          sitr->second.stats.uncoveredBytes++;
        }
        a++;

      }
    }
  }


  void DesiredSymbols::computeUncovered( void )
  {
    uint32_t                              a, la, ha;
    uint32_t                              endAddress;
    uint32_t                              count;
    DesiredSymbols::symbolSet_t::iterator sitr;
    CoverageRanges*                       theBranches;
    CoverageMapBase*                      theCoverageMap;
    CoverageRanges*                       theRanges;

    // Look at each symbol.
    for (sitr = SymbolsToAnalyze->set.begin();
         sitr != SymbolsToAnalyze->set.end();
         sitr++) {

      // If the unified coverage map does not exist, the symbol was
      // never referenced by any executable.  Just skip it.
      theCoverageMap = sitr->second.unifiedCoverageMap;
      if (!theCoverageMap)
        continue;

      // Create containers for the symbol's uncovered ranges and branches.
      theRanges = new CoverageRanges();
      sitr->second.uncoveredRanges = theRanges;
      theBranches = new CoverageRanges();
      sitr->second.uncoveredBranches = theBranches;

      // Mark NOPs as executed
      endAddress = sitr->second.stats.sizeInBytes - 1;
      a = 0;
      while (a < endAddress) {
        if (!theCoverageMap->wasExecuted( a )) {
          a++;
          continue;
        }

	for (ha=a+1;
	     ha<=endAddress && !theCoverageMap->isStartOfInstruction( ha );
	     ha++)
	  ;
        if ( ha >= endAddress )
          break;

        if (theCoverageMap->isNop( ha ))
          do {
            theCoverageMap->setWasExecuted( ha );
	    ha++;
            if ( ha >= endAddress )
              break;
          } while ( !theCoverageMap->isStartOfInstruction( ha ) );
        a = ha;
      }

      // Now scan through the coverage map of this symbol.
      endAddress = sitr->second.stats.sizeInBytes - 1;
      a = 0;
      while (a <= endAddress) {

        // If an address was NOT executed, find consecutive unexecuted
        // addresses and add them to the uncovered ranges.
        if (!theCoverageMap->wasExecuted( a )) {

          la = a;
          count = 1;
          for (ha=a+1;
               ha<=endAddress && !theCoverageMap->wasExecuted( ha );
               ha++)
          {
            if ( theCoverageMap->isStartOfInstruction( ha ) )
              count++;
          }
          ha--;

          stats.uncoveredRanges++;
          sitr->second.stats.uncoveredRanges++;
          theRanges->add(
            sitr->second.baseAddress + la,
            sitr->second.baseAddress + ha,
            CoverageRanges::UNCOVERED_REASON_NOT_EXECUTED,
            count
          );
          a = ha + 1;
        }

        // If an address is a branch instruction, add any uncovered branches
        // to the uncoverd branches.
        else if (theCoverageMap->isBranch( a )) {
          la = a;
          for (ha=a+1;
               ha<=endAddress && !theCoverageMap->isStartOfInstruction( ha );
               ha++)
            ;
          ha--;

          if (theCoverageMap->wasAlwaysTaken( la )) {
            stats.branchesAlwaysTaken++;
            sitr->second.stats.branchesAlwaysTaken++;
            theBranches->add(
              sitr->second.baseAddress + la,
              sitr->second.baseAddress + ha,
              CoverageRanges::UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN,
              1
            );
            if (Verbose)
              fprintf(
                stderr,
                "Branch always taken found in %s (0x%x - 0x%x)\n",
                (sitr->first).c_str(),
                sitr->second.baseAddress + la,
                sitr->second.baseAddress + ha
              );
          }

          else if (theCoverageMap->wasNeverTaken( la )) {
            stats.branchesNeverTaken++;
            sitr->second.stats.branchesNeverTaken++;
            theBranches->add(
              sitr->second.baseAddress + la,
              sitr->second.baseAddress + ha,
              CoverageRanges::UNCOVERED_REASON_BRANCH_NEVER_TAKEN,
              1
            );
            if (Verbose)
              fprintf(
                stderr,
                "Branch never taken found in %s (0x%x - 0x%x)\n",
                (sitr->first).c_str(),
                sitr->second.baseAddress + la,
                sitr->second.baseAddress + ha
              );
          }
          a = ha + 1;
        }
        else
          a++;
      }
    }
  }


  void DesiredSymbols::createCoverageMap(
    const std::string& exefileName,
    const std::string& symbolName,
    uint32_t           size
  )
  {
    CoverageMapBase*      aCoverageMap;
    uint32_t              highAddress;
    symbolSet_t::iterator itr;

    // Ensure that the symbol is a desired symbol.
    itr = set.find( symbolName );

    if (itr == set.end()) {

      fprintf(
        stderr,
        "ERROR: DesiredSymbols::createCoverageMap - Unable to create "
        "unified coverage map for %s because it is NOT a desired symbol\n",
        symbolName.c_str()
      );
      exit( -1 );
    }

    // If we have already created a coverage map, ...
    if (itr->second.unifiedCoverageMap) {

      // ensure that the specified size matches the existing size.
      if (itr->second.stats.sizeInBytes != size) {

        // Changed ERROR to INFO because size mismatch is not treated as
        // error anymore.
        // Set smallest size as size and continue.
        // Update value for longer byte size.
        // 2015-07-22
        fprintf(
          stderr,
          "INFO: DesiredSymbols::createCoverageMap - Attempt to create "
          "unified coverage maps for %s with different sizes (%s/%d != %s/%d)\n",

          symbolName.c_str(),
          exefileName.c_str(),
          itr->second.stats.sizeInBytes,
          itr->second.sourceFile->getFileName().c_str(),
          size
        );

        if ( itr->second.stats.sizeInBytes < size )
          itr->second.stats.sizeInBytes = size;
        else
          size = itr->second.stats.sizeInBytes;
      }
    }

    // If we don't already have a coverage map, create one.
    else {

      highAddress = size - 1;

      aCoverageMap = new CoverageMap( exefileName, 0, highAddress );
      if (!aCoverageMap) {

        fprintf(
          stderr,
          "ERROR: DesiredSymbols::createCoverageMap - Unable to allocate "
          "coverage map for %s:%s\n",
          exefileName.c_str(),
          symbolName.c_str()
        );
        exit( -1 );
      }

      if ( Verbose )
        fprintf(
          stderr,
          "Created unified coverage map for %s (0x%x - 0x%x)\n",
          symbolName.c_str(), 0, highAddress
        );
      itr->second.unifiedCoverageMap = aCoverageMap;
      itr->second.stats.sizeInBytes = size;
    }
  }

  void DesiredSymbols::determineSourceLines(
    CoverageRanges* const theRanges,
    ExecutableInfo* const theExecutable

  )
  {
    for (auto& r : theRanges->set) {
      std::string location;
      theExecutable->getSourceAndLine(r.lowAddress, location);
      r.lowSourceLine = rld::path::basename (location);
      theExecutable->getSourceAndLine(r.highAddress, location);
      r.highSourceLine = rld::path::basename (location);
    }
  }

  SymbolInformation* DesiredSymbols::find(
    const std::string& symbolName
  )
  {
    if (set.find( symbolName ) == set.end())
      return NULL;
    else
      return &set[ symbolName ];
  }

  void DesiredSymbols::findSourceForUncovered( void )
  {
    DesiredSymbols::symbolSet_t::iterator ditr;
    CoverageRanges*                       theBranches;
    CoverageRanges*                       theRanges;

    // Process uncovered ranges and/or branches for each symbol.
    for (ditr = SymbolsToAnalyze->set.begin();
         ditr != SymbolsToAnalyze->set.end();
         ditr++) {

      // First the unexecuted ranges, ...
      theRanges = ditr->second.uncoveredRanges;
      if (theRanges == NULL)
        continue;

      if (!theRanges->set.empty()) {
        if (Verbose)
          fprintf(
            stderr,
            "Looking up source lines for uncovered ranges in %s\n",
            (ditr->first).c_str()
          );
        determineSourceLines(
          theRanges,
          ditr->second.sourceFile
        );
      }

      // then the uncovered branches.
      theBranches = ditr->second.uncoveredBranches;
      if (theBranches == NULL)
        continue;

      if (!theBranches->set.empty()) {
        if (Verbose)
          fprintf(
            stderr,
            "Looking up source lines for uncovered branches in %s\n",
            (ditr->first).c_str()
          );
        determineSourceLines(
          theBranches,
          ditr->second.sourceFile
        );
      }
    }
  }

  uint32_t DesiredSymbols::getNumberBranchesAlwaysTaken( void ) const {
    return stats.branchesAlwaysTaken;
  };

  uint32_t DesiredSymbols::getNumberBranchesFound( void ) const {
    return (stats.branchesNotExecuted + stats.branchesExecuted);
  };

  uint32_t DesiredSymbols::getNumberBranchesNeverTaken( void ) const {
    return stats.branchesNeverTaken;
  };

  uint32_t DesiredSymbols::getNumberUncoveredRanges( void ) const {
    return stats.uncoveredRanges;
  };

  bool DesiredSymbols::isDesired (
    const std::string& symbolName
  ) const
  {
    if (set.find( symbolName ) == set.end()) {
      #if 0
        fprintf( stderr,
          "Warning: Unable to find symbol %s\n",
          symbolName.c_str()
        );
      #endif
      return false;
    }
    return true;
  }

  void DesiredSymbols::mergeCoverageMap(
    const std::string&           symbolName,
    const CoverageMapBase* const sourceCoverageMap
  )
  {
    uint32_t              dAddress;
    CoverageMapBase*      destinationCoverageMap;
    uint32_t              dMapSize;
    symbolSet_t::iterator itr;
    uint32_t              sAddress;
    uint32_t              sBaseAddress;
    uint32_t              sMapSize;
    uint32_t              executionCount;

    // Ensure that the symbol is a desired symbol.
    itr = set.find( symbolName );

    if (itr == set.end()) {

      fprintf(
        stderr,
        "ERROR: DesiredSymbols::mergeCoverageMap - Unable to merge "
        "coverage map for %s because it is NOT a desired symbol\n",
        symbolName.c_str()
      );
      exit( -1 );
    }

    // Ensure that the source and destination coverage maps
    // are the same size.
    // Changed from ERROR msg to INFO, because size mismatch is not
    // treated as error anymore. 2015-07-20
    dMapSize = itr->second.stats.sizeInBytes;
    sBaseAddress = sourceCoverageMap->getFirstLowAddress();
    sMapSize = sourceCoverageMap->getSize();
    if (dMapSize != sMapSize) {

      fprintf(
        stderr,
        "INFO: DesiredSymbols::mergeCoverageMap - Unable to merge "
        "coverage map for %s because the sizes are different\n",
        symbolName.c_str()
      );
      return;
    }

    // Merge the data for each address.
    destinationCoverageMap = itr->second.unifiedCoverageMap;

    for (dAddress = 0; dAddress < dMapSize; dAddress++) {

      sAddress = dAddress + sBaseAddress;

      // Merge start of instruction indication.
      if (sourceCoverageMap->isStartOfInstruction( sAddress ))
        destinationCoverageMap->setIsStartOfInstruction( dAddress );

      // Merge the execution data.
      executionCount = sourceCoverageMap->getWasExecuted( sAddress );
      destinationCoverageMap->sumWasExecuted( dAddress, executionCount );

      // Merge the branch data.
      executionCount = sourceCoverageMap->getWasTaken( sAddress );
      destinationCoverageMap->sumWasTaken( dAddress, executionCount );

      executionCount = sourceCoverageMap->getWasNotTaken( sAddress );
      destinationCoverageMap->sumWasNotTaken( dAddress, executionCount );
    }
  }

}
