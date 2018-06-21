
/*! @file CoverageMapBase.cc
 *  @brief CoverageMapBase Implementation
 *
 *  This file contains the implementation of the functions
 *  which provide a base level of functionality of a CoverageMap.
 */

#include <limits.h>

#include <iostream>
#include <iomanip>

#include "CoverageMapBase.h"

namespace Coverage {

  CoverageMapBase::CoverageMapBase(
    const std::string& exefileName,
    uint32_t           low,
    uint32_t           high
  )
  {
    uint32_t     a;
    AddressRange range;

    range.fileName    = exefileName;
    range.lowAddress  = low;
    range.highAddress = high;
    Ranges.push_back( range );

    Size = high - low + 1;

    Info = new perAddressInfo[ Size ];

    for (a = 0; a < Size; a++) {

      perAddressInfo *i = &Info[ a ];

      i->isStartOfInstruction = false;
      i->wasExecuted          = 0;
      i->isBranch             = false;
      i->isNop                = false;
      i->wasTaken             = 0;
      i->wasNotTaken          = 0;
    }
  }

  CoverageMapBase::~CoverageMapBase()
  {
    if (Info)
      delete Info;
  }

  void  CoverageMapBase::Add( uint32_t low, uint32_t high )
  {
    AddressRange range;

    range.lowAddress  = low;
    range.highAddress = high;
    Ranges.push_back( range );
  }

  bool CoverageMapBase::determineOffset(
    uint32_t  address,
    uint32_t *offset
  )const
  {
    AddressRanges::const_iterator  itr;

    for ( auto& r : Ranges ) {
      if ((address >= r.lowAddress) && (address <= r.highAddress)){
        *offset = address - r.lowAddress;
        return true;
      }
    }
    *offset = 0;
    return false;
  }


  void CoverageMapBase::dump( void ) const
  {
    fprintf( stderr, "Coverage Map Contents:\n" );
    /*
     * XXX - Dump is only marking the first Address Range.
     */
    for (uint32_t a = 0; a < Size; a++) {
      perAddressInfo* entry = &Info[ a ];
      std::cerr << std::hex << std::setfill('0')
                << "0x" << a + Ranges.front().lowAddress
                << "- isStartOfInstruction:"
                << (char*) (entry->isStartOfInstruction ? "yes" : "no")
                << " wasExecuted:"
                << (char*) (entry->wasExecuted ? "yes" : "no")
                << std::endl
                << "           isBranch:"
                << (char*) (entry->isBranch ? "yes" : "no")
                << " wasTaken:"
                << (char*) (entry->wasTaken ? "yes" : "no")
                << " wasNotTaken:"
                << (char*) (entry->wasNotTaken ? "yes" : "no")
                << std::dec << std::setfill(' ')
                << std::endl;
    }
  }

  bool CoverageMapBase::getBeginningOfInstruction(
    uint32_t  address,
    uint32_t* beginning
  ) const
  {
    bool         status = false;
    uint32_t     start;
    AddressRange range;


    status = getRange( address, &range );
    if ( status != true )
      return status;

    start = address;

    while (start >= range.lowAddress ) {
      if (Info[ start - range.lowAddress ].isStartOfInstruction) {
        *beginning = start;
        status = true;
        break;
      }
      else
        start--;
    }

    return status;
  }

  int32_t CoverageMapBase::getFirstLowAddress() const
  {
    return Ranges.front().lowAddress;
  }

  bool CoverageMapBase::getRange( uint32_t address, AddressRange *range ) const
  {
    for ( auto r : Ranges ) {
      if ((address >= r.lowAddress) && (address <= r.highAddress)){
        range->lowAddress = r.lowAddress;
        range->highAddress = r.highAddress;
        return true;
      }
    }

    range->lowAddress  = 0;
    range->highAddress = 0;

    return false;

  }

  uint32_t CoverageMapBase::getSize() const
  {
    return Size;
  }

  void CoverageMapBase::setIsStartOfInstruction(
    uint32_t    address
  )
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].isStartOfInstruction = true;
  }

  bool CoverageMapBase::isStartOfInstruction( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return false;

    return Info[ offset ].isStartOfInstruction;
  }

  void CoverageMapBase::setWasExecuted( uint32_t address )
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].wasExecuted += 1;
  }

  void CoverageMapBase::sumWasExecuted( uint32_t address, uint32_t addition)
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].wasExecuted += addition;
  }

  bool CoverageMapBase::wasExecuted( uint32_t address ) const
  {
    uint32_t offset;
    bool     result;

    result = true;

    if (determineOffset( address, &offset ) != true)
      result = false;

    if (Info[ offset ].wasExecuted <= 0)
      result = false;

    return result;
  }

  uint32_t CoverageMapBase::getWasExecuted( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return 0;

    return Info[ offset ].wasExecuted;
  }

  void CoverageMapBase::setIsBranch(
    uint32_t    address
  )
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].isBranch = true;
  }

  bool CoverageMapBase::isNop( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return false;

    return Info[ offset ].isNop;
  }

  void CoverageMapBase::setIsNop(
    uint32_t    address
  )
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].isNop = true;
  }

  bool CoverageMapBase::isBranch( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return false;

    return Info[ offset ].isBranch;
  }

  void CoverageMapBase::setWasTaken(
    uint32_t    address
  )
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].wasTaken += 1;
  }

  void CoverageMapBase::setWasNotTaken(
    uint32_t    address
  )
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].wasNotTaken += 1;
  }

  bool CoverageMapBase::wasAlwaysTaken( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return false;

    return (Info[ offset ].wasTaken &&
            !Info[ offset ].wasNotTaken);
  }

  bool CoverageMapBase::wasNeverTaken( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return false;

    return (!Info[ offset ].wasTaken &&
            Info[ offset ].wasNotTaken);
  }

  bool CoverageMapBase::wasNotTaken( uint32_t address ) const
  {
	    uint32_t offset;
	    bool     result;

	    result = true;

	    if (determineOffset( address, &offset ) != true)
	      result = false;

	    if (Info[ offset ].wasNotTaken <= 0)
	      result = false;

	    return result;
  }

  void CoverageMapBase::sumWasNotTaken( uint32_t address, uint32_t addition)
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].wasNotTaken += addition;
  }

  uint32_t CoverageMapBase::getWasNotTaken( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return 0;

    return Info[ offset ].wasNotTaken;
  }

  bool CoverageMapBase::wasTaken( uint32_t address ) const
  {
    uint32_t offset;
    bool     result;

    result = true;

    if (determineOffset( address, &offset ) != true)
      result = false;

    if (Info[ offset ].wasTaken <= 0)
      result = false;

    return result;
  }

  void CoverageMapBase::sumWasTaken( uint32_t address, uint32_t addition)
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return;

    Info[ offset ].wasTaken += addition;
  }

  uint32_t CoverageMapBase::getWasTaken( uint32_t address ) const
  {
    uint32_t offset;

    if (determineOffset( address, &offset ) != true)
      return 0;

    return Info[ offset ].wasTaken;
  }
}
