/*! @file Target_m68k.cc
 *  @brief Target_m68k Implementation
 *
 *  This file contains the implementation of the base class for
 *  functions supporting target unique functionallity.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rld.h>

#include "Target_m68k.h"
#include "qemu-traces.h"

namespace Target {

  Target_m68k::Target_m68k( std::string targetName ):
    TargetBase( targetName )
  {
    conditionalBranchInstructions.push_back("bcc");
    conditionalBranchInstructions.push_back("bccs");
    conditionalBranchInstructions.push_back("bccl");
    conditionalBranchInstructions.push_back("bcs");
    conditionalBranchInstructions.push_back("bcss");
    conditionalBranchInstructions.push_back("bcsl");
    conditionalBranchInstructions.push_back("beq");
    conditionalBranchInstructions.push_back("beqs");
    conditionalBranchInstructions.push_back("beql");
    conditionalBranchInstructions.push_back("bge");
    conditionalBranchInstructions.push_back("bges");
    conditionalBranchInstructions.push_back("bgel");
    conditionalBranchInstructions.push_back("bgt");
    conditionalBranchInstructions.push_back("bgts");
    conditionalBranchInstructions.push_back("bgtl");
    conditionalBranchInstructions.push_back("bhi");
    conditionalBranchInstructions.push_back("bhis");
    conditionalBranchInstructions.push_back("bhil");
    conditionalBranchInstructions.push_back("bhs");
    conditionalBranchInstructions.push_back("bhss");
    conditionalBranchInstructions.push_back("bhsl");
    conditionalBranchInstructions.push_back("ble");
    conditionalBranchInstructions.push_back("bles");
    conditionalBranchInstructions.push_back("blel");
    conditionalBranchInstructions.push_back("blo");
    conditionalBranchInstructions.push_back("blos");
    conditionalBranchInstructions.push_back("blol");
    conditionalBranchInstructions.push_back("bls");
    conditionalBranchInstructions.push_back("blss");
    conditionalBranchInstructions.push_back("blsl");
    conditionalBranchInstructions.push_back("blt");
    conditionalBranchInstructions.push_back("blts");
    conditionalBranchInstructions.push_back("bltl");
    conditionalBranchInstructions.push_back("bmi");
    conditionalBranchInstructions.push_back("bmis");
    conditionalBranchInstructions.push_back("bmil");
    conditionalBranchInstructions.push_back("bne");
    conditionalBranchInstructions.push_back("bnes");
    conditionalBranchInstructions.push_back("bnel");
    conditionalBranchInstructions.push_back("bpl");
    conditionalBranchInstructions.push_back("bpls");
    conditionalBranchInstructions.push_back("bpll");
    conditionalBranchInstructions.push_back("bvc");
    conditionalBranchInstructions.push_back("bvcs");
    conditionalBranchInstructions.push_back("bvcl");
    conditionalBranchInstructions.push_back("bvs");
    conditionalBranchInstructions.push_back("bvss");
    conditionalBranchInstructions.push_back("bvsl");

    conditionalBranchInstructions.sort();

  }

  Target_m68k::~Target_m68k()
  {
  }

  bool Target_m68k::isNopLine(
    const char* const line,
    int&              size
  )
  {
    if (!strcmp( &line[strlen(line)-3], "nop")) {
      size = 2;
      return true;
    }

    #define GNU_LD_FILLS_ALIGNMENT_WITH_RTS
    #if defined(GNU_LD_FILLS_ALIGNMENT_WITH_RTS)
      // Until binutils 2.20, binutils would fill with rts not nop
      if (!strcmp( &line[strlen(line)-3], "rts")) {
        size = 4;
        return true;
      }
    #endif

    return false;
  }

  bool Target_m68k::isBranch(
      const char* const instruction
  )
  {
    throw rld::error(
      "DETERMINE BRANCH INSTRUCTIONS FOR THIS ARCHITECTURE! -- fix me",
      "Target_m68k::isBranch"
    );
  }

  uint8_t Target_m68k::qemuTakenBit(void)
  {
    return TRACE_OP_BR1;
  }

  uint8_t Target_m68k::qemuNotTakenBit(void)
  {
    return TRACE_OP_BR0;
  }

  TargetBase *Target_m68k_Constructor(
    std::string          targetName
  )
  {
    return new Target_m68k( targetName );
  }

}
