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
    branchInstructions.push_back("bcc");
    branchInstructions.push_back("bcs");
    branchInstructions.push_back("beq");
    branchInstructions.push_back("bge");
    branchInstructions.push_back("bgt");
    branchInstructions.push_back("bhi");
    branchInstructions.push_back("bl-hi");
    branchInstructions.push_back("bl-lo");
    branchInstructions.push_back("ble");
    branchInstructions.push_back("bls");
    branchInstructions.push_back("blt");
    branchInstructions.push_back("bmi");
    branchInstructions.push_back("bne");
    branchInstructions.push_back("bpl");
    branchInstructions.push_back("bvc");
    branchInstructions.push_back("bvs");

    branchInstructions.sort();

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
