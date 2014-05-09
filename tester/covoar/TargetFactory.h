//
//

//! @file TargetFactory.h
//! @brief TargetFactory Specification
//!
//! This file contains the specification of a factory for a
//! instances of a family of classes derived from TargetBase.
//!

#ifndef __TARGET_FACTORY_H__
#define __TARGET_FACTORY_H__

#include <string>
#include "TargetBase.h"

namespace Target {

  //!
  //! @brief Target Construction Factory
  //!
  //! This method implements a factory for construction of instances
  //! classes derived from TargetBase.  Given the name of the class instance
  //! and name of the instance, it returns an instance.
  //!
  //! @param [in] targetName is the name of the target to create.
  //!
  //! @return This method constructs a new instance of an TargetBase and
  //!         returns that to the caller.
  //!
  TargetBase *TargetFactory(
    std::string          targetName
  );

}
#endif

