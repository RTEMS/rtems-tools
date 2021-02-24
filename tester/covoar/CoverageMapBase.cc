
/*! @file CoverageMapBase.cc
 *  @brief CoverageMapBase Implementation
 *
 *  This file contains the implementation of the functions
 *  which provide a base level of functionality of a CoverageMap.
 */

#include <limits.h>

#include <iostream>
#include <iomanip>

#include <rld.h>

#include "CoverageMapBase.h"

namespace Coverage {

  AddressInfo::AddressInfo ()
    : isStartOfInstruction (false),
      wasExecuted (false),
      isBranch (false),
      isNop (false),
      wasTaken (false),
      wasNotTaken (false)
  {
  }

  AddressRange::AddressRange ()
    : lowAddress(0),
      highAddress(0)
  {
  }

  AddressRange::AddressRange (const std::string& name,
                              uint32_t           lowAddress,
                              uint32_t           highAddress)
    : fileName (name),
      lowAddress (lowAddress),
      highAddress (highAddress)
  {
    info.resize( size( ) );
  }

  size_t AddressRange::size () const
  {
    return highAddress - lowAddress + 1;
  }

  bool AddressRange::inside (uint32_t address) const
  {
    return address >= lowAddress && address <= highAddress;
  }

  AddressInfo& AddressRange::get (uint32_t address)
  {
    if ( !inside( address ) )
      throw rld::error( "address outside range", "AddressRange::get" );
    size_t slot = address - lowAddress;
    if (slot >= info.size ())
      throw rld::error( "address slot not found", "AddressRange::get" );
    return info[slot];
  }

  const AddressInfo& AddressRange::get (uint32_t address) const
  {
    if ( !inside( address ) )
      throw rld::error( "address outside range", "AddressRange::get" );
    size_t slot = address - lowAddress;
    if (slot >= info.size ())
      throw rld::error( "address slot not found", "AddressRange::get" );
    return info[slot];
  }

  void AddressRange::dump (std::ostream& out, bool show_slots) const
  {
    out << std::hex << std::setfill('0')
        << "Address range: low = " << std::setw(8) << lowAddress
        << " high = " << std::setw(8) << highAddress
        << std::endl;
    if (show_slots) {
      size_t slot = 0;
      for (auto& i : info) {
        out << std::hex << std::setfill('0')
            << "0x" << std::setw(8) << slot++ + lowAddress
            << "- isStartOfInstruction:"
            << (char*) (i.isStartOfInstruction ? "yes" : "no")
            << " wasExecuted:"
            << (char*) (i.wasExecuted ? "yes" : "no")
            << std::endl
            << "           isBranch:"
            << (char*) (i.isBranch ? "yes" : "no")
            << " wasTaken:"
            << (char*) (i.wasTaken ? "yes" : "no")
            << " wasNotTaken:"
            << (char*) (i.wasNotTaken ? "yes" : "no")
            << std::dec << std::setfill(' ')
            << std::endl;
      }
    }
  }

  CoverageMapBase::CoverageMapBase(
    const std::string& exefileName,
    uint32_t           low,
    uint32_t           high
    ) : exefileName (exefileName)
  {
    Ranges.push_back( AddressRange( exefileName, low, high ) );
  }

  CoverageMapBase::~CoverageMapBase()
  {
  }

  void CoverageMapBase::Add( uint32_t low, uint32_t high )
  {
    Ranges.push_back( AddressRange( exefileName, low, high ) );
  }

  bool CoverageMapBase::validAddress( const uint32_t address ) const
  {
    for ( auto r : Ranges )
      if (r.inside( address ))
        return true;
    return false;
  }

  void CoverageMapBase::dump( void ) const
  {
    std::cerr << "Coverage Map Contents:" << std::endl;
    for (auto& r : Ranges)
      r.dump( std::cerr );
  }

  uint32_t CoverageMapBase::getSize() const
  {
    size_t size = 0;
    for (auto& r : Ranges)
      size += r.size ();
    return size;
  }

  uint32_t CoverageMapBase::getSizeOfRange( size_t index ) const
  {
    return Ranges.at(index).size();
  }

  bool CoverageMapBase::getBeginningOfInstruction(
    uint32_t  address,
    uint32_t* beginning
  ) const
  {
    bool         status = false;
    uint32_t     start;
    AddressRange range;

    status = getRange( address, range );
    if ( status != true )
      return status;

    start = address;

    while (start >= range.lowAddress ) {
      if (isStartOfInstruction( start - range.lowAddress )) {
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
    /*
     * This is broken, do not trust it.
     */
    return Ranges.front().lowAddress;
  }

  uint32_t CoverageMapBase::getLowAddressOfRange( size_t index ) const
  {
    return Ranges.at(index).lowAddress;
  }

  bool CoverageMapBase::getRange( uint32_t address, AddressRange& range ) const
  {
    for ( auto r : Ranges ) {
      if (r.inside( address )) {
        range.lowAddress = r.lowAddress;
        range.highAddress = r.highAddress;
        range.info = r.info;
        return true;
      }
    }
    return false;
  }

  AddressInfo& CoverageMapBase::getInfo( uint32_t address )
  {
    for ( auto& r : Ranges )
      if (r.inside( address ))
        return r.get( address );
    throw rld::error( "address out of bounds", "CoverageMapBase::getInfo" );
  }

  const AddressInfo& CoverageMapBase::getInfo( uint32_t address ) const
  {
    for ( auto& r : Ranges )
      if (r.inside( address ))
        return r.get( address );
    throw rld::error( "address out of bounds", "CoverageMapBase::getInfo" );
  }

  void CoverageMapBase::setIsStartOfInstruction(
    uint32_t    address
  )
  {
    if ( validAddress( address ) )
      getInfo( address ).isStartOfInstruction = true;
  }

  bool CoverageMapBase::isStartOfInstruction( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return false;
    return getInfo( address ).isStartOfInstruction;
  }

  void CoverageMapBase::setWasExecuted( uint32_t address )
  {
    if ( validAddress( address ) )
      getInfo( address ).wasExecuted += 1;
  }

  void CoverageMapBase::sumWasExecuted( uint32_t address, uint32_t addition)
  {
    if ( validAddress( address ) )
      getInfo( address ).wasExecuted += addition;
  }

  bool CoverageMapBase::wasExecuted( uint32_t address ) const
  {
    bool result = false;
    if ( validAddress( address ) && (getInfo( address ).wasExecuted > 0))
      result = true;
    return result;
  }

  uint32_t CoverageMapBase::getWasExecuted( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return 0;
    return getInfo( address ).wasExecuted;
  }

  void CoverageMapBase::setIsBranch(
    uint32_t    address
  )
  {
    if ( validAddress( address ) )
      getInfo( address ).isBranch = true;
  }

  bool CoverageMapBase::isNop( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return false;
    return getInfo( address ).isNop;
  }

  void CoverageMapBase::setIsNop(
    uint32_t    address
  )
  {
    if ( !validAddress( address ) )
      return;
    getInfo( address ).isNop = true;
  }

  bool CoverageMapBase::isBranch( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return false;
    return getInfo( address ).isBranch;
  }

  void CoverageMapBase::setWasTaken(
    uint32_t    address
  )
  {
    if ( !validAddress( address ) )
      return;
    getInfo( address ).wasTaken += 1;
  }

  void CoverageMapBase::setWasNotTaken(
    uint32_t    address
  )
  {
    if ( !validAddress( address ) )
      return;
    getInfo( address ).wasNotTaken += 1;
  }

  bool CoverageMapBase::wasAlwaysTaken( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return false;
    const AddressInfo& info = getInfo( address );
    return info.wasTaken && !info.wasNotTaken;
  }

  bool CoverageMapBase::wasNeverTaken( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return false;
    const AddressInfo& info = getInfo( address );
    return !info.wasTaken && info.wasNotTaken;
  }

  bool CoverageMapBase::wasNotTaken( uint32_t address ) const
  {
    bool result = true;
    if ( !validAddress( address ) )
      result = false;
    else if ( getInfo( address ).wasNotTaken <= 0 )
      result = false;
    return result;
  }

  void CoverageMapBase::sumWasNotTaken( uint32_t address, uint32_t addition)
  {
    if ( validAddress( address ) )
      getInfo( address ).wasNotTaken += addition;
  }

  uint32_t CoverageMapBase::getWasNotTaken( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return 0;
    return getInfo( address ).wasNotTaken;
  }

  bool CoverageMapBase::wasTaken( uint32_t address ) const
  {
    bool result = true;
    if ( !validAddress( address ) )
      result = false;
    else if ( getInfo( address ).wasTaken <= 0 )
      result = false;
    return result;
  }

  void CoverageMapBase::sumWasTaken( uint32_t address, uint32_t addition)
  {
    if ( !validAddress( address ) )
      return;
    getInfo( address ).wasTaken += addition;
  }

  uint32_t CoverageMapBase::getWasTaken( uint32_t address ) const
  {
    if ( !validAddress( address ) )
      return 0;
    return getInfo( address ).wasTaken;
  }
}
