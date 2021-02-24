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

    conditionalBranchInstructions.sort();

  }

  Target_arm::~Target_arm()
  {
  }

  bool Target_arm::isNopLine(
    const char* const line,
    int&              size
  )
  {
    if (!strcmp( &line[strlen(line)-3], "nop")) {
      size = 4;
      return true;
    }

    // On ARM, there are literal tables at the end of methods.
    // We need to avoid them.
    if (!strncmp( &line[strlen(line)-10], ".byte", 5)) {
      size = 1;
      return true;
    }
    if (!strncmp( &line[strlen(line)-13], ".short", 6)) {
      size = 2;
      return true;
    }
    if (!strncmp( &line[strlen(line)-16], ".word", 5)) {
      size = 4;
      return true;
    }

    return false;

  }

  bool Target_arm::isBranch(
      const char* instruction
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
