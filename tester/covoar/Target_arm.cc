/*! @file Target_arm.cc
 *  @brief Target_arm Implementation
 *
 *  This file contains the implementation of the base class for
 *  functions supporting target unique functionallity.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rld.h>

#include "Target_arm.h"

namespace Target {

  Target_arm::Target_arm( std::string targetName ):
    TargetBase( targetName )
  {
    conditionalBranchInstructions.push_back("bcc");
    conditionalBranchInstructions.push_back("bcs");
    conditionalBranchInstructions.push_back("beq");
    conditionalBranchInstructions.push_back("bge");
    conditionalBranchInstructions.push_back("bgt");
    conditionalBranchInstructions.push_back("bhi");
    conditionalBranchInstructions.push_back("bl-hi");
    conditionalBranchInstructions.push_back("bl-lo");
    conditionalBranchInstructions.push_back("ble");
    conditionalBranchInstructions.push_back("bls");
    conditionalBranchInstructions.push_back("blt");
    conditionalBranchInstructions.push_back("bmi");
    conditionalBranchInstructions.push_back("bne");
    conditionalBranchInstructions.push_back("bpl");
    conditionalBranchInstructions.push_back("bvc");
    conditionalBranchInstructions.push_back("bvs");

    conditionalBranchInstructions.push_back("beq.n");
    conditionalBranchInstructions.push_back("bne.n");
    conditionalBranchInstructions.push_back("bcs.n");
    conditionalBranchInstructions.push_back("bhs.n");
    conditionalBranchInstructions.push_back("bcc.n");
    conditionalBranchInstructions.push_back("blo.n");
    conditionalBranchInstructions.push_back("bmi.n");
    conditionalBranchInstructions.push_back("bpl.n");
    conditionalBranchInstructions.push_back("bvs.n");
    conditionalBranchInstructions.push_back("bvc.n");
    conditionalBranchInstructions.push_back("bhi.n");
    conditionalBranchInstructions.push_back("bls.n");
    conditionalBranchInstructions.push_back("bge.n");
    conditionalBranchInstructions.push_back("blt.n");
    conditionalBranchInstructions.push_back("bgt.n");
    conditionalBranchInstructions.push_back("ble.n");

    conditionalBranchInstructions.push_back("beq.w");
    conditionalBranchInstructions.push_back("bne.w");
    conditionalBranchInstructions.push_back("bcs.w");
    conditionalBranchInstructions.push_back("bhs.w");
    conditionalBranchInstructions.push_back("bcc.w");
    conditionalBranchInstructions.push_back("blo.w");
    conditionalBranchInstructions.push_back("bmi.w");
    conditionalBranchInstructions.push_back("bpl.w");
    conditionalBranchInstructions.push_back("bvs.w");
    conditionalBranchInstructions.push_back("bvc.w");
    conditionalBranchInstructions.push_back("bhi.w");
    conditionalBranchInstructions.push_back("bls.w");
    conditionalBranchInstructions.push_back("bge.w");
    conditionalBranchInstructions.push_back("blt.w");
    conditionalBranchInstructions.push_back("bgt.w");
    conditionalBranchInstructions.push_back("ble.w");

    conditionalBranchInstructions.push_back("cbz");
    conditionalBranchInstructions.push_back("cbnz");

    conditionalBranchInstructions.sort();

  }

  Target_arm::~Target_arm()
  {
  }

  bool Target_arm::isNopLine(
    const std::string& line,
    int&               size
  )
  {
    size_t stringLen = line.length();

    if ( line.substr( stringLen - 3 ) == "nop" ) {
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

  bool Target_arm::isBranch(
      const std::string& instruction
  )
  {
    throw rld::error(
      "DETERMINE BRANCH INSTRUCTIONS FOR THIS ARCHITECTURE! -- fix me",
      "Target_arm::isBranch"
    );
  }

  TargetBase *Target_arm_Constructor(
    std::string          targetName
  )
  {
    return new Target_arm( targetName );
  }

}
