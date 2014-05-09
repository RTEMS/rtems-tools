/*! @file CoverageMapBase.h
 *  @brief CoverageMapBase Specification
 *
 *  This file contains the specification of the CoverageMapBase class.
 */

#ifndef __COVERAGE_MAP_BASE_H__
#define __COVERAGE_MAP_BASE_H__

#include <stdint.h>
#include <string>
#include <list>

namespace Coverage {

  /*! @class CoverageMapBase
   *
   *  This is the base class for Coverage Map implementations.
   */
  class CoverageMapBase {

  public:

    /*!
     *  This structure identifies the low and high addresses
     *  of one range.  Note:: There may be more than one address 
     *  range per symbol.
     */
    typedef struct {
      /*!
       *  This is the low address of the address map range.
       */
      uint32_t lowAddress;

      /*!
       *  This is the high address of the address map range.
       */
      uint32_t highAddress;

    } AddressRange_t;

    /*
     *  This type identifies a list of ranges.
     */
    typedef std::list< AddressRange_t > 	  AddressRange;
    typedef std::list< AddressRange_t >::iterator AddressRangeIterator_t;

    /*! 
     *  This method constructs a CoverageMapBase instance.
     *
     *  @param[in] low specifies the lowest address of the coverage map
     *  @param[in] high specifies the highest address of the coverage map
     */
    CoverageMapBase(
      uint32_t low,
      uint32_t high
    );

    /*! 
     *  This method destructs a CoverageMapBase instance.
     */
    virtual ~CoverageMapBase();

    /*!
     *  This method adds a address range to the RangeList.
     *
     *  @param[in]  Low specifies the lowAddress
     *  @param[in]  High specifies the highAddress
     *  
     */
    void Add( uint32_t low, uint32_t high );
 
    /*!
     *  This method returns true and sets the offset if
     *  the address falls with the bounds of an address range 
     *  in the RangeList.
     *
     *  @param[in]  address specifies the address to find
     *  @param[out] offset contains the offset from the low
     *              address of the address range.
     *  
     *  @return Returns TRUE if the address range can be found
     *   and FALSE if it was not.
      */
    bool determineOffset( uint32_t address, uint32_t *offset ) const;

    /*!
     *  This method prints the contents of the coverage map to stdout.
     */
    void dump( void ) const;

    /*!
     *  This method will return the low address of the first range in
     *  the RangeList.
     *
     *  @return Returns the low address of the first range in
     *  the RangeList.
     */
    int32_t getFirstLowAddress() const;

    /*!
     *  This method returns true and sets the address range if
     *  the address falls with the bounds of an address range 
     *  in the RangeList.
     *
     *  @param[in]  address specifies the address to find
     *  @param[out] range contains the high and low addresse for
     *              the range
     *  
     *  @return Returns TRUE if the address range can be found
     *   and FALSE if it was not.
     */
    bool getRange( uint32_t address, AddressRange_t *range ) const;

    /*!
     *  This method returns the size of the address range.
     * 
     *  @return Returns Size of the address range.
     */
    uint32_t getSize() const;


    /*!
     *  This method returns the address of the beginning of the
     *  instruction that contains the specified address.
     *
     *  @param[in] address specifies the address to search from
     *  @param[out] beginning contains the address of the beginning of
     *              the instruction.
     *  
     *  @return Returns TRUE if the beginning of the instruction was
     *   found and FALSE if it was not.
     */
    bool getBeginningOfInstruction(
      uint32_t  address,
      uint32_t* beginning
    ) const;

    /*!
     *  This method returns the high address of the coverage map.
     *
     *  @return Returns the high address of the coverage map.
    uint32_t getHighAddress( void ) const;
     */

    /*!
     *  This method returns the low address of the coverage map.
     *
     *  @return Returns the low address of the coverage map.
    uint32_t getLowAddress( void ) const;
     */

    /*!
     *  This method sets the boolean which indicates if this
     *  is the starting address for an instruction.
     *
     *  @param[in] address specifies the address of the start of an instruction
     */
    void setIsStartOfInstruction(
      uint32_t address
    );

    /*!
     *  This method returns a boolean which indicates if this
     *  is the starting address of an instruction.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if the specified address is the start
     *   of an instruction and FALSE otherwise.
     */
    bool isStartOfInstruction( uint32_t address ) const;

    /*!
     *  This method increments the counter which indicates how many times
     *  the instruction at the specified address was executed.
     *
     *  @param[in] address specifies the address which was executed
     */
    virtual void setWasExecuted( uint32_t address );

    /*!
     *  This method returns a boolean which indicates if the instruction
     *  at the specified address was executed.
     *
     *  @param[in] address specifies the address to check
     *  
     *  @return Returns TRUE if the instruction at the specified
     *   address was executed and FALSE otherwise.
     */
    bool wasExecuted( uint32_t address ) const;

    /*!
     *  This method increases the counter which indicates how many times
     *  the instruction at the specified address was executed. It is used
     *  for merging coverage maps.
     *
     *  @param[in] address specifies the address which was executed
     *  @param[in] address specifies the execution count that should be
     *             added
     */
    virtual void sumWasExecuted( uint32_t address, uint32_t addition );

    /*!
     *  This method returns an unsigned integer which indicates how often
     *  the instruction at the specified address was executed.
     *
     *  @param[in] address specifies the address to check
     *  
     *  @return Returns number of executins
     */
    uint32_t getWasExecuted( uint32_t address ) const;

    /*!
     *  This method sets the boolean which indicates if the specified
     *  address is the starting address of a branch instruction.
     *
     *  @param[in] address specifies the address of the branch instruction
     */
    void setIsBranch( uint32_t address );

    /*!
     *  This method returns a boolean which indicates if the specified
     *  address is the starting address of a NOP instruction.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if a NOP instruction is at the
     *   specified address and FALSE otherwise.
     */
    bool isNop( uint32_t address ) const;

    /*!
     *  This method sets the boolean which indicates if the specified
     *  address is the starting address of a NOP instruction.
     *
     *  @param[in] address specifies the address of the NOP instruction
     */
    void setIsNop( uint32_t address );

    /*!
     *  This method returns a boolean which indicates if the specified
     *  address is the starting address of a branch instruction.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if a branch instruction is at the
     *   specified address and FALSE otherwise.
     */
    bool isBranch( uint32_t address ) const;

    /*!
     *  This method increments the counter which indicates how many times
     *  the branch at the specified address was taken.
     *
     *  @param[in] address specifies the address of the branch instruction
     */
    void setWasTaken( uint32_t address );

    /*!
     *  This method increases the counter which indicates how many times
     *  the branch at the specified address was taken. It is used
     *  for merging coverage maps.
     *
     *  @param[in] address specifies the address which was executed
     *  @param[in] address specifies the execution count that should be
     *             added
     */
    virtual void sumWasTaken( uint32_t address, uint32_t addition );

    /*!
     *  This method returns an unsigned integer which indicates how often
     *  the branch at the specified address was taken.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns number of executins
     */
    uint32_t getWasTaken( uint32_t address ) const;

    /*!
     *  This method increments the counter which indicates how many times
     *  the branch at the specified address was not taken.
     *
     *  @param[in] address specifies the address of the branch instruction
     */
    void setWasNotTaken( uint32_t address );

    /*!
     *  This method increases the counter which indicates how many times
     *  the branch at the specified address was not taken. It is used
     *  for merging coverage maps.
     *
     *  @param[in] address specifies the address which was executed
     *  @param[in] address specifies the execution count that should be
     *             added
     */
    virtual void sumWasNotTaken( uint32_t address, uint32_t addition );

    /*!
     *  This method returns an unsigned integer which indicates how often
     *  the branch at the specified address was not taken.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns number of executins
     */
    uint32_t getWasNotTaken( uint32_t address ) const;


    /*!
     *  This method returns a boolean which indicates if the branch
     *  instruction at the specified address is ALWAYS taken.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if the branch instruction at the
     *   specified address is ALWAYS taken and FALSE otherwise.
     */
    bool wasAlwaysTaken( uint32_t address ) const;

    /*!
     *  This method returns a boolean which indicates if the branch
     *  instruction at the specified address is NEVER taken.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if the branch instruction at the
     *  specified address is NEVER taken and FALSE otherwise.
     */
    bool wasNeverTaken( uint32_t address ) const;

    /*!
     *  This method returns a boolean which indicates if the branch
     *  instruction at the specified address was NOT taken.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if the branch instruction at the
     *   specified address was NOT taken and FALSE otherwise.
     */
    bool wasNotTaken( uint32_t address ) const;

    /*!
     *  This method returns a boolean which indicates if the branch
     *  instruction at the specified address was taken.
     *
     *  @param[in] address specifies the address to check
     *
     *  @return Returns TRUE if the branch instruction at the
     *  specified address was taken and FALSE otherwise.
     */
    bool wasTaken( uint32_t address ) const;

  protected:

    /*!
     *  This structure defines the information that is gathered and
     *  tracked per address.
     */
    typedef struct {
      /*!
       *  This member indicates that the address is the start of
       *  an instruction.
       */
      bool isStartOfInstruction;
      /*!
       *  This member indicates how many times the address was executed.
       */
      uint32_t wasExecuted;
      /*!
       *  This member indicates that the address is a branch instruction.
       */
      bool isBranch;
      /*!
       *  This member indicates that the address is a NOP instruction.
       */
      bool isNop;
      /*!
       *  When isBranch is TRUE, this member indicates that the branch
       *  instruction at the address was taken.
       */
      uint32_t wasTaken;
      /*!
       *  When isBranch is TRUE, this member indicates that the branch
       *  instruction at the address was NOT taken.
       */
      uint32_t wasNotTaken;
    } perAddressInfo_t;

    /*!
     * 
     *  This is a list of address ranges for this symbolic address.
     */
    AddressRange RangeList;

    /*!
     *  
     *  This variable contains the size of the code block.
     */
    uint32_t Size;

    /*!
     *  This is a dynamically allocated array of data that is
     *  kept for each address.
     */
    perAddressInfo_t* Info;
  };

}
#endif
