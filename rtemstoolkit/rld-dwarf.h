/*
 * Copyright (c) 2018, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS Linker DWARF module manages the libdwarf interface.
 *
 */

#if !defined (_RLD_DWARF_H_)
#define _RLD_DWARF_H_

#include <rld.h>
#include <rld-dwarf-types.h>

namespace rld
{
  namespace dwarf
  {
    /**
     * Forward decls
     */
    class sources;
    class debug_info_entry;
    class file;

    /**
     * Address.
     */
    class address
    {
    public:
      address (const sources& source, dwarf_line& line);
      address (const address& orig, const sources& source);
      address (const address& orig, dwarf_address addr);
      address (const address& orig);
      address ();
      ~address ();

      /**
       * Is this address valid?
       */
      bool valid () const;

      /**
       * The location in the address space.
       */
      dwarf_address location () const;

      /**
       * Source file path. This is a full path.
       */
      std::string path () const;

      /**
       * Line number.
       */
      int line () const;

      /**
       * Is a begin statement?
       */
      bool is_a_begin_statement () const;

      /**
       * Is in a block?
       */
      bool is_in_a_block () const;

      /**
       * Is an end sequence?
       */
      bool is_an_end_sequence () const;

      /**
       * Assigment operator.
       */
      address& operator = (const address& rhs);

      /**
       * Less than operator to allow sorting.
       */
      bool operator < (const address& rhs) const;

    private:

      dwarf_address  addr;
      sources const* source;
      dwarf_unsigned source_index;
      dwarf_signed   source_line;
      bool           begin_statement;
      bool           block;
      bool           end_sequence;
    };

    /**
     * The addresses table is a vector sorted from low to high addresses..
     */
    typedef std::vector < address > addresses;

    /**
     * Line addresses.
     */
    class line_addresses
    {
    public:
      line_addresses (file& debug, debug_info_entry& die);
      ~line_addresses ();

      /**
       * Count of lines.
       */
      size_t count () const;

      /**
       * Index operator.
       */
      dwarf_line& operator [] (const int index);

    private:

      file&        debug;
      dwarf_line*  lines;
      dwarf_signed count_;
    };

    /**
     * Sources.
     *
     * This is a CU table of sources. The address's contain an index to a
     * string in this table.
     */
    class sources
    {
    public:
      sources (file& debug, dwarf_offset die_offset);
      sources (const sources& orig);
      //sources (sources&& orig);
      ~sources ();

      /**
       * Index operator.
       */
      std::string operator [] (const int index) const;

      /**
       * Deallocate.
       */
      void dealloc ();

      /**
       * Move assignment operator.
       */
      sources& operator = (sources&& rhs);

    private:

      file&        debug;
      char**       source;
      dwarf_signed count;
      dwarf_offset die_offset;
    };

    /**
     * Debug Information Element (DIE).
     *
     * This class clean up and deallocations a DIE when it desctructs.
     */
    class debug_info_entry
    {
    public:
      /**
       * Construct the DIE, we need to be careful not to share the DIE pointer.
       */
      debug_info_entry (file& debug);
      debug_info_entry (file& debug, dwarf_die& die);
      debug_info_entry (file& debug, dwarf_offset offset);

      /**
       * Destruct and clean up.
       */
      ~debug_info_entry ();

      /**
       * Get the DIE.
       */
      dwarf_die get () const;

      /**
       * Casting operators to get the DIE.
       */
      operator dwarf_die& ();
      operator dwarf_die* ();

      /**
       * Assignment operators.
       */
      debug_info_entry& operator = (debug_info_entry& rhs);

      /**
       * Compare operators.
       */
      bool operator == (debug_info_entry& rhs) const;
      bool operator == (const dwarf_die rhs) const;

      /**
       * Get the tag.
       */
      dwarf_tag tag ();

      /**
       * Get the offset.
       */
      dwarf_offset offset ();

      /**
       * Get an unsigned attribute.
       */
      bool attribute (dwarf_attr      attr,
                      dwarf_unsigned& value,
                      bool            error = true) const;

      /**
       * Get a string attribute.
       */
      bool attribute (dwarf_attr   attr,
                      std::string& value,
                      bool         error = true) const;

      /**
       * Get source lines. Returns the CU line table with all columns.
       *
       * You need to clean this up.
       */
      bool source_lines (dwarf_line*&  lines,
                         dwarf_signed& linecount) const;

      /**
       * Get the source files. This is a table of source files in a CU
       */
      void source_files (char**&       source,
                         dwarf_signed& sourcecount) const;

      /**
       * deallocate the DIE.
       */
      void dealloc ();

    private:

      file&        debug;
      dwarf_die    die;
      dwarf_tag    tag_;
      dwarf_offset offset_;

    };

    /**
     * Compilation Unit.
     */
    class compilation_unit
    {
    public:
      compilation_unit (file&             debug,
                        debug_info_entry& die,
                        dwarf_offset      offset);
      compilation_unit (const compilation_unit& orig);
      ~compilation_unit ();

      /**
       * Name of the CU.
       */
      std::string name () const;

      /**
       * Producer of the CL, the tool that compiled it.
       */
      std::string producer () const;

      /**
       * The low PC value, 0 if there is no attribute.
       */
      unsigned int pc_low () const;

      /**
       * The high PC value, ~0 if there is no attribute.
       */
      unsigned int pc_high () const;

      /**
       * Get the source and line for an address. If the address does not match
       * false is returned the file is set to 'unknown' and the line is set to
       * 0.
       */
      bool get_source (const dwarf_address addr,
                       address&            addr_line);

      /**
       * Is the address inside the CU? If the PC low and high attributes are
       * valid they are used or the lines are checked.
       */
      bool inside (dwarf_unsigned addr) const;

      /**
       * Copy assignment operator.
       */
      compilation_unit& operator = (const compilation_unit& rhs);

    private:

      file&          debug;       ///< The DWARF debug handle.
      dwarf_unsigned offset_;     ///< The CU offset in .debug_info
      std::string    name_;       ///< The name of the CU.
      std::string    producer_;   ///< The producer of the CU.
      dwarf_unsigned pc_low_;     ///< The PC low address
      dwarf_unsigned pc_high_;    ///< The PC high address.

      dwarf_offset   die_offset;  ///< The offset of the DIE in the image.

      sources        source_;     ///< Sources table for this CU.
      addresses      addr_lines_; ///< Address table.
    };

    typedef std::list < compilation_unit > compilation_units;

    /**
     * A source and flags.
     */
    struct source_flags
    {
      std::string  source;  ///< The source file.
      rld::strings flags;   ///< the flags used to build the code.

      source_flags (const std::string& source);
    };

    typedef std::vector < source_flags > sources_flags;

    /**
     * Worker to sort the sources.
     */
    struct source_flags_compare
    {
      const bool by_basename;

      bool operator () (const source_flags& a, const source_flags& b) const;

      source_flags_compare (bool by_basename = true);
    };

    /**
     * A container of producers and the source they build.
     */
    struct producer_source
    {
      std::string   producer; ///< The producer
      sources_flags sources;  ///< The sources built by the producer with
                              ///  flags.

      producer_source (const std::string& producer);
      producer_source ();
    };

    typedef std::list < producer_source > producer_sources;

    /**
     * A DWARF file.
     */
    class file
    {
    public:
     /**
       * Construct an DWARF file.
       */
      file ();

      /**
       * Destruct the DWARF file object.
       */
      ~file ();

      /**
       * Begin using the DWARF information in an ELF file.
       *
       * @param elf The ELF file.
       */
      void begin (rld::elf::file& elf);

      /**
       * End using the DWARF file.
       */
      void end ();

      /**
       * Load the DWARF debug information.
       */
      void load_debug ();

      /**
       * Get the source location given an address.
       */
      bool get_source (const unsigned int address,
                       std::string&       source_file,
                       int&               source_line);

      /**
       * Get the producer sources from the compilation units.
       */
      void get_producer_sources (producer_sources& producers);

      /**
       * Get the DWARF debug information reference.
       */
      dwarf& get_debug ();

      /**
       * Get the compilation units.
       */
      compilation_units& get_cus ();

      /*
       * The DWARF debug conversion operator.
       */
      operator dwarf () { return get_debug (); }

      /**
       * Get the name of the file.
       */
      const std::string& name () const;

    private:

      /**
       * Check if the file is usable. Throw an exception if not.
       *
       * @param where Where the check is performed.
       */
      void check (const char* where) const;

      dwarf           debug;   ///< The libdwarf debug data
      rld::elf::file* elf_;    ///< The libelf reference used to access the
                               ///  DWARF data.

      compilation_units cus;   ///< Image's compilation units
    };

  }
}

#endif
