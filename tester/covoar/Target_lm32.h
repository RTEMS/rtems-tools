/*! @file Target_lm32.h
 *  @brief Target_lm32 Specification
 *
 *  This file contains the specification of the Target_lm32 class.
 */

#ifndef __TARGET_LM32_H__
#define __TARGET_LM32_H__

#include <list>
#include <string>
#include "TargetBase.h"


namespace Target {

  /*!
   *
   *  This class is the class for the m68k Target.
   *
   */
  class Target_lm32: public TargetBase {

  public:

    /*!
     *  This method constructs a Target_lm32 instance.
     */
    Target_lm32( std::string targetName );

    /*!
     *  This method destructs a Target_m68 instance.
     */
    virtual ~Target_lm32();

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
      const std::string& line,
      int&               size
    );

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
  TargetBase *Target_lm32_Constructor(
    std::string          targetName
  );

}
#endif
