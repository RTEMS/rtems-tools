/*
 * Copyright (c) 2011, Chris Johns <chrisj@rtems.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/**
 * @file
 *
 * @ingroup rtems-ld
 *
 * @brief RTEMS Linker symbols manages the symbols from all the object files.
 *
 */

#if !defined (_RLD_SYMBOLS_H_)
#define _RLD_SYMBOLS_H_

#include <iostream>
#include <list>
#include <map>
#include <string>

#include <rld-elf-types.h>

namespace rld
{
  /**
   * Forward declarations.
   */
  namespace files
  {
    class object;
  }

  namespace symbols
  {
    /**
     * A symbol.
     */
    class symbol
    {
    public:
      /**
       * Default constructor. No symbol has been defined.
       */
      symbol ();

      /**
       * Construct an exported symbol with a object file.
       */
      symbol (int                 index,
              const std::string&  name,
              files::object&      object,
              const elf::elf_sym& esym);

      /**
       * Construct a symbol with no object file and an ELF index.
       */
      symbol (int index, const std::string& name, const elf::elf_sym& esym);

      /**
       * Construct a linker symbol that is internally created.
       */
      symbol (const std::string&  name,
              const elf::elf_addr value = 0);

      /**
       * Construct a linker symbol that is internally created.
       */
      symbol (const char*   name,
              elf::elf_addr value = 0);

      /**
       * The symbol's index in the symtab section of the ELF file.
       */
      int index () const;

      /**
       * The symbol's name.
       */
      const std::string& name () const;

      /**
       * The symbol's demangled name.
       */
      const std::string& demangled () const;

      /**
       * Is the symbol a C++ name ?
       */
      bool is_cplusplus () const;

      /**
       * Is the symbol binding is local ?
       */
      bool is_local () const;

      /**
       * Is the symbol binding weak ?
       */
      bool is_weak () const;

      /**
       * Is the symbol binding global ?
       */
      bool is_global () const;

      /**
       * The symbol's type.
       */
      int type () const;

      /**
       * The symbol's binding, ie local, weak, or global.
       */
      int binding () const;

      /**
       * The symbol's section index.
       */
      int section_index () const;

      /**
       * The value of the symbol.
       */
      elf::elf_addr value () const;

      /**
       * The data of the symbol.
       */
      uint32_t info () const;

      /**
       * The symbol's object file name.
       */
      files::object* object () const;

      /**
       * Set the symbol's object file name. Used when resolving unresolved
       * symbols.
       */
      void set_object (files::object& obj);

      /**
       * The ELF symbol.
       */
      const elf::elf_sym& esym () const;

      /**
       * Return the number of references.
       */
      int references () const {
        return references_;
      }

      /**
       * Return the number of references.
       */
      void referenced ();

      /**
       * Less than operator for the map container.
       */
      bool operator< (const symbol& rhs) const;

      /**
       * Output to the a stream.
       */
      void output (std::ostream& out) const;

    private:

      int            index_;      //< The symbol's index in the ELF file.
      std::string    name_;       //< The name of the symbol.
      std::string    demangled_;  //< If a C++ symbol the demangled name.
      files::object* object_;     //< The object file containing the symbol.
      elf::elf_sym   esym_;       //< The ELF symbol.
      int            references_; //< The number of times if it referenced.
    };

    /**
     * Container of symbols. A bucket of symbols.
     */
    typedef std::list < symbol > bucket;

    /**
     * References to symbols. Should always point to symbols held in a bucket.
     */
    typedef std::list < symbol* > pointers;

    /**
     * A symbols table is a map container of symbols. Should always point to
     * symbols held in a bucket.
     */
    typedef std::map < std::string, symbol* > symtab;

    /**
     * A symbols contains a symbol table of global, weak and local symbols.
     */
    class table
    {
    public:
      /**
       * Construct a table.
       */
      table ();

      /**
       * Destruct a table.
       */
      ~table ();

      /**
       * Add a global symbol.
       */
      void add_global (symbol& sym);

      /**
       * Add a weak symbol.
       */
      void add_weak (symbol& sym);

      /**
       * Add a local symbol.
       */
      void add_local (symbol& sym);

      /**
       * Find a global symbol.
       */
      symbol* find_global (const std::string& name);

      /**
       * Find an weak symbol.
       */
      symbol* find_weak (const std::string& name);

      /**
       * Find an local symbol.
       */
      symbol* find_local (const std::string& name);

      /**
       * Return the size of the symbols loaded.
       */
      size_t size () const;

      /**
       * Return the globals symbol table.
       */
      const symtab& globals () const;

      /**
       * Return the weaks symbol table.
       */
      const symtab& weaks () const;

      /**
       * Return the locals symbol table.
       */
      const symtab& locals () const;

    private:

      /**
       * Cannot copy a table.
       */
      table (const table& orig);

      /**
       * A table of global symbols.
       */
      symtab globals_;

      /**
       * A table of weak symbols.
       */
      symtab weaks_;

      /**
       * A table of local symbols.
       */
      symtab locals_;
    };

    /**
     * Load a table from a buckey.
     */
    void load (bucket& bucket_, table& table_);

    /**
     * Load a table from a buckey.
     */
    void load (bucket& bucket_, symtab& table_);

    /**
     * Given a container of symbols return how many are referenced.
     */
    size_t referenced (pointers& symbols);

    /**
     * Output the symbol table.
     */
    void output (std::ostream& out, const table& symbols);

    /**
     * Output the symbol table.
     */
    void output (std::ostream& out, const symtab& symbols);
  }
}

/**
 * Output stream operator.
 */
static inline std::ostream& operator<< (std::ostream&               out,
                                        const rld::symbols::symbol& sym) {
  sym.output (out);
  return out;
}

#endif
