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
    branchInstructions.push_back("bn");
    branchInstructions.push_back("bn,a");
    branchInstructions.push_back("be");
    branchInstructions.push_back("be,a");
    branchInstructions.push_back("ble");
    branchInstructions.push_back("ble,a");
    branchInstructions.push_back("bl");
    branchInstructions.push_back("bl,a");
    branchInstructions.push_back("bleu");
    branchInstructions.push_back("bleu,a");
    branchInstructions.push_back("bcs");
    branchInstructions.push_back("bcs,a");
    branchInstructions.push_back("bneg");
    branchInstructions.push_back("bneg,a");
    branchInstructions.push_back("bvs");
    branchInstructions.push_back("bvs,a");
    branchInstructions.push_back("ba");
    branchInstructions.push_back("ba,a");
    branchInstructions.push_back("bne");
    branchInstructions.push_back("bne,a");
    branchInstructions.push_back("bg");
    branchInstructions.push_back("bg,a");
    branchInstructions.push_back("bge");
    branchInstructions.push_back("bge,a");
    branchInstructions.push_back("bgu");
    branchInstructions.push_back("bgu,a");
    branchInstructions.push_back("bcc");
    branchInstructions.push_back("bcc,a");
    branchInstructions.push_back("bpos");
    branchInstructions.push_back("bpos,a");
    branchInstructions.push_back("bvc");
    branchInstructions.push_back("bvc,a");
  
    branchInstructions.sort();    
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
