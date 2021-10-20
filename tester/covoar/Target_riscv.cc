/*
 * RTEMS Tools Project (http://www.rtems.org/)
 * Copyright 2019 Vijay K. Banerjee <vijaykumar9597@gmail.com>
 * All rights reserved.
 *
 * This file is part of the RTEMS Tools package in 'rtems-tools'.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*! @file Target_riscv.cc
 *  @brief Target_riscv Implementation
 */

#include "Target_riscv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace Target {

  Target_riscv::Target_riscv( std::string targetName ):
    TargetBase( targetName )
  {
    conditionalBranchInstructions.push_back("beqz");
    conditionalBranchInstructions.push_back("bnez");
    conditionalBranchInstructions.push_back("blez");
    conditionalBranchInstructions.push_back("bgez");
    conditionalBranchInstructions.push_back("bltz");
    conditionalBranchInstructions.push_back("bgt");
    conditionalBranchInstructions.push_back("bgtz");
    conditionalBranchInstructions.push_back("ble");
    conditionalBranchInstructions.push_back("bgtu");
    conditionalBranchInstructions.push_back("bleu");

    conditionalBranchInstructions.sort();
   }

  Target_riscv::~Target_riscv()
  {
  }

  bool Target_riscv::isNopLine(
    const std::string& line,
    int&               size
    )
  {
    if ( line.substr( line.length() - 3 ) == "nop" ) {
        size = 4;
        return true;
    }

  return false;
  }

  TargetBase *Target_riscv_Constructor(
    std::string        targetName
    )
    {
      return new Target_riscv( targetName );
    }
}
