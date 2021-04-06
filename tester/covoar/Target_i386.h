/*! @file Target_i386.h
 *  @brief Target_i386 Specification
 *
 *  This file contains the specification of the Target_i386 class.
 */

#ifndef __TARGET_I386_H__
#define __TARGET_I386_H__

#include <list>
#include <string>
#include "TargetBase.h"


namespace Target {

  /*! @class Target_i386
   *
   *  This class is the class for i386 Target.
   *
   */
  class Target_i386: public TargetBase {

  public:

    /*!
     *  This method constructs an Target_i386 instance.
     */
    Target_i386( std::string targetName );

    /*!
     *  This method destructs an Target_i386 instance.
     */
    virtual ~Target_i386();

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
  TargetBase *Target_i386_Constructor(
    std::string          targetName
  );

}
#endif
