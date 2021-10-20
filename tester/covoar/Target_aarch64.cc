/*! @file Target_aarch64.cc
 *  @brief Target_aarch64 Implementation
 *
 *  This file contains the implementation of the base class for
 *  functions supporting target unique functionallity.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rld.h>

#include "qemu-traces.h"
#include "Target_aarch64.h"

namespace Target {

  Target_aarch64::Target_aarch64( std::string targetName ):
    TargetBase( targetName )
  {
    conditionalBranchInstructions.push_back("cbnz");
    conditionalBranchInstructions.push_back("cbz");
    conditionalBranchInstructions.push_back("tbnz");
    conditionalBranchInstructions.push_back("tbz");
    conditionalBranchInstructions.push_back("b.eq");
    conditionalBranchInstructions.push_back("b.ne");
    conditionalBranchInstructions.push_back("b.cs");
    conditionalBranchInstructions.push_back("b.hs");
    conditionalBranchInstructions.push_back("b.cc");
    conditionalBranchInstructions.push_back("b.lo");
    conditionalBranchInstructions.push_back("b.mi");
    conditionalBranchInstructions.push_back("b.pl");
    conditionalBranchInstructions.push_back("b.vs");
    conditionalBranchInstructions.push_back("b.vc");
    conditionalBranchInstructions.push_back("b.hi");
    conditionalBranchInstructions.push_back("b.ls");
    conditionalBranchInstructions.push_back("b.ge");
    conditionalBranchInstructions.push_back("b.lt");
    conditionalBranchInstructions.push_back("b.gt");
    conditionalBranchInstructions.push_back("b.le");

    conditionalBranchInstructions.sort();
  }

  Target_aarch64::~Target_aarch64()
  {
  }

  bool Target_aarch64::isNopLine(
    const std::string& line,
    int&               size
  )
  {
    size_t stringLen = line.length();

    if ( line.substr( stringLen - 3 ) == "nop" ) {
      size = 4;
      return true;
    }

    if ( line.substr( stringLen - 6, 3 ) == "udf" ) {
      size = 4;
      return true;
    }

    // On ARM, there are literal tables at the end of methods.
    // We need to avoid them.
    if ( line.substr( stringLen - 10, 5 ) == ".byte" ) {
      size = 1;
      return true;
    }
    if ( line.substr( stringLen - 13, 6 ) == ".short" ) {
      size = 2;
      return true;
    }
    if ( line.substr( stringLen - 16, 5 ) == ".word" ) {
      size = 4;
      return true;
    }

    return false;
  }

  bool Target_aarch64::isBranch(
      const std::string& instruction
  )
  {
    throw rld::error(
      "DETERMINE BRANCH INSTRUCTIONS FOR THIS ARCHITECTURE! -- fix me",
      "Target_aarch64::isBranch"
    );
  }

  uint8_t Target_aarch64::qemuTakenBit()
  {
    return TRACE_OP_BR1;
  }

  uint8_t Target_aarch64::qemuNotTakenBit()
  {
    return TRACE_OP_BR0;
  }

  TargetBase *Target_aarch64_Constructor(
    std::string          targetName
  )
  {
    return new Target_aarch64( targetName );
  }

}
