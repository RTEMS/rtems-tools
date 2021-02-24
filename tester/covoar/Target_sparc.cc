/*! @file Target_sparc.cc
 *  @brief Target_sparc Implementation
 *
 *  This file contains the implementation of the base class for 
 *  functions supporting target unique functionallity.
 */
#include "Target_sparc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace Target {

  Target_sparc::Target_sparc( std::string targetName ):
    TargetBase( targetName )
  {
    conditionalBranchInstructions.push_back("bn");
    conditionalBranchInstructions.push_back("bn,a");
    conditionalBranchInstructions.push_back("be");
    conditionalBranchInstructions.push_back("be,a");
    conditionalBranchInstructions.push_back("ble");
    conditionalBranchInstructions.push_back("ble,a");
    conditionalBranchInstructions.push_back("bl");
    conditionalBranchInstructions.push_back("bl,a");
    conditionalBranchInstructions.push_back("bleu");
    conditionalBranchInstructions.push_back("bleu,a");
    conditionalBranchInstructions.push_back("bcs");
    conditionalBranchInstructions.push_back("bcs,a");
    conditionalBranchInstructions.push_back("bneg");
    conditionalBranchInstructions.push_back("bneg,a");
    conditionalBranchInstructions.push_back("bvs");
    conditionalBranchInstructions.push_back("bvs,a");
    conditionalBranchInstructions.push_back("ba");
    conditionalBranchInstructions.push_back("ba,a");
    conditionalBranchInstructions.push_back("bne");
    conditionalBranchInstructions.push_back("bne,a");
    conditionalBranchInstructions.push_back("bg");
    conditionalBranchInstructions.push_back("bg,a");
    conditionalBranchInstructions.push_back("bge");
    conditionalBranchInstructions.push_back("bge,a");
    conditionalBranchInstructions.push_back("bgu");
    conditionalBranchInstructions.push_back("bgu,a");
    conditionalBranchInstructions.push_back("bcc");
    conditionalBranchInstructions.push_back("bcc,a");
    conditionalBranchInstructions.push_back("bpos");
    conditionalBranchInstructions.push_back("bpos,a");
    conditionalBranchInstructions.push_back("bvc");
    conditionalBranchInstructions.push_back("bvc,a");
  
    conditionalBranchInstructions.sort();
  }

  Target_sparc::~Target_sparc()
  {
  }

  bool Target_sparc::isNopLine(
    const char* const line,
    int&              size
  )
  {
    if (!strcmp( &line[strlen(line)-3], "nop")) {
      size = 4;
      return true;
    }

    if (!strcmp( &line[strlen(line)-7], "unknown")) {
      size = 4; 
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


  TargetBase *Target_sparc_Constructor(
    std::string          targetName
  )
  {
    return new Target_sparc( targetName );
  }
}
