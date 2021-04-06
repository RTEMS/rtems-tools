/*! @file GcovFunctionData.h
 *  @brief GcovFunctionData Specification
 *
 *  This file contains the specification of the GcovGcnoWriter class.
 */

#ifndef __GCOV_FUNCTION_DATA_H__
#define __GCOV_FUNCTION_DATA_H__

#include <stdint.h>
#include <list>
#include "CoverageMapBase.h"
#include "DesiredSymbols.h"

namespace Gcov {

#define FUNCTION_NAME_LENGTH	        64
#define FILE_NAME_LENGTH		256

#define ON_TREE_ARC_FLAG		0x1
#define FAKE_ARC_FLAG			0x2
#define FALLTHROUGH_ARC_FLAG		0x4

struct gcov_arc_info
{
    uint32_t	sourceBlock;
    uint32_t	destinationBlock;
    uint32_t	flags;
    uint64_t	counter;
};

struct gcov_block_info
{
    uint32_t 			id;
    uint32_t			flags;
    uint32_t			numberOfLines;
    uint64_t 			counter;
    char			sourceFileName[FILE_NAME_LENGTH];
    std::list<uint32_t>		lines;
};

typedef std::list<gcov_arc_info>		arcs_t;
typedef std::list<gcov_arc_info>::iterator	arcs_iterator_t;
typedef std::list<gcov_block_info>		blocks_t;
typedef std::list<gcov_block_info>::iterator	blocks_iterator_t;

  /*! @class GcovFunctionData
   *
   *  This is the specification of the GcovFunctionData class.
   */
  class GcovFunctionData {

  public:

    /*!
     *  This method constructs a GcovFunctionData instance.
     */
	GcovFunctionData();

    /*!
     *  This method destructs a GcovFunctionData instance.
     */
    virtual ~GcovFunctionData();

    /*!
     *  This method stores checksum related to function
     *
     *  @param[in] chk stores the checksum value
     */
    void setChecksum(
            const uint32_t		chk
    );

    /*!
     *  This method stores id of function
     *
     *  @param[in] idNumber stores the id value
     */
    void setId(
            const uint32_t		idNumber
    );

    /*!
     *  This method stores checksum related to function
     *
     *  @param[in] lineNo passes number of the line begining the function
     */
    void setFirstLineNumber(
            const uint32_t		lineNo
    );

    /*!
     *  This method stores name of the function and ties it to its
     *  unified coverage map.
     *
     *  @param[in] functionName passes name of the the function
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    bool setFunctionName(
            const char*			fcnName
    );

    /*!
     *  This method stores name of the source file where function is located
     *
     *  @param[in] fileName passes name of the the file
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    bool setFileName(
            const char*			fileName
    );

    /*!
     *  This method stores name of the source file where block is located
     *
     *  @param[in] block identifies block
     *  @param[in] fileName passes name of the the file
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    void setBlockFileName(
            const blocks_iterator_t		block,
            const char*				fileName
    );

    /*!
     *  This method returns arcs list
     */
    arcs_t getArcs() const;

    /*!
     *  This method returns blocks list
     */
    blocks_t getBlocks() const;

    /*!
     *  This method returns checksum
     */
    uint32_t getChecksum() const;

    /*!
     *  This method returns id
     */
    uint32_t getId() const;

    /*!
     *  This method returns counters
     *
     *  @param[out] counterValues array of counter values
     *  @param[out] countersFound used to return counters number
     *  @param[out] countersSum used to return sum counters values
     *  @param[out] countersMax used to return max counter value
     */
    void getCounters( uint64_t* counterValues, uint32_t &countersFound, uint64_t &countersSum, uint64_t &countersMax );

    /*!
     *  This method adds new arc to arc list
     *
     *  @param[in] source passes source block number
     *  @param[in] destination passes destination block number
     */
    void addArc(
            uint32_t	source,
            uint32_t	destination,
            uint32_t	flags
    );

    /*!
     *  This method adds new arc to arc list
     *
     *  @param[in] block identifies block
     *  @param[in] line passes the line number
     */
    void addBlockLine(
            const blocks_iterator_t	block,
            const uint32_t		line
    );

    /*!
     *  This method finds block by its ID
     *
     *  @param[in] id passes block id number
     *
     *  @return Returns iterator to a matching block or NULL for error.
     */
    blocks_iterator_t findBlockById(
            const uint32_t		id
    );

    /*!
     *  This method adds new block to block list
     *
     *  @param[in] id passes block id number
     *  @param[in] flags passes per block flags
     *  @param[in] sourceFileName passes containing file name
     */
    void addBlock(
            const uint32_t		id,
            const uint32_t		flags,
            const char *		sourceFileName
    );

    /*!
     *  This method prints info about function
     */
    void printFunctionInfo( FILE * textFile, uint32_t function_number );

    /*!
     *  This method prints info about coverage of this function
     */
    void printCoverageInfo( FILE * textFile, uint32_t function_number );

    /*!
     *  This method prints info about chosen arc in arcs list
     *
     *  @param[in] textFile specifies output file
     *  @param[in] arc passes iterator identifying arc
     */
    void printArcInfo(
            FILE * textFile,
            arcs_iterator_t arc
    );

    /*!
     *  This method prints info about chosen block in blocks list
     *
     *  @param[in] block passes iterator identifying block
     */
    void printBlockInfo(
            FILE * textFile,
            blocks_iterator_t block
    );

    /*!
     *  This method calculates values of arc counters
     */
    bool processFunctionCounters( void );

  private:

    uint32_t		id;
    uint32_t		checksum;
    uint32_t		firstLineNumber;
    uint32_t		numberOfBlocks;
    uint32_t		numberOfArcs;
    arcs_t		arcs;
    blocks_t		blocks;
    char		functionName[FUNCTION_NAME_LENGTH];
    char		sourceFileName[FILE_NAME_LENGTH];

    /*!
     *  This member contains the unified or merged coverage map
     *  and symbol info for the symbol.
     */
    Coverage::CoverageMapBase*          coverageMap;
    Coverage::SymbolInformation* 	symbolInfo;

    /*!
     *  This method creates list of taken/not taken values
     *  for branches
     *
     *  @param[in] taken      used to return taken counts list
     *  @param[in] notTaken   used to return not taken counts list
     */
    bool processBranches(
            std::list<uint64_t> * taken ,
            std::list<uint64_t> * notTaken
    );
  };

}
#endif
