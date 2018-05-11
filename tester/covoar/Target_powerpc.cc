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
    branchInstructions.push_back("beq");
    branchInstructions.push_back("beq+");
    branchInstructions.push_back("beq-");
    branchInstructions.push_back("bne");
    branchInstructions.push_back("bne+");
    branchInstructions.push_back("bne-");
    branchInstructions.push_back("bge");
    branchInstructions.push_back("bge+");
    branchInstructions.push_back("bge-");
    branchInstructions.push_back("bgt");
    branchInstructions.push_back("bgt+");
    branchInstructions.push_back("bgt-");
    branchInstructions.push_back("ble");
    branchInstructions.push_back("ble+");
    branchInstructions.push_back("ble-");
    branchInstructions.push_back("blt");
    branchInstructions.push_back("blt+");
    branchInstructions.push_back("blt-");
    branchInstructions.push_back("bla");
    branchInstructions.push_back("bc");
    branchInstructions.push_back("bca");
    branchInstructions.push_back("bcl");
    branchInstructions.push_back("bcla");
    branchInstructions.push_back("bcctr");
    branchInstructions.push_back("bcctrl");
    branchInstructions.push_back("bclr");
    branchInstructions.push_back("bclrl");


    branchInstructions.sort();
  }

  Target_powerpc::~Target_powerpc()
  {
  }

  bool Target_powerpc::isNopLine(
    const char* const line,
    int&              size
  )
  {
    if (!strcmp( &line[strlen(line)-3], "nop")) {
      size = 4;
      return true;
    }

    return false;
  }

  bool Target_powerpc::isBranch(
      const char* const instruction
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
