/*! @file AddressToLineMapper.cc
 *  @brief AddressToLineMapper Implementation
 *
 *  This file contains the implementation of the functionality
 *  of the AddressToLineMapper class.
 */

#include "AddressToLineMapper.h"

namespace Coverage {

  uint64_t SourceLine::location() const
  {
    return address;
  }

  bool SourceLine::is_an_end_sequence() const
  {
    return is_end_sequence;
  }

  const std::string SourceLine::path() const
  {
    if (!path_) {
      return "unknown";
    } else {
      return *path_;
    }
  }

  int SourceLine::line() const
  {
    return line_num;
  }

  void AddressLineRange::addSourceLine(const rld::dwarf::address& address)
  {
    auto insertResult = sourcePaths.insert(
      std::make_shared<std::string>(address.path()));

    sourceLines.emplace_back(
      SourceLine (
        address.location(),
        *insertResult.first,
        address.line(),
        address.is_an_end_sequence()
      )
    );
  }

  const SourceLine& AddressLineRange::getSourceLine(uint32_t address) const
  {
    if (address < lowAddress || address > highAddress) {
      throw SourceNotFoundError(std::to_string(address));
    }

    const SourceLine* last_line = nullptr;
    for (const auto &line : sourceLines) {
      if (address <= line.location())
      {
        if (address == line.location())
          last_line = &line;
        break;
      }
      last_line = &line;
    }

    if (last_line == nullptr) {
      throw SourceNotFoundError(std::to_string(address));
    }

    return *last_line;
  }

  void AddressToLineMapper::getSource(
    uint32_t address,
    std::string& sourceFile,
    int& sourceLine
  ) const {
    const SourceLine default_sourceline = SourceLine();
    const SourceLine* match = &default_sourceline;

    for (const auto &range : addressLineRanges) {
      try {
        const SourceLine& potential_match = range.getSourceLine(address);

        if (match->is_an_end_sequence() || !potential_match.is_an_end_sequence()) {
          match = &potential_match;
        }
      } catch (const AddressLineRange::SourceNotFoundError&) {}
    }

    sourceFile = match->path();
    sourceLine = match->line();
  }

  AddressLineRange& AddressToLineMapper::makeRange(
    uint32_t low,
    uint32_t high
  )
  {
    addressLineRanges.emplace_back(
      AddressLineRange(low, high)
    );

    return addressLineRanges.back();
  }

}
