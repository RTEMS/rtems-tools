/*! @file TargetBase.cc
 *  @brief TargetBase Implementation
 *
 *  This file contains the implementation of the base class for
 *  functions supporting target unique functionallity.
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <rld.h>

#include "TargetBase.h"
#include "qemu-traces.h"

namespace Target {

  TargetBase::TargetBase(
    std::string targetName
  ):
    targetName_m( targetName )
  {
    int i;
    std::string front = "";

    for (i=0 ; targetName_m[i] && targetName_m[i] != '-' ; ) {
      cpu_m[i]   = targetName_m[i];
      cpu_m[++i] = '\0';
    }
    if (targetName_m[i] == '-')
      front = targetName_m + "-";


    addr2line_m = front + "addr2line";
    objdump_m   = front + "objdump";
  }

  TargetBase::~TargetBase()
  {
  }

  const char* TargetBase::getAddr2line() const
  {
    return addr2line_m.c_str();
  }

  const char* TargetBase::getCPU( void ) const
  {
    return cpu_m.c_str();
  }

  const char* TargetBase::getObjdump() const
  {
    return objdump_m.c_str();
  }

  const char* TargetBase::getTarget( void ) const
  {
    return targetName_m.c_str();
  }

  bool TargetBase::isBranch(
      const char* const instruction
  )
  {
    std::list <std::string>::iterator i;

    if (conditionalBranchInstructions.empty()) {
      throw rld::error(
        "DETERMINE BRANCH INSTRUCTIONS FOR THIS ARCHITECTURE! -- fix me",
        "TargetBase::isBranch"
      );
    }

    i = find(
      conditionalBranchInstructions.begin(),
      conditionalBranchInstructions.end(),
      instruction
    );
    if ( i  == conditionalBranchInstructions.end() )
      return false;

    return true;
  }

  bool TargetBase::isBranchLine(
    const char* const line
  )
  {
    #define WARNING \
        "WARNING: TargetBase::isBranchLine - (%d) " \
        "Unable to find instruction in: %s\n"
    const char *ch;
    char instruction[120];
    int  result;


    ch = &(line[0]);

    // Increment to the first tab in the line
    while ((*ch != '\t') && (*ch != '\0')) {
      ch++;
    }
    if (*ch != '\t') {
      fprintf( stderr, WARNING, 1, line );
      return false;
    }
    ch++;

    // Increment to the second tab in the line
    while ((*ch != '\t') && (*ch != '\0'))
      ch++;
    if (*ch != '\t') {
      fprintf( stderr, WARNING, 2, line) ;
      return false;
    }
    ch++;

    // Grab the instruction which is the next word in the buffer
    // after the second tab.
    result = sscanf( ch, "%s", instruction );
    if (result != 1) {
        fprintf( stderr, WARNING, 3, line );
        return false;
    }

    return isBranch( instruction );
  }

  uint8_t TargetBase::qemuTakenBit(void)
  {
    return TRACE_OP_BR0;
  }

  uint8_t TargetBase::qemuNotTakenBit(void)
  {
    return TRACE_OP_BR1;
  }

}
