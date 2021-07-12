/*! @file TargetBase.h
 *  @brief TargetBase Specification
 *
 *  This file contains the specification of the TargetBase class.
 */

#ifndef __TARGETBASE_H__
#define __TARGETBASE_H__

#include <list>
#include <string>
#include <stdint.h>

namespace Target {

  /*! @class TargetBase
   *
   *  This class is the base class for all Target classes.  Each
   *  target class contains routines that are specific to the target
   *  in question.
   */
  class TargetBase {

  public:

    /*!
     *  This method constructs an TargetBase instance.
     *
     *  @param[in] targetName specifies the desired target
     */
    TargetBase(
      std::string targetName
    );

    /*!
     *  This method destructs an TargetBase instance.
     */
    virtual ~TargetBase();

    /*!
     *  This method returns the program name for addr2line.
     *
     *  @return Returns the target specific addr2line program name
     */
    const char* getAddr2line( void ) const;

    /*!
     *  This method returns the CPU name.
     *
     *  @return Returns the target cpu name
     */
    const char* getCPU( void ) const;

    /*!
     *  This method returns the program name for objdump.
     *
     *  @return Returns the target specific objdump program name
     */
    const char* getObjdump( void ) const;

    /*!
     *  This method returns the target name.
     *
     *  @return Returns the target name
     */
    const char* getTarget( void ) const;

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
      const char* const line,
      int&              size
    ) = 0;


    /*!
     *  This method determins if the objdump line contains a
     *  branch instruction.
     *
     *  @param[in] line contains the object dump line to check
     *  @param[out] size is set to the size in bytes of the nop
     *
     *  @return Returns TRUE if the instruction is a branch, FALSE otherwise.
     */
    bool isBranchLine(
      const char* const line
    );


    /*!
     *  This method determines if the specified line from an
     *  objdump file is a branch instruction.
     */
    bool isBranch( const std::string& instruction );

    /*!
     *  This method returns the bit set by Qemu in the trace record
     *  when a branch is taken.
     */
    virtual uint8_t qemuTakenBit(void);

    /*!
     *  This method returns the bit set by Qemu in the trace record
     *  when a branch is taken.
     */
    virtual uint8_t qemuNotTakenBit(void);

  protected:

    /*!
     * This member variable contains the target name string.
     */
    std::string    targetName_m;

    /*!
     * This member variable is an array of all conditional branch instructions
     * for this target.
     */
    std::list <std::string> conditionalBranchInstructions;

  private:

    /*!
     * This member variable contains the name of the host program
     * which reports the source line for the specified program address.
     */
    std::string addr2line_m;

    /*!
     * This member variable contains the name of the target cpu architecture.
     */
    std::string cpu_m;

    /*!
     * This member variable contains the name of the host program
     * which disassembles an executable or library.
     */
    std::string objdump_m;
  };
}
#endif
