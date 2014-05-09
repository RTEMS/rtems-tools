/*! @file Target_lm32.cc
 *  @brief Target_lm32 Implementation
 *
 *  This file contains the implementation of the base class for 
 *  functions supporting target unique functionallity.
 */
#include "Target_lm32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>

namespace Target {

  // http://www.latticesemi.com/documents/doc20890x45.pdf
  Target_lm32::Target_lm32( std::string targetName ):
    TargetBase( targetName )
  {
    branchInstructions.push_back("be");
    branchInstructions.push_back("bge");
    branchInstructions.push_back("bgeu");
    branchInstructions.push_back("bg");
    branchInstructions.push_back("bgu");
    branchInstructions.push_back("bne");
  }

  Target_lm32::~Target_lm32()
  {
  }

  bool Target_lm32::isNopLine(
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

  TargetBase *Target_lm32_Constructor(
    std::string          targetName
  )
  {
    return new Target_lm32( targetName );
  }

}
