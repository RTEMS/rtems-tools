/*! @file Target_m68k.h
 *  @brief Target_m68k Specification
 *
 *  This file contains the specification of the Target_m68k class.
 */

#ifndef __TARGET_M68K_H__
#define __TARGET_M68K_H__

#include <list>
#include <string>
#include "TargetBase.h"


namespace Target {

  /*!
   *
   *  This class is the class for the m68k Target.
   *
   */
  class Target_m68k: public TargetBase {

  public:

    /*!
     *  This method constructs a Target_m68k instance.
     */
    Target_m68k( std::string targetName );

    /*!
     *  This method destructs a Target_m68 instance.
     */
    virtual ~Target_m68k();

    /*!
     *  This method determines whether the specified line from a
     *  objdump file is a nop instruction.
     *
     *  @param[in] line contains the object dump line to check
     *  @param[out] size is set to the size in bytes of the nop
     *
     *  @return Returns TRUE if the instruction is a nop, FALSE otherwise.
     */
    bool isNopLine(
      const char* const line,
      int&              size
    );

    /*!
     *  This method determines if the specified line from an
     *  objdump file is a branch instruction.
     */
    bool isBranch(
      const char* const instruction
    );

    /* Documentation inherited from base class */
    virtual uint8_t qemuTakenBit(void);

    /* Documentation inherited from base class */
    virtual uint8_t qemuNotTakenBit(void);

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
  TargetBase *Target_m68k_Constructor(
    std::string          targetName
  );

}
#endif
