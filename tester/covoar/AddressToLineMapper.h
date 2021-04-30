/*! @file AddressToLineMapper.h
 *  @brief AddressToLineMapper Specification
 *
 *  This file contains the specification of the AddressToLineMapper class.
 */

#ifndef __ADDRESS_TO_LINE_MAPPER_H__
#define __ADDRESS_TO_LINE_MAPPER_H__

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <rld-dwarf.h>

namespace Coverage {

  /*! @class SourceLine
   *
   *  This class stores source information for a specific address.
   */
  class SourceLine {

  public:

    SourceLine()
    : address(0),
      path_("unknown"),
      line_num(-1),
      is_end_sequence(true)
    {
    }

    SourceLine(
      uint64_t addr,
      const std::string& src,
      int line,
      bool end_sequence
    ) : address(addr),
        path_(src),
        line_num(line),
        is_end_sequence(end_sequence)
    {
    }

    /*!
     *  This method gets the address of this source information.
     *
     *  @return Returns the address of this source information
     */
    uint64_t location() const;

    /*!
     *  This method gets a value indicating whether this address represents an
     *  end sequence.
     *
     *  @return Returns whether this address represents an end sequence or not
     */
    bool is_an_end_sequence() const;

    /*!
     *  This method gets the source file path of this address.
     *
     *  @return Returns the source file path of this address
     */
    const std::string& path() const;

    /*!
     *  This method gets the source line number of this address.
     *
     *  @return Returns the source line number of this address
     */
    int line() const;

  private:

    /*!
     *  The address of this source information.
     */
    uint64_t address;

    /*!
     *  An iterator pointing to the location in the set that contains the
     *  source file path of the address.
     */
    const std::string& path_;

    /*!
     *  The source line number of the address.
     */
    int line_num;

    /*!
     *  Whether the address represents an end sequence or not.
     */
    bool is_end_sequence;

  };

  typedef std::vector<SourceLine> SourceLines;

  typedef std::set<std::string> SourcePaths;

  /*! @class AddressLineRange
   *
   *  This class stores source information for an address range.
   */
  class AddressLineRange {

  public:

    class SourceNotFoundError : public std::runtime_error {
      /* Use the base class constructors. */
      using std::runtime_error::runtime_error;
    };

    AddressLineRange(
      uint32_t low,
      uint32_t high
    ) : lowAddress(low), highAddress(high)
    {
    }

    /*!
     *  This method adds source and line information for a specified address.
     *
     *  @param[in] address specifies the DWARF address information
     */
    void addSourceLine(const rld::dwarf::address& address);

    /*!
     *  This method gets the source file name and line number for a given
     *  address.
     *
     *  @param[in] address specifies the address to look up
     *
     *  @return Returns the source information for the specified address.
     */
    const SourceLine& getSourceLine(uint32_t address) const;

  private:

    /*!
     *  The low address for this range.
     */
    uint32_t lowAddress;

    /*!
     *  The high address for this range.
     */
    uint32_t highAddress;

    /*!
     *  The source information for addresses in this range.
     */
    SourceLines sourceLines;

    /*!
     *  The set of source file names for this range.
     */
    SourcePaths sourcePaths;

  };

  typedef std::vector<AddressLineRange> AddressLineRanges;

  /*! @class AddressToLineMapper
   *
   *  This class provides address-to-line capabilities.
   */
  class AddressToLineMapper {

  public:

    /*!
     *  This method gets the source file name and line number for a given
     *  address.
     *
     *  @param[in] address specifies the address to look up
     *  @param[out] sourceFile specifies the name of the source file
     *  @param[out] sourceLine specifies the line number in the source file
     */
    void getSource(
      uint32_t address,
      std::string& sourceFile,
      int& sourceLine
    ) const;

    /*!
     *  This method creates a new range with the specified addresses.
     *
     *  @param[in] low specifies the low address of the range
     *  @param[in] high specifies the high address of the range
     *
     *  @return Returns a reference to the newly created range
     */
    AddressLineRange& makeRange(uint32_t low, uint32_t high);

  private:

    /*!
     *  The address and line information ranges.
     */
    AddressLineRanges addressLineRanges;

  };

}

#endif
