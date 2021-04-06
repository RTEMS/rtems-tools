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

/*! @file Target_riscv.h
 *  @brief Target_riscv Specification
 *
 *  This file contains the specification of the Target_riscv class.
 */

#ifndef __TARGET_RISCV_H__
#define __TARGET_RISCV_H__

#include <list>
#include <string>
#include "TargetBase.h"

namespace Target {

  /*!  @class Target_riscv
   *
   * This is the class for the riscv target.
   */
  class Target_riscv: public TargetBase {

  public:

  /*!
   *  Target_riscv constructor
   */
  Target_riscv( std::string targetName );

  virtual ~Target_riscv();

  /*!
   *  This method determines nop instruction.
   *
   *  @param[in] line contains the object dump line to check
   *  @param[out] size is set to the size in bytes of the nop
   *
   *  @return Returns True if the instruction is nop, False otherwise.
   */
  bool isNopLine(
    const char* const line,
    int&              size
  );

  /*!
   *  This method determines if it's a branch instruction
   *
   *  @param[in] instruction
   *
   *  @return Returns True if the instruction is a branch instruction, False otherwise.
   */

  bool isBranch(
    const char* const instruction
  );

  private:

  };

  TargetBase *Target_riscv_Constructor(
    std::string        targetName
  );

}
#endif
