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

  void DesiredSymbols::load(
    const std::string& symbolsSet,
    const std::string& buildTarget,
    const std::string& buildBSP,
    bool               verbose
  )
  {
    rld::files::cache cache;

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
        if (sym.type() == sym.st_func)
          set[sym.name()] = *(new SymbolInformation);
      }
      for (auto& kv : symbols.weaks()) {
        const rld::symbols::symbol& sym = *(kv.second);
        if (sym.type() == sym.st_func)
          set[sym.name()] = *(new SymbolInformation);
      }
    } catch (...) {
      cache.close();
      throw;
    }

    cache.close();
  }

  void DesiredSymbols::preprocess( void )
  {

    // Look at each symbol.
    for (auto& s : SymbolsToAnalyze->set) {
      // If the unified coverage map does not exist, the symbol was
      // never referenced by any executable.  Just skip it.
      CoverageMapBase* theCoverageMap = s.second.unifiedCoverageMap;
      if (theCoverageMap)
      {
        // Mark any branch and NOP instructions.
        for (auto& f : s.second.instructions) {
          if (f.isBranch)
            theCoverageMap->setIsBranch( f.address - s.second.baseAddress );
          if (f.isNop)
            theCoverageMap->setIsNop( f.address - s.second.baseAddress );
        }
      }
    }
  }

  void DesiredSymbols::calculateStatistics( void )
  {
    // Look at each symbol.
    for (auto& s : SymbolsToAnalyze->set) {
      // If the unified coverage map does not exist, the symbol was
      // never referenced by any executable.  Just skip it.
      CoverageMapBase* theCoverageMap = s.second.unifiedCoverageMap;
      if (theCoverageMap)
      {
        // Increment the total sizeInBytes byt the bytes in the symbol
        stats.sizeInBytes += s.second.stats.sizeInBytes;

        // Now scan through the coverage map of this symbol.
        uint32_t endAddress = s.second.stats.sizeInBytes - 1;

        for (uint32_t a = 0; a <= endAddress; ++a) {
          // If we are at the start of instruction increment
          // instruction type counters as needed.
          if ( theCoverageMap->isStartOfInstruction( a ) ) {

            stats.sizeInInstructions++;
            s.second.stats.sizeInInstructions++;

            if (!theCoverageMap->wasExecuted( a ) ) {
              stats.uncoveredInstructions++;
              s.second.stats.uncoveredInstructions++;

              if ( theCoverageMap->isBranch( a )) {
                stats.branchesNotExecuted++;
                s.second.stats.branchesNotExecuted++;
              }
            } else if (theCoverageMap->isBranch( a )) {
              stats.branchesExecuted++;
              s.second.stats.branchesExecuted++;
            }
          }

          if (!theCoverageMap->wasExecuted( a )) {
            stats.uncoveredBytes++;
            s.second.stats.uncoveredBytes++;
          }
        }
      }
    }
  }


  void DesiredSymbols::computeUncovered( void )
  {
    // Look at each symbol.
    for (auto& s : SymbolsToAnalyze->set) {
      // If the unified coverage map does not exist, the symbol was
      // never referenced by any executable.  Just skip it.
      CoverageMapBase* theCoverageMap = s.second.unifiedCoverageMap;
      if (theCoverageMap)
      {
        // Create containers for the symbol's uncovered ranges and branches.
        CoverageRanges* theRanges = new CoverageRanges();
        s.second.uncoveredRanges = theRanges;
        CoverageRanges* theBranches = new CoverageRanges();
        s.second.uncoveredBranches = theBranches;

        uint32_t a;
        uint32_t la;
        uint32_t ha;
        uint32_t endAddress;
        uint32_t count;

        // Mark NOPs as executed
        endAddress = s.second.stats.sizeInBytes - 1;
        a = 0;
        while (a < endAddress) {
          if (!theCoverageMap->wasExecuted( a )) {
            a++;
            continue;
          }

          for (ha=a+1;
               ha <= endAddress && !theCoverageMap->isStartOfInstruction( ha );
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
        endAddress = s.second.stats.sizeInBytes - 1;
        a = 0;
        while (a <= endAddress) {
          // If an address was NOT executed, find consecutive unexecuted
          // addresses and add them to the uncovered ranges.
          if (!theCoverageMap->wasExecuted( a )) {

            la = a;
            count = 1;
            for (ha = a + 1;
                 ha <= endAddress && !theCoverageMap->wasExecuted( ha );
                 ha++)
            {
              if ( theCoverageMap->isStartOfInstruction( ha ) )
                count++;
            }
            ha--;

            stats.uncoveredRanges++;
            s.second.stats.uncoveredRanges++;
            theRanges->add(
              s.second.baseAddress + la,
              s.second.baseAddress + ha,
              CoverageRanges::UNCOVERED_REASON_NOT_EXECUTED,
              count
            );
            a = ha + 1;
          }

          // If an address is a branch instruction, add any uncovered branches
          // to the uncoverd branches.
          else if (theCoverageMap->isBranch( a )) {
            la = a;
            for (ha = a + 1;
               ha <= endAddress && !theCoverageMap->isStartOfInstruction( ha );
                 ha++)
              ;
            ha--;

            if (theCoverageMap->wasAlwaysTaken( la )) {
              stats.branchesAlwaysTaken++;
              s.second.stats.branchesAlwaysTaken++;
              theBranches->add(
                s.second.baseAddress + la,
                s.second.baseAddress + ha,
                CoverageRanges::UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN,
                1
              );
              if (Verbose)
                std::cerr << "Branch always taken found in" << s.first
                          << std::hex
                          << " (0x" << s.second.baseAddress + la
                          << " - 0x" << s.second.baseAddress + ha
                          << ")"
                          << std::dec
                          << std::endl;
            }
            else if (theCoverageMap->wasNeverTaken( la )) {
              stats.branchesNeverTaken++;
              s.second.stats.branchesNeverTaken++;
              theBranches->add(
                s.second.baseAddress + la,
                s.second.baseAddress + ha,
                CoverageRanges::UNCOVERED_REASON_BRANCH_NEVER_TAKEN,
                1
                );
              if (Verbose)
                std::cerr << "Branch never taken found in " << s.first
                          << " (0x" << s.second.baseAddress + la
                          << " - 0x" << s.second.baseAddress + ha
                          << ")"
                          << std::dec
                          << std::endl;
            }
            a = ha + 1;
          }
          else
            a++;
        }
      }
    }
  }


  void DesiredSymbols::createCoverageMap(
    const std::string& exefileName,
    const std::string& symbolName,
    uint32_t           size
  )
  {
    CoverageMapBase* aCoverageMap;
    uint32_t         highAddress;

    // Ensure that the symbol is a desired symbol.
    symbolSet_t::iterator itr = set.find( symbolName );

    if (itr == set.end()) {
      std::ostringstream what;
      what << "Unable to create unified coverage map for "
           << symbolName
           << " because it is NOT a desired symbol";
      throw rld::error( what, "DesiredSymbols::createCoverageMap" );
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
        std::cerr << "INFO: DesiredSymbols::createCoverageMap - Attempt to create "
                  << "unified coverage maps for "
                  << symbolName
                  << " with different sizes ("
                  << rld::path::basename(exefileName) << '/' << itr->second.stats.sizeInBytes
                  << " != "
                  << rld::path::basename(itr->second.sourceFile->getFileName())
                  << '/' << size << ')'
                  << std::endl;

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
    // Process uncovered ranges and/or branches for each symbol.
    for (auto& d : SymbolsToAnalyze->set) {
      // First the unexecuted ranges, ...
      CoverageRanges* theRanges = d.second.uncoveredRanges;
      if (theRanges != nullptr) {
        if (!theRanges->set.empty()) {
          if (Verbose)
            std::cerr << "Looking up source lines for uncovered ranges in "
                      << d.first
                      << std::endl;
            determineSourceLines( theRanges, d.second.sourceFile );
        }

        // then the uncovered branches.
        CoverageRanges* theBranches = d.second.uncoveredBranches;
        if (theBranches != nullptr) {
          if (!theBranches->set.empty()) {
            if (Verbose)
              std::cerr << "Looking up source lines for uncovered branches in "
                        << d.first
                        << std::endl;
            determineSourceLines( theBranches, d.second.sourceFile );
          }
        }
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
    return set.find( symbolName ) == set.end() ? false : true;
  }

  void DesiredSymbols::mergeCoverageMap(
    const std::string&           symbolName,
    const CoverageMapBase* const sourceCoverageMap
  )
  {
    // Ensure that the symbol is a desired symbol.
    symbolSet_t::iterator itr = set.find( symbolName );

    if (itr == set.end()) {
      std::ostringstream what;
      what << "Unable to merge coverage map for "
           << symbolName
           << " because it is NOT a desired symbol";
      throw rld::error( what, "DesiredSymbols::mergeCoverageMap" );
    }

    SymbolInformation& sinfo = itr->second;

    // Ensure that the source and destination coverage maps
    // are the same size.
    // Changed from ERROR msg to INFO, because size mismatch is not
    // treated as error anymore. 2015-07-20
    uint32_t dMapSize = sinfo.stats.sizeInBytes;
    uint32_t sBaseAddress = sourceCoverageMap->getFirstLowAddress();
    uint32_t sMapSize = sourceCoverageMap->getSize();
    if (dMapSize != 0 && dMapSize != sMapSize) {
      std::cerr << "INFO: DesiredSymbols::mergeCoverageMap - Unable to merge "
                << "coverage map for " << symbolName
                << " because the sizes are different ("
                << "size: " << dMapSize << ", source: " << sMapSize << ')'
                << std::endl;
      return;
    }

    // Merge the data for each address.
    CoverageMapBase* destinationCoverageMap = sinfo.unifiedCoverageMap;

    for (uint32_t dAddress = 0; dAddress < dMapSize; dAddress++) {

      uint32_t sAddress = dAddress + sBaseAddress;

      // Merge start of instruction indication.
      if (sourceCoverageMap->isStartOfInstruction( sAddress ))
        destinationCoverageMap->setIsStartOfInstruction( dAddress );

      // Merge the execution data.
      uint32_t executionCount = sourceCoverageMap->getWasExecuted( sAddress );
      destinationCoverageMap->sumWasExecuted( dAddress, executionCount );

      // Merge the branch data.
      executionCount = sourceCoverageMap->getWasTaken( sAddress );
      destinationCoverageMap->sumWasTaken( dAddress, executionCount );

      executionCount = sourceCoverageMap->getWasNotTaken( sAddress );
      destinationCoverageMap->sumWasNotTaken( dAddress, executionCount );
    }
  }
}
