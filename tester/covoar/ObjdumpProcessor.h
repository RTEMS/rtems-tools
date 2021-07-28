/*! @file ObjdumpProcessor.h
 *  @brief ObjdumpProcessor Specification
 *
 *  This file contains the specification of the ObjdumpProcessor class.
 */

#ifndef __OBJDUMP_PROCESSOR_H__
#define __OBJDUMP_PROCESSOR_H__

#include <list>
#include <string>

#include "ExecutableInfo.h"
#include "TargetBase.h"
#include "DesiredSymbols.h"

#include "rld-process.h"

namespace Coverage {

  class DesiredSymbols;
  class ExecutableInfo;

  /*!
   *  This type defines the elements of an objdump line.
   */
  struct objdumpLine_t {
    /*!
     *  This member variable contains the actual line from the object dump.
     */
    std::string line;

    /*!
     *  This member variable contains the address from the object dump line.
     */
    uint32_t address;

    /*!
     *  This member variable contains an indication of whether the line
     *  is an instruction.
     */
    bool isInstruction;

    /*!
     *  This member variable contains an indication of whether the line
     *  is a nop instruction.
     */
    bool isNop;

    /*!
     *  This member variable contains the size of the nop instruction.
     */
    int nopSize;

    /*!
     *  This member variable contains an indication of whether the line
     *  is a branch instruction.
     */
    bool isBranch;

  };

  /*! @class ObjdumpProcessor
   *
   *  This class implements the functionality which reads the output of
   *  an objdump.  Various information is extracted from the objdump line
   *  to support analysis and report writing.  Analysis of the objdump line
   *  also allows for identification of "nops".  For the purpose of coverage
   *  analysis, nops in the executable may be ignored.  Compilers often
   *  produce nops to align functions on particular alignment boundaries
   *  and the nop between functions can not possibly be executed.
   */
  class ObjdumpProcessor {

  public:

    /*!
     *  This object defines a list of object dump lines
     *  for a file.
     */
    typedef std::list<objdumpLine_t> objdumpLines_t;


    /*!
     *  This object defines a list of instruction addresses
     *  that will be extracted from the objdump file.
     */
    typedef std::list<uint32_t> objdumpFile_t;

    /*!
     *  This method constructs an ObjdumpProcessor instance.
     */
    ObjdumpProcessor(
      DesiredSymbols& symbolsToAnalyze
    );

    /*!
     *  This method destructs an ObjdumpProcessor instance.
     */
    virtual ~ObjdumpProcessor();

    uint32_t determineLoadAddress(
      ExecutableInfo* theExecutable
    );

    /*!
     *  This method fills a tempfile with the .text section of objdump
     *  for the given file name.
     */
    void getFile( std::string fileName,
                  rld::process::tempfile& dmp,
                  rld::process::tempfile& err );

    /*!
     *  This method fills the objdumpList list with all the
     *  instruction addresses in the object dump file.
     */
    void loadAddressTable (
      ExecutableInfo* const executableInformation,
      rld::process::tempfile& dmp,
      rld::process::tempfile& err
    );

    /*!
     *  This method generates and processes an object dump for
     *  the specified executable.
     */
    void load(
      ExecutableInfo* const   executableInformation,
      rld::process::tempfile& dmp,
      rld::process::tempfile& err,
      bool                    verbose
    );

    /*!
     *  This method returns the next address in the objdumpList.
     */
    uint32_t getAddressAfter( uint32_t address );

    /*!
     *  This method returns true if the instruction is
     *  an instruction that results in a code branch, otherwise
     *  it returns false.
     */
    bool IsBranch( const char *instruction );

    /*!
     *  This method returns true if the instruction from
     *  the given line in the objdmp file is a branch instruction,
     *  otherwise it returns false.
     */
    bool isBranchLine(
      const char* const line
    );

  private:

    /*!
     *  This variable consists of a list of all instruction addresses
     *  extracted from the obj dump file.
     */
    objdumpFile_t       objdumpList;

    /*!
     *  This method determines whether the specified line is a
     *  nop instruction.
     *
     *  @param[in] line contains the object dump line to check
     *  @param[out] size is set to the size in bytes of the nop
     *
     *  @return Returns TRUE if the instruction is a nop, FALSE otherwise.
     */
    bool isNop(
      const char* const line,
      int&              size
    );

    /*!
     * This member variable contains the symbols to be analyzed
     */
    DesiredSymbols& symbolsToAnalyze_m;
  };
}
#endif
