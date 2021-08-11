/*! @file DesiredSymbols.h
 *  @brief DesiredSymbols Specification
 *
 *  This file contains the specification of the DesiredSymbols class.
 */

#ifndef __DESIRED_SYMBOLS_H__
#define __DESIRED_SYMBOLS_H__

#include <list>
#include <map>
#include <stdint.h>
#include <string>

#include "CoverageMapBase.h"
#include "CoverageRanges.h"
#include "ExecutableInfo.h"
#include "ObjdumpProcessor.h"

namespace Coverage {

  class ObjdumpProcessor;
  struct objdumpLine_t;
  class ExecutableInfo;


  /*!
   *
   *  This class defines the statistics that are tracked.
   */
  class Statistics {
    public:

    /*!
     *  This member variable contains the total number of branches always
     *  taken.
     */
    int branchesAlwaysTaken;

    /*!
     *  This member variable contains the total number of branches where
     *  one or more paths were executed.
     */
    int branchesExecuted;

    /*!
     *  This member variable contains the total number of branches never
     *  taken.
     */
    int branchesNeverTaken;

    /*!
     *  This member variable contains the total number of branches not
     *  executed AT ALL.
     */
    int branchesNotExecuted;

    /*!
     *  This member contains the size in Bytes.
     */
    uint32_t sizeInBytes;

    /*!
     *  This member contains the size in Bytes not accounting for NOPs.
     */
    uint32_t sizeInBytesWithoutNops;

    /*!
     *  This member contains the size in instructions.
     */
    uint32_t sizeInInstructions;

    /*!
     *  This member variable contains the total number of uncovered bytes.
     */
    int uncoveredBytes;

    /*!
     *  This member variable contains the total number of uncovered assembly
     *  instructions.
     */
    int uncoveredInstructions;

    /*!
     *  This member variable contains the total number of uncovered ranges.
     */
    int uncoveredRanges;

    /*!
     *  This member variable contains the total number of unreferenced symbols.
     */
    int unreferencedSymbols;

    /*!
     *  This method returns the percentage of uncovered instructions.
     *
     *  @return Returns the percent uncovered instructions
     */
    uint32_t getPercentUncoveredInstructions( void ) const;

    /*!
     *  This method returns the percentage of uncovered bytes.
     *
     *  @return Returns the percent uncovered bytes
     */
     uint32_t getPercentUncoveredBytes( void ) const;

    /*!
     *  This method constructs a Statistics instance.
     */
     Statistics():
       branchesAlwaysTaken(0),
       branchesExecuted(0),
       branchesNeverTaken(0),
       branchesNotExecuted(0),
       sizeInBytes(0),
       sizeInBytesWithoutNops(0),
       sizeInInstructions(0),
       uncoveredBytes(0),
       uncoveredInstructions(0),
       uncoveredRanges(0),
       unreferencedSymbols(0)
     {
     }

  };

  /*! @class SymbolInformation
   *
   *  This class defines the information kept for each symbol that is
   *  to be analyzed.
   */
  class SymbolInformation {

  public:

    /*!
     *  This member contains the base address of the symbol.
     */
    uint32_t baseAddress;


    /*!
     *  This member contains the disassembly associated with a symbol.
     */
    std::list<objdumpLine_t> instructions;

    /*!
     *  This member contains the executable that was used to
     *  generate the disassembled instructions.
     */
    ExecutableInfo* sourceFile;

    /*!
     *  This member contains the statistics kept on each symbol.
     */
    Statistics stats;

    /*!
     *  This member contains information about the branch instructions of
     *  a symbol that were not fully covered (i.e. taken/not taken).
     */
    CoverageRanges* uncoveredBranches;

    /*!
     *  This member contains information about the instructions of a
     *  symbol that were not executed.
     */
    CoverageRanges* uncoveredRanges;

    /*!
     *  This member contains the unified or merged coverage map
     *  for the symbol.
     */
    CoverageMapBase* unifiedCoverageMap;

    /*!
     *  This method constructs a SymbolInformation instance.
     */
    SymbolInformation() :
      baseAddress( 0 ),
      sourceFile( NULL ),
      uncoveredBranches( NULL ),
      uncoveredRanges( NULL ),
      unifiedCoverageMap( NULL )
    {
    }

    ~SymbolInformation() {}
  };

  /*! @class DesiredSymbols
   *
   *  This class defines the set of desired symbols to analyze.
   */
  class DesiredSymbols {

  public:

    /*!
     *  This map associates each symbol with its symbol information.
     */
    typedef std::map<std::string, SymbolInformation> symbolSet_t;

    /*!
     *  This method constructs a DesiredSymbols instance.
     */
    DesiredSymbols();

    /*!
     *  This method destructs a DesiredSymbols instance.
     */
    ~DesiredSymbols();

    /*!
     *  The set of all symbols.
     */
    const symbolSet_t& allSymbols() const;

    /*!
     *  This method loops through the coverage map and
     *  calculates the statistics that have not already
     *  been filled in.
     */
    void calculateStatistics( void );

    /*!
     *  This method analyzes each symbols coverage map to determine any
     *  uncovered ranges or branches.
     *
     *  @param[in] verbose specifies whether to be verbose with output
     */
    void computeUncovered( bool verbose );

    /*!
     *  This method creates a coverage map for the specified symbol
     *  using the specified size.
     *
     *  @param[in] exefileName specifies the executable from which the
     *             coverage map is being created
     *  @param[in] symbolName specifies the symbol for which to create
     *             a coverage map
     *  @param[in] size specifies the size of the coverage map to create
     *  @param[in] sizeWithoutNops specifies the size without no ops
     *  @param[in] verbose specifies whether to be verbose with output
     */
    void createCoverageMap(
      const std::string& exefileName,
      const std::string& symbolName,
      uint32_t           size,
      uint32_t           sizeWithoutNops,
      bool               verbose
    );

    /*!
     *  This method looks up the symbol information for the specified symbol.
     *
     *  @param[in] symbolName specifies the symbol for which to search
     *
     *  @return Returns a pointer to the symbol's information
     */
    SymbolInformation* find(
      const std::string& symbolName
    );

    /*!
     *  This method determines the source lines that correspond to any
     *  uncovered ranges or branches.
     *
     *  @param[in] verbose specifies whether to be verbose with output
     *  @param[in] symbolsToAnalyze the symbols to be analyzed
     */
    void findSourceForUncovered(
      bool verbose,
      const DesiredSymbols& symbolsToAnalyze
    );

    /*!
     *  This method returns the total number of branches always taken
     *  for all analyzed symbols in a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns the total number of branches always taken
     */
    uint32_t getNumberBranchesAlwaysTaken(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns the total number of branches found for
     *  all analyzed symbols in a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns the total number of branches found
     */
    uint32_t getNumberBranchesFound(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns the total number of branches never taken
     *  for all analyzed symbols in a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns the total number of branches never taken
     */
    uint32_t getNumberBranchesNeverTaken(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns the total number of branches not executed
     *  for all analyzed symbols in a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns the total number of branches not executed
     */
    uint32_t getNumberBranchesNotExecuted(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns the total number of uncovered ranges
     *  for all analyzed symbols in a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns the total number of uncovered ranges
     */
    uint32_t getNumberUncoveredRanges(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns the total number of unreferenced symbols
     *  for all analyzed symbols in a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns the total number of unreferenced symbols
     */
    uint32_t getNumberUnreferencedSymbols(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns all symbol set names.
     *
     *  @return Returns all symbol set names
     */
    std::vector<std::string> getSetNames( void ) const;

    /*!
     *  This method returns all symbols for a given set.
     *
     *  @param[in] symbolSetName specifies the symbol set of interest
     *
     *  @return Returns all symbols for the given set
     */
    const std::vector<std::string>& getSymbolsForSet(
      const std::string& symbolSetName
    ) const;

    /*!
     *  This method returns an indication of whether or not the specified
     *  symbol is a symbol to analyze.
     *
     *  @return Returns TRUE if the specified symbol is a symbol to analyze
     *   and FALSE otherwise.
     */
    bool isDesired (
      const std::string& symbolName
    ) const;

    /*!
     *  This method creates the set of symbols to analyze from the symbols
     *  listed in the specified file.
     *
     *  @param[in] symbolsSet An INI format file of the symbols to be loaded.
     *  @param[in] buildTarget The build target
     *  @param[in] buildBSP The BSP
     */
    void load(
      const std::string& symbolsSet,
      const std::string& buildTarget,
      const std::string& buildBSP,
      bool               verbose
    );

    /*!
     *  This method merges the coverage information from the source
     *  coverage map into the unified coverage map for the specified symbol.
     *
     *  @param[in] symbolName specifies the symbol associated with the
     *             destination coverage map
     *  @param[in] sourceCoverageMap specifies the source coverage map
     */
    void mergeCoverageMap(
      const std::string&           symbolName,
      const CoverageMapBase* const sourceCoverageMap
    );

    /*!
     *  This method preprocesses each symbol's coverage map to mark nop
     *  and branch information.
     *
     *  @param[in] symbolsToAnalyze the symbols to be analyzed
     */
    void preprocess( const DesiredSymbols& symbolsToAnalyze );

  private:

    /*!
     *  This method uses the specified executable file to determine the
     *  source lines for the elements in the specified ranges.
     */
    void determineSourceLines(
      CoverageRanges* const theRanges,
      ExecutableInfo* const theExecutable
    );

    /*!
     *  This variable contains a map of symbol sets for each
     *  symbol in the system keyed on the symbol name.
     */
    symbolSet_t set;

    /*!
     *  This variable contains a map of symbol set names to symbol name lists.
     */
    std::map<std::string, std::vector<std::string>> setNamesToSymbols;

    /*!
     *  This member contains a map of symbol set names to statistics.
     */
    std::map<std::string, Statistics> stats;

  };
}

#endif
