/*! @file Target_powerpc.cc
 *  @brief Target_powerpc Implementation
 *
 *  This file contains the implementation of the base class for
 *  functions supporting target unique functionallity.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rld.h>

#include "Target_powerpc.h"

namespace Target {

  Target_powerpc::Target_powerpc( std::string targetName ):
    TargetBase( targetName )
  {
    // bl is actually branch and link which is a call
    // branchInstructions.push_back("b");
    // branchInstructions.push_back("ba");
    conditionalBranchInstructions.push_back("beq");
    conditionalBranchInstructions.push_back("beq+");
    conditionalBranchInstructions.push_back("beq-");
    conditionalBranchInstructions.push_back("bne");
    conditionalBranchInstructions.push_back("bne+");
    conditionalBranchInstructions.push_back("bne-");
    conditionalBranchInstructions.push_back("bge");
    conditionalBranchInstructions.push_back("bge+");
    conditionalBranchInstructions.push_back("bge-");
    conditionalBranchInstructions.push_back("bgt");
    conditionalBranchInstructions.push_back("bgt+");
    conditionalBranchInstructions.push_back("bgt-");
    conditionalBranchInstructions.push_back("ble");
    conditionalBranchInstructions.push_back("ble+");
    conditionalBranchInstructions.push_back("ble-");
    conditionalBranchInstructions.push_back("blt");
    conditionalBranchInstructions.push_back("blt+");
    conditionalBranchInstructions.push_back("blt-");
    conditionalBranchInstructions.push_back("bla");
    conditionalBranchInstructions.push_back("bc");
    conditionalBranchInstructions.push_back("bca");
    conditionalBranchInstructions.push_back("bcl");
    conditionalBranchInstructions.push_back("bcla");
    conditionalBranchInstructions.push_back("bcctr");
    conditionalBranchInstructions.push_back("bcctrl");
    conditionalBranchInstructions.push_back("bclr");
    conditionalBranchInstructions.push_back("bclrl");


    conditionalBranchInstructions.sort();
  }

  Target_powerpc::~Target_powerpc()
  {
  }

  bool Target_powerpc::isNopLine(
    const std::string& line,
    int&               size
  )
  {
    if ( line.substr( line.length() - 3 ) == "nop" ) {
      size = 4;
      return true;
    }

    return false;
  }

  bool Target_powerpc::isBranch(
      const std::string& instruction
  )
  {
    throw rld::error(
      "DETERMINE BRANCH INSTRUCTIONS FOR THIS ARCHITECTURE! -- fix me",
      "Target_powerpc::isBranch"
    );
  }

  TargetBase *Target_powerpc_Constructor(
    std::string          targetName
  )
  {
    return new Target_powerpc( targetName );
  }

}
