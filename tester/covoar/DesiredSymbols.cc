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
#include "CoverageMap.h"

namespace Coverage {

  DesiredSymbols::DesiredSymbols()
  {
  }

  DesiredSymbols::~DesiredSymbols()
  {
  }

  const DesiredSymbols::symbolSet_t& DesiredSymbols::allSymbols() const
  {
    return set;
  }

  void DesiredSymbols::load(
    const std::string& symbolsSet,
    const std::string& buildTarget,
    const std::string& buildBSP,
    bool               verbose
  )
  {
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
    rld::config::config config;

    if (verbose)
      std::cerr << "Loading symbol sets: " << symbolsSet << std::endl;

    config.load (symbolsSet);

    const rld::config::section& sym_section = config.get_section("symbol-sets");

    rld::strings sets;
    rld::config::parse_items (sym_section, "sets", sets, true);

    // Load the symbols for each set specified in the config file.
    for (const auto& setName : sets) {
      rld::files::cache cache;
      cache.open();

      if (verbose)
        std::cerr << "Loading symbols for set: " << setName << std::endl;
      const rld::config::section& set_section = config.get_section(setName);
      rld::strings libs;
      rld::config::parse_items (set_section, "libraries", libs, true);
      for (std::string lib : libs) {
        lib = rld::find_replace(lib, "@BUILD-TARGET@", buildTarget);
        lib = rld::find_replace(lib, "@BSP@", buildBSP);
        if (verbose)
          std::cerr << " Loading library: " << lib << std::endl;
        cache.add(lib);
      }

      rld::symbols::table symbols;

      cache.load_symbols (symbols, true);

      // Populate the symbol maps with all global symbols.
      for (auto& kv : symbols.globals()) {
        const rld::symbols::symbol& sym = *(kv.second);
        if (sym.type() == sym.st_func) {
          set[sym.name()] = SymbolInformation();
          setNamesToSymbols[setName].push_back(sym.name());
        }
      }
      // Populate the symbol maps with all weak symbols.
      for (auto& kv : symbols.weaks()) {
        const rld::symbols::symbol& sym = *(kv.second);
        if (sym.type() == sym.st_func) {
          set[sym.name()] = SymbolInformation();
          setNamesToSymbols[setName].push_back(sym.name());
        }
      }
    }
  }

  void DesiredSymbols::preprocess( const DesiredSymbols& symbolsToAnalyze )
  {
    // Look at each symbol.
    for (auto& s : symbolsToAnalyze.set) {
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
    // Look at each symbol set.
    for (const auto& kv : setNamesToSymbols) {
      // Look at each symbol.
      for (const auto& symbol : kv.second) {
        SymbolInformation& info = set.at(symbol);

        // If the unified coverage map does not exist, the symbol was
        // never referenced by any executable.  Just skip it.
        CoverageMapBase* theCoverageMap = info.unifiedCoverageMap;
        if (theCoverageMap) {
          // Increment the total sizeInBytes by the bytes in the symbol
          stats[kv.first].sizeInBytes += info.stats.sizeInBytes;

          // Now scan through the coverage map of this symbol.
          uint32_t endAddress = info.stats.sizeInBytes - 1;

          for (uint32_t a = 0; a <= endAddress; ++a) {
            // If we are at the start of instruction increment
            // instruction type counters as needed.
            if ( theCoverageMap->isStartOfInstruction( a ) ) {

              stats[kv.first].sizeInInstructions++;
              info.stats.sizeInInstructions++;

              if (!theCoverageMap->wasExecuted( a ) ) {
                stats[kv.first].uncoveredInstructions++;
                info.stats.uncoveredInstructions++;

                if ( theCoverageMap->isBranch( a )) {
                  stats[kv.first].branchesNotExecuted++;
                  info.stats.branchesNotExecuted++;
                }
              } else if (theCoverageMap->isBranch( a )) {
                stats[kv.first].branchesExecuted++;
                info.stats.branchesExecuted++;
              }
            }

            if (!theCoverageMap->wasExecuted( a )) {
              stats[kv.first].uncoveredBytes++;
              info.stats.uncoveredBytes++;
            }
          }
        } else {
          stats[kv.first].unreferencedSymbols++;
        }
      }
    }
  }


  void DesiredSymbols::computeUncovered( bool verbose )
  {
    // Look at each symbol set.
    for (const auto& kv : setNamesToSymbols) {
      // Look at each symbol.
      for (const auto& symbol : kv.second) {
        SymbolInformation& info = set.at(symbol);
        // If the unified coverage map does not exist, the symbol was
        // never referenced by any executable.  Just skip it.
        CoverageMapBase* theCoverageMap = info.unifiedCoverageMap;
        if (theCoverageMap) {
          // Create containers for the symbol's uncovered ranges and branches.
          CoverageRanges* theRanges = new CoverageRanges();
          info.uncoveredRanges = theRanges;
          CoverageRanges* theBranches = new CoverageRanges();
          info.uncoveredBranches = theBranches;

          uint32_t a;
          uint32_t la;
          uint32_t ha;
          uint32_t endAddress;
          uint32_t count;

          // Mark NOPs as executed
          a = info.stats.sizeInBytes - 1;
          count = 0;
          while (a > 0) {
            if (theCoverageMap->isStartOfInstruction( a )) {
              break;
            }

            count++;

            if (theCoverageMap->isNop( a )) {
              for (la = a; la < (a + count); la++) {
                theCoverageMap->setWasExecuted( la );
              }

              count = 0;
            }

            a--;
          }

          endAddress = info.stats.sizeInBytes - 1;
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
                if ( ha > endAddress )
                  break;
              } while ( !theCoverageMap->isStartOfInstruction( ha ) ||
                        theCoverageMap->isNop( ha ) );
            a = ha;
          }

          // Now scan through the coverage map of this symbol.
          endAddress = info.stats.sizeInBytesWithoutNops - 1;
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

              stats[kv.first].uncoveredRanges++;
              info.stats.uncoveredRanges++;
              theRanges->add(
                info.baseAddress + la,
                info.baseAddress + ha,
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
                stats[kv.first].branchesAlwaysTaken++;
                info.stats.branchesAlwaysTaken++;
                theBranches->add(
                  info.baseAddress + la,
                  info.baseAddress + ha,
                  CoverageRanges::UNCOVERED_REASON_BRANCH_ALWAYS_TAKEN,
                  1
                );
                if (verbose)
                  std::cerr << "Branch always taken found in" << symbol
                            << std::hex
                            << " (0x" << info.baseAddress + la
                            << " - 0x" << info.baseAddress + ha
                            << ")"
                            << std::dec
                            << std::endl;
              }
              else if (theCoverageMap->wasNeverTaken( la )) {
                stats[kv.first].branchesNeverTaken++;
                info.stats.branchesNeverTaken++;
                theBranches->add(
                  info.baseAddress + la,
                  info.baseAddress + ha,
                  CoverageRanges::UNCOVERED_REASON_BRANCH_NEVER_TAKEN,
                  1
                  );
                if (verbose)
                  std::cerr << "Branch never taken found in " << symbol
                            << std::hex
                            << " (0x" << info.baseAddress + la
                            << " - 0x" << info.baseAddress + ha
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
  }


  void DesiredSymbols::createCoverageMap(
    const std::string& exefileName,
    const std::string& symbolName,
    uint32_t           size,
    uint32_t           sizeWithoutNops,
    bool               verbose
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

        if ( itr->second.stats.sizeInBytes < size ) {
          itr->second.stats.sizeInBytes = size;
          itr->second.stats.sizeInBytesWithoutNops = sizeWithoutNops;
        } else
          size = itr->second.stats.sizeInBytes;
      }
    }

    // If we don't already have a coverage map, create one.
    else {

      highAddress = size - 1;

      aCoverageMap = new CoverageMap( exefileName, 0, highAddress );

      if ( verbose )
        fprintf(
          stderr,
          "Created unified coverage map for %s (0x%x - 0x%x)\n",
          symbolName.c_str(), 0, highAddress
        );
      itr->second.unifiedCoverageMap = aCoverageMap;
      itr->second.stats.sizeInBytes = size;
      itr->second.stats.sizeInBytesWithoutNops = sizeWithoutNops;
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

  void DesiredSymbols::findSourceForUncovered(
    bool                  verbose,
    const DesiredSymbols& symbolsToAnalyze
  )
  {
    // Process uncovered ranges and/or branches for each symbol.
    for (auto& d : symbolsToAnalyze.set) {
      // First the unexecuted ranges, ...
      CoverageRanges* theRanges = d.second.uncoveredRanges;
      if (theRanges != nullptr) {
        if (!theRanges->set.empty()) {
          if (verbose)
            std::cerr << "Looking up source lines for uncovered ranges in "
                      << d.first
                      << std::endl;
          determineSourceLines( theRanges, d.second.sourceFile );
        }

        // then the uncovered branches.
        CoverageRanges* theBranches = d.second.uncoveredBranches;
        if (theBranches != nullptr) {
          if (!theBranches->set.empty()) {
            if (verbose)
              std::cerr << "Looking up source lines for uncovered branches in "
                        << d.first
                        << std::endl;
            determineSourceLines( theBranches, d.second.sourceFile );
          }
        }
      }
    }
  }

  uint32_t DesiredSymbols::getNumberBranchesAlwaysTaken(
    const std::string& symbolSetName
  ) const {
    return stats.at(symbolSetName).branchesAlwaysTaken;
  };

  uint32_t DesiredSymbols::getNumberBranchesFound(
    const std::string& symbolSetName
  ) const {
    return (
      stats.at(symbolSetName).branchesNotExecuted +
      stats.at(symbolSetName).branchesExecuted
    );
  };

  uint32_t DesiredSymbols::getNumberBranchesNeverTaken(
    const std::string& symbolSetName
  ) const {
    return stats.at(symbolSetName).branchesNeverTaken;
  };

  uint32_t DesiredSymbols::getNumberBranchesNotExecuted(
    const std::string& symbolSetName
  ) const {
    return stats.at(symbolSetName).branchesNotExecuted;
  };

  uint32_t DesiredSymbols::getNumberUncoveredRanges(
    const std::string& symbolSetName
  ) const {
    return stats.at(symbolSetName).uncoveredRanges;
  };

  uint32_t DesiredSymbols::getNumberUnreferencedSymbols(
    const std::string& symbolSetName
  ) const {
    return stats.at(symbolSetName).unreferencedSymbols;
  };

  std::vector<std::string> DesiredSymbols::getSetNames( void ) const {
    std::vector<std::string> setNames;
    for (const auto &kv : setNamesToSymbols) {
      setNames.push_back(kv.first);
    }

    return setNames;
  }

  const std::vector<std::string>& DesiredSymbols::getSymbolsForSet(
    const std::string& symbolSetName
  ) const
  {
    return setNamesToSymbols.at(symbolSetName);
  }

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
    uint32_t dMapSize = sinfo.stats.sizeInBytesWithoutNops;
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
