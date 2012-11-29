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
      symbol (const std::string&  name,
              files::object&      object,
              const elf::elf_sym& esym);

      /**
       * Construct an unresolved symbol with no object file.
       */
      symbol (const std::string& name, const elf::elf_sym& esym);

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
       * The symbol's type.
       */
      int type () const;

      /**
       * The symbol's binding, ie local, weak, or global.
       */
      int binding () const;

      /**
       * The synbol's section index.
       */
      int index () const;

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
    typedef std::map < std::string, symbol* > table;

    /**
     * Load a table from a buckey.
     */
    void load (bucket& bucket_, table& table_);

    /**
     * Given a container of symbols return how many are referenced.
     */
    size_t referenced (pointers& symbols);

    /**
     * Output the symbol table.
     */
    void output (std::ostream& out, const table& symbols);
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
