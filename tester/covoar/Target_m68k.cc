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
    branchInstructions.push_back("bcc");
    branchInstructions.push_back("bccs");
    branchInstructions.push_back("bccl");
    branchInstructions.push_back("bcs");
    branchInstructions.push_back("bcss");
    branchInstructions.push_back("bcsl");
    branchInstructions.push_back("beq");
    branchInstructions.push_back("beqs");
    branchInstructions.push_back("beql");
    branchInstructions.push_back("bge");
    branchInstructions.push_back("bges");
    branchInstructions.push_back("bgel");
    branchInstructions.push_back("bgt");
    branchInstructions.push_back("bgts");
    branchInstructions.push_back("bgtl");
    branchInstructions.push_back("bhi");
    branchInstructions.push_back("bhis");
    branchInstructions.push_back("bhil");
    branchInstructions.push_back("bhs");
    branchInstructions.push_back("bhss");
    branchInstructions.push_back("bhsl");
    branchInstructions.push_back("ble");
    branchInstructions.push_back("bles");
    branchInstructions.push_back("blel");
    branchInstructions.push_back("blo");
    branchInstructions.push_back("blos");
    branchInstructions.push_back("blol");
    branchInstructions.push_back("bls");
    branchInstructions.push_back("blss");
    branchInstructions.push_back("blsl");
    branchInstructions.push_back("blt");
    branchInstructions.push_back("blts");
    branchInstructions.push_back("bltl");
    branchInstructions.push_back("bmi");
    branchInstructions.push_back("bmis");
    branchInstructions.push_back("bmil");
    branchInstructions.push_back("bne");
    branchInstructions.push_back("bnes");
    branchInstructions.push_back("bnel");
    branchInstructions.push_back("bpl");
    branchInstructions.push_back("bpls");
    branchInstructions.push_back("bpll");
    branchInstructions.push_back("bvc");
    branchInstructions.push_back("bvcs");
    branchInstructions.push_back("bvcl");
    branchInstructions.push_back("bvs");
    branchInstructions.push_back("bvss");
    branchInstructions.push_back("bvsl");

    branchInstructions.sort();

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
