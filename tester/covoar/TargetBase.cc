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

  const std::string& TargetBase::getAddr2line() const
  {
    return addr2line_m;
  }

  const std::string& TargetBase::getCPU() const
  {
    return cpu_m;
  }

  const std::string& TargetBase::getObjdump() const
  {
    return objdump_m;
  }

  const std::string& TargetBase::getTarget() const
  {
    return targetName_m;
  }

  bool TargetBase::isBranch( const std::string& instruction )
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
      instruction.c_str()
    );
    if ( i  == conditionalBranchInstructions.end() )
      return false;

    return true;
  }

  bool TargetBase::isBranchLine(
    const std::string& line
  )
  {
    #define WARNING_PT1 \
        "WARNING: TargetBase::isBranchLine - ("
    #define WARNING_PT2 \
        ") Unable to find instruction in: "
    const char *ch;
    char instruction[120];
    int  result;


    ch = &(line[0]);

    // Increment to the first tab in the line
    while ((*ch != '\t') && (*ch != '\0')) {
      ch++;
    }
    if (*ch != '\t') {
      std::cerr << WARNING_PT1 << 1 << WARNING_PT2 << line << std::endl;
      return false;
    }
    ch++;

    // Increment to the second tab in the line
    while ((*ch != '\t') && (*ch != '\0'))
      ch++;
    if (*ch != '\t') {
      std::cerr << WARNING_PT1 << 2 << WARNING_PT2 << line << std::endl;
      return false;
    }
    ch++;

    // Grab the instruction which is the next word in the buffer
    // after the second tab.
    result = sscanf( ch, "%s", instruction );
    if (result != 1) {
        std::cerr << WARNING_PT1 << 3 << WARNING_PT2 << line << std::endl;
        return false;
    }

    return isBranch( instruction );
  }

  uint8_t TargetBase::qemuTakenBit()
  {
    return TRACE_OP_BR0;
  }

  uint8_t TargetBase::qemuNotTakenBit()
  {
    return TRACE_OP_BR1;
  }

}
