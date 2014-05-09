/*! @file Target_i386.cc
 *  @brief Target_i386 Implementation
 *
 *  This file contains the implementation of the base class for 
 *  functions supporting target unique functionallity.
 */

#include "Target_i386.h"
#include "qemu-traces.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace Target {

  Target_i386::Target_i386( std::string targetName ):
    TargetBase( targetName )
  {
    branchInstructions.push_back("ja");
    branchInstructions.push_back("jb");
    branchInstructions.push_back("jc");
    branchInstructions.push_back("je");
    branchInstructions.push_back("jg");
    branchInstructions.push_back("jl");
    branchInstructions.push_back("jo");
    branchInstructions.push_back("jp");
    branchInstructions.push_back("js");
    branchInstructions.push_back("jz");
    branchInstructions.push_back("jae");
    branchInstructions.push_back("jbe");
    branchInstructions.push_back("jge");
    branchInstructions.push_back("jle");
    branchInstructions.push_back("jne");
    branchInstructions.push_back("jna");
    branchInstructions.push_back("jnb");
    branchInstructions.push_back("jnc");
    branchInstructions.push_back("jne");
    branchInstructions.push_back("jng");
    branchInstructions.push_back("jnl");
    branchInstructions.push_back("jno");
    branchInstructions.push_back("jnp");
    branchInstructions.push_back("jns");
    branchInstructions.push_back("jnz");
    branchInstructions.push_back("jpe");
    branchInstructions.push_back("jpo");
    branchInstructions.push_back("jnbe");
    branchInstructions.push_back("jnae");
    branchInstructions.push_back("jnle");
    branchInstructions.push_back("jnge");

    branchInstructions.sort();

  }

  Target_i386::~Target_i386()
  {
  }

  bool Target_i386::isNopLine(
    const char* const line,
    int&              size
  )
  {
    if (!strcmp( &line[strlen(line)-3], "nop")) {
      size = 1; 
      return true;
    }

    // i386 has some two and three byte nops
    if (!strncmp( &line[strlen(line)-14], "xchg   %ax,%ax", 14)) {
      size = 2;
      return true;
    }
    if (!strncmp( &line[strlen(line)-16], "xor    %eax,%eax", 16)) {
      size = 2;
      return true;
    }
    if (!strncmp( &line[strlen(line)-16], "xor    %ebx,%ebx", 16)) {
      size = 2;
      return true;
    }
    if (!strncmp( &line[strlen(line)-16], "xor    %esi,%esi", 16)) {
      size = 2;
      return true;
    }
    if (!strncmp( &line[strlen(line)-21], "lea    0x0(%esi),%esi", 21)) {
      size = 3;
      return true;
    }

    return false;
  }

  uint8_t Target_i386::qemuTakenBit(void)
  {
    return TRACE_OP_BR1;
  }

  uint8_t Target_i386::qemuNotTakenBit(void)
  {
    return TRACE_OP_BR0;
  }

  TargetBase *Target_i386_Constructor(
    std::string          targetName
  )
  {
    return new Target_i386( targetName );
  }

}
