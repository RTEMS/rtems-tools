/*! @file SymbolTable.h
 *  @brief SymbolTable Specification
 *
 *  This file contains the specification of the SymbolTable class.
 */

#ifndef __SYMBOLTABLE_H__
#define __SYMBOLTABLE_H__

#include <stdint.h>
#include <string>
#include <map>
#include <list>

namespace Coverage {

  /*! @class SymbolTable
   *
   *  This class maintains information for each desired symbol within an
   *  executable.  A desired symbol is a symbol for which analysis is to
   *  be performed.
   */
  class SymbolTable {

  public:

    /*!
     *  This structure defines the information kept for each symbol.
     */
    typedef struct {
      uint32_t startingAddress;
      uint32_t length;
    } symbolInfo_t;

   typedef std::list< symbolInfo_t > symbolInfo;
   typedef std::list< symbolInfo_t >::iterator  symbolInfoIterator_t;

    /*!
     *  This method constructs a SymbolTable instance.
     */
    SymbolTable();

    /*!
     *  This method destructs a SymbolTable instance.
     */
    virtual ~SymbolTable();

    /*!
     *  This method adds the specified symbol information to the
     *  symbol table.
     *
     *  @param[in] symbol specifies the symbol to add
     *  @param[in] start specifies the symbol's start address
     *  @param[in] length specifies the symbol's length
     *
     */
    void addSymbol(
      const std::string& symbol,
      const uint32_t     start,
      const uint32_t     length
    );

    /*!
     *  This method returns the symbol information for the specified symbol.
     *
     *  @param[in] symbol specifies the symbol for which to obtain information
     *
     *  @return Returns a pointer to the symbol information
     */
    symbolInfo* getInfo(
      const std::string& symbol
    );

    /*!
     *  This method returns the length in bytes of the specified symbol.
     *
     *  @param[in] symbol specifies the symbol for which to obtain the length
     *
     *  @return Returns the length of the symbol
     */
    uint32_t getLength(
      const std::string& symbol
    );

    /*!
     *  This method returns the symbol that contains the specified address.
     *
     *  @param[in] address specifies the address for which to obtain the symbol
     *
     *  @return Returns the symbol containing the address
     */
    std::string getSymbol(
      uint32_t address
    );

    /*!
     *  This method prints SymbolTable content to stdout
     *
     */
    void dumpSymbolTable( void );

  private:

    /*!
     *  This map associates the end address of a symbol's address
     *  range with the symbol's address range definition.
     */
    typedef struct {
       uint32_t    low;
       uint32_t    high;
       std::string symbol;
    } symbol_entry_t;
    typedef std::map< uint32_t, symbol_entry_t > contents_t;
    contents_t contents;

    /*!
     *  This map associates each symbol from an executable with
     *  the symbol's information.
     */
    typedef std::map<std::string, symbolInfo> info_t;
    typedef std::map<std::string, symbolInfo>::iterator infoIterator_t;
    info_t info;

  };
}
#endif
