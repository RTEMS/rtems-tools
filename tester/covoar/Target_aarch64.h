/*! @file Target_aarch64.h
 *  @brief Target_aarch64 Specification
 *
 *  This file contains the specification of the Target_aarch64 class.
 */

#ifndef __TARGET_AARCH64_H__
#define __TARGET_AARCH64_H__

#include <list>
#include <string>
#include "TargetBase.h"

namespace Target {

  /*! @class Target_aarch64
   *
   *  This class is the Target class for the aarch64 processor.
   *
   */
  class Target_aarch64: public TargetBase {

  public:

    /*!
     *  This method constructs an Target_aarch64 instance.
     */
    Target_aarch64( std::string targetName );

    /*!
     *  This method destructs an Target_aarch64 instance.
     */
    virtual ~Target_aarch64();

    /*!
     *  This method determines whether the specified line from a
     *  objdump file is a nop instruction.
     *
     *  @param[in] line contains the object dump line to check
     *  @param[out] size is set to the size in bytes of the nop
     *
     *  @return Returns TRUE if the instruction is a nop, FALSE otherwise.
     */
    virtual bool isNopLine(
      const std::string& line,
      int&               size
    ) override;

    /*!
     *  This method determines if the specified line from an
     *  objdump file is a branch instruction.
     */
    bool isBranch(
      const std::string& instruction
    );

    /* Documentation inherited from base class */
    uint8_t qemuTakenBit() override;

    /* Documentation inherited from base class */
    uint8_t qemuNotTakenBit() override;

  private:

  };

  //!
  //! @brief Constructor Helper
  //!
  //! This is a constructor helper for this class which can be used in support
  //! of factories.
  //!
  //! @param [in] Targetname is the name of the Target being constructed.
  //!
  //! @return This method constructs a new instance of the Target and returns
  //!         that to the caller.
  //!
  TargetBase *Target_aarch64_Constructor(
    std::string          targetName
  );

}
#endif
