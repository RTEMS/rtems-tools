/*! @file Target_sparc.h
 *  @brief Target_sparc Specification
 *
 *  This file contains the specification of the Target_sparc class.
 */

#ifndef __TARGET_SPARC_H__
#define __TARGET_SPARC_H__

#include <list>
#include <string>
#include "TargetBase.h"


namespace Target {

  /*! @class Target_sparc
   *
   *  This is the class for the sparc target.
   *
   */
  class Target_sparc: public TargetBase {

  public:

    /*!
     *  This method constructs an Target_sparc instance.
     */
    Target_sparc( std::string targetName );

    /*!
     *  This method destructs an Target_sparc instance.
     */
    virtual ~Target_sparc();

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
  TargetBase *Target_sparc_Constructor(
    std::string          targetName
  );


}
#endif
