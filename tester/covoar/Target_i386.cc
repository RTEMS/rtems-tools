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
    conditionalBranchInstructions.push_back("ja");
    conditionalBranchInstructions.push_back("jb");
    conditionalBranchInstructions.push_back("jc");
    conditionalBranchInstructions.push_back("je");
    conditionalBranchInstructions.push_back("jg");
    conditionalBranchInstructions.push_back("jl");
    conditionalBranchInstructions.push_back("jo");
    conditionalBranchInstructions.push_back("jp");
    conditionalBranchInstructions.push_back("js");
    conditionalBranchInstructions.push_back("jz");
    conditionalBranchInstructions.push_back("jae");
    conditionalBranchInstructions.push_back("jbe");
    conditionalBranchInstructions.push_back("jge");
    conditionalBranchInstructions.push_back("jle");
    conditionalBranchInstructions.push_back("jne");
    conditionalBranchInstructions.push_back("jna");
    conditionalBranchInstructions.push_back("jnb");
    conditionalBranchInstructions.push_back("jnc");
    conditionalBranchInstructions.push_back("jne");
    conditionalBranchInstructions.push_back("jng");
    conditionalBranchInstructions.push_back("jnl");
    conditionalBranchInstructions.push_back("jno");
    conditionalBranchInstructions.push_back("jnp");
    conditionalBranchInstructions.push_back("jns");
    conditionalBranchInstructions.push_back("jnz");
    conditionalBranchInstructions.push_back("jpe");
    conditionalBranchInstructions.push_back("jpo");
    conditionalBranchInstructions.push_back("jnbe");
    conditionalBranchInstructions.push_back("jnae");
    conditionalBranchInstructions.push_back("jnle");
    conditionalBranchInstructions.push_back("jnge");

    conditionalBranchInstructions.sort();

  }

  Target_i386::~Target_i386()
  {
  }

  bool Target_i386::isNopLine(
    const std::string& line,
    int&               size
  )
  {
    size_t stringLen = line.length();

    if ( line.substr( stringLen - 3 ) == "nop" ) {
      size = 1;
      return true;
    }

    // i386 has some two and three byte nops
    if ( line.substr( stringLen - 14 ) == "xchg   %ax,%ax" ) {
      size = 2;
      return true;
    }
    if ( line.substr( stringLen - 16 ) == "xor    %eax,%eax" ) {
      size = 2;
      return true;
    }
    if ( line.substr( stringLen - 16 ) == "xor    %ebx,%ebx" ) {
      size = 2;
      return true;
    }
    if ( line.substr( stringLen - 16 ) == "xor    %esi,%esi" ) {
      size = 2;
      return true;
    }
    if ( line.substr( stringLen - 21 ) == "lea    0x0(%esi),%esi" ) {
      size = 3;
      return true;
    }
    if ( line.substr( stringLen - 28 ) == "lea    0x0(%esi,%eiz,1),%esi" ) {
      // Could be 4 or 7 bytes of padding.
      if ( line.substr( stringLen - 32, 2 ) == "00" ) {
        size = 7;
      } else {
        size = 4;
      }
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
