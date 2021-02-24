//
//

//! @file TargetFactory.cc
//! @brief TargetFactory Implementation
//!
//! This file contains the implementation of a factory for a
//! instances of a family of classes derived from TargetBase.
//!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rld.h>

#include "TargetFactory.h"

#include "Target_aarch64.h"
#include "Target_arm.h"
#include "Target_i386.h"
#include "Target_m68k.h"
#include "Target_powerpc.h"
#include "Target_lm32.h"
#include "Target_sparc.h"
#include "Target_riscv.h"

namespace Target {

  //!
  //! @brief TargetBase Factory Table Entry
  //!
  //! This structure contains the @a name associated with the target
  //! in the configuration structures.  The table of names is scanned
  //! to find a constructor helper.
  //!
  typedef struct {
     //! This is the string found in configuration to match.
     const char    *theTarget;
     //! This is the static wrapper for the constructor.
     TargetBase *(*theCtor)(
       std::string
     );
  } FactoryEntry_t;

  //!
  //! @brief TargetBase Factory Table
  //!
  //! This is the table of possible types we can construct
  //! dynamically based upon user specified configuration information.
  //! All must be derived from TargetBase.
  //!
  static FactoryEntry_t FactoryTable[] = {
    { "aarch64", Target_aarch64_Constructor },
    { "arm",     Target_arm_Constructor },
    { "i386",    Target_i386_Constructor },
    { "lm32",    Target_lm32_Constructor },
    { "m68k",    Target_m68k_Constructor },
    { "powerpc", Target_powerpc_Constructor },
    { "sparc",   Target_sparc_Constructor },
    { "riscv",   Target_riscv_Constructor },
    { "TBD",     NULL },
  };

  TargetBase* TargetFactory(
    std::string          targetName
  )
  {
    size_t      i;
    std::string cpu;

    i = targetName.find( '-' );
    if ( i == targetName.npos )
      cpu = targetName;
    else
      cpu = targetName.substr( 0, i );

    // fprintf( stderr, "%s --> %s\n", targetName.c_str(), cpu.c_str());
    // Iterate over the table trying to find an entry with a matching name
    for ( i=0 ; i < sizeof(FactoryTable) / sizeof(FactoryEntry_t); i++ ) {
      if ( !strcmp(FactoryTable[i].theTarget, cpu.c_str() ) )
        return FactoryTable[i].theCtor( targetName );
    }

    std::ostringstream what;
    what << cpu << "is not a known architecture!!! - fix me" << std::endl;
    throw rld::error( what, "TargetFactory" );

    return NULL;
  }
}
