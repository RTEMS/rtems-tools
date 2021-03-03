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

#include <iostream>

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
     * Range, one entry in an address range container.
     */
    class range
    {
    public:
      range (const dwarf_ranges* range);
      range (const range& orig);
      ~range ();

      /**
       * Address 1 in the range.
       */
      dwarf_unsigned addr1 () const;
      dwarf_unsigned addr2 () const;

      /**
       * Get the type of range.
       */
      dwarf_ranges_type type () const;

      /**
       * Is the range the end?
       */
      bool end () const;

      /**
       * Is the range empty? See DWARF 2.17.3.
       */
      bool empty () const;

      /**
       * Assigment operator.
       */
      range& operator = (const range& rhs);

      /**
       * Dump the range.
       */
      void dump (std::ostream& out) const;

    private:

      const dwarf_ranges* range_;
    };

    typedef std::vector < range > ranges;

    /**
     * Address ranges, is a range of addresses.
     */
    class address_ranges
    {
    public:
      address_ranges (file& debug);
      address_ranges (debug_info_entry& die);
      address_ranges (file& debug, dwarf_offset offset);
      address_ranges (const address_ranges& orig);
      ~address_ranges ();

      /**
       * Load the ranges from the DIE.
       */
      bool load (debug_info_entry& die, bool error = true);

      /**
       * Load the ranges from the debug info.
       */
      bool load (dwarf_offset offset, bool error = true);

      /**
       * Get the container.
       */
      const ranges& get () const;

      /**
       * Address range empty?
       */
      bool empty () const;

      /**
       * Assigment operator.
       */
      address_ranges& operator = (const address_ranges& rhs);

      /**
       * Dump the address ranges.
       */
      void dump (std::ostream& out) const;

    private:

      file&         debug;
      dwarf_offset  offset;
      dwarf_ranges* dranges;
      dwarf_signed  dranges_count;
      ranges        ranges_;
    };

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
     * Variable.
     */
    class variable
    {
    public:

      variable (file& debug, debug_info_entry& die);
      variable (const variable& orig);
      ~variable ();

      /**
       * Get the name of the variable.
       */
      std::string name () const;

      /**
       * Is the variable external?
       */
      bool is_external () const;

      /**
       * Is this just a declaration?
       */
      bool is_declaration () const;

      /**
       * Size of the variable.
       */
      size_t size () const;

      /**
       * Assigment operator.
       */
      variable& operator = (const variable& rhs);

      /**
       * Dump the variable.
       */
      void dump (std::ostream& out) const;

    private:

      file&          debug;
      bool           external_;
      bool           declaration_;
      std::string    name_;
      std::string    decl_file_;
      dwarf_unsigned decl_line_;
    };

    typedef std::vector < variable > variables;

    /**
     * Function.
     */
    class function
    {
    public:

      /**
       * The various inline states. See Table 3.4 DWARF 5 standard.
       */
      enum inlined {
        inl_not_inlined = 0,          /**< Not declared inline nore inlined. */
        inl_inline = 1,               /**< Not declared inline but inlined. */
        inl_declared_not_inlined = 2, /**< Declared inline but not inlined. */
        inl_declared_inlined = 3      /**< Declared inline and inlined */
      };

      function (file& debug, debug_info_entry& die);
      function (const function& orig);
      ~function ();

      /**
       * Get the name of the function.
       */
      std::string name () const;

      /**
       * Get the linkage name of the function.
       */
      std::string linkage_name () const;

      /**
       * Get the ranges for the funcion, if empty the PC low and PC high values
       * will be valid.
       */
      const address_ranges& get_ranges () const;

      /**
       * Get the PC low address, valid if ranges is empty.
       */
      dwarf_unsigned pc_low () const;

      /**
       * Get the PC high address, valid if ranges is empty.
       */
      dwarf_unsigned pc_high () const;

      /**
       * Does the function have an entry PC?
       */
      bool has_entry_pc () const;

      /**
       * Does the function have machine code in the image?
       */
      bool has_machine_code () const;

      /**
       * Is the function external?
       */
      bool is_external () const;

      /**
       * Is this just a declaration?
       */
      bool is_declaration () const;

      /**
       * Is the function inlined?
       */
      bool is_inlined () const;

      /**
       * Get the inlined state.
       */
      inlined get_inlined () const;

      /**
       * Get the call file of the inlined function.
       */
      std::string call_file () const;

      /**
       * Is the address inside the function.
       */
      bool inside (dwarf_address addr) const;

      /**
       * Size of the function.
       */
      size_t size () const;

      /**
       * Assigment operator.
       */
      function& operator = (const function& rhs);

      /**
       * Dump the function.
       */
      void dump (std::ostream& out) const;

    private:

      file&          debug;
      bool           machine_code_;
      bool           external_;
      bool           declaration_;
      bool           prototyped_;
      dwarf_unsigned inline_;
      dwarf_unsigned entry_pc_;
      bool           has_entry_pc_;
      dwarf_unsigned pc_low_;
      dwarf_unsigned pc_high_;
      address_ranges ranges_;
      std::string    name_;
      std::string    linkage_name_;
      std::string    decl_file_;
      dwarf_unsigned decl_line_;
      std::string    call_file_;
      dwarf_unsigned call_line_;
    };

    typedef std::vector < function > functions;

    /**
     * Worker to sort the functions.
     */
    struct function_compare
    {
      enum sort_by
      {
        fc_by_name,
        fc_by_size,
        fc_by_address
      };

      const sort_by by;

      bool operator () (const function& a, const function& b) const;

      function_compare (sort_by by = fc_by_name);
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
      debug_info_entry (const debug_info_entry& orig);

      /**
       * Destruct and clean up.
       */
      ~debug_info_entry ();

      /**
       * Is the DIE valid?
       */
      bool valid (bool fatal = true) const;

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
      debug_info_entry& operator = (dwarf_offset offset);

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
       * Get the low PC.
       */
      bool get_lowpc (dwarf_address& addr, bool error = false) const;

      /**
       * Get the high PC.
       */
      bool get_highpc (dwarf_address& addr,
                       bool&          is_address,
                       bool           error = false) const;

      /**
       * Get an attribute.
       */
      bool attribute (dwarf_attr       attr,
                      dwarf_attribute& value,
                      bool             error = true) const;

      /**
       * Get a flag.
       */
      bool attribute (dwarf_attr  attr,
                      dwarf_bool& value,
                      bool        error = true) const;

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
       * Get the ranges.
       */
      bool ranges (dwarf_ranges*& ranges, dwarf_signed& rangescount) const;

      /**
       * Get the child.
       */
      bool get_child (debug_info_entry& child_die);

      /**
       * Has a child?
       */
      bool has_child () const;

      /**
       * Get the silbing
       */
      bool get_sibling (debug_info_entry& sibling_die);

      /**
       * Has a silbing?
       */
      bool has_sibling () const;

      /**
       * Get the debug info for this DIE.
       */
      file& get_debug () const;

      /**
       * deallocate the DIE.
       */
      void dealloc ();

      /**
       * Dump this DIE.
       */
      void dump (std::ostream& out,
                 std::string   prefix,
                 bool          newline = true);

    private:

      /**
       * Update the internal DIE and offset values.
       */
      void update ();

      file&        debug;
      dwarf_die    die;
      dwarf_tag    tag_;
      dwarf_offset offset_;

    };

    /**
     * Dump the DIE and all it's children and siblings.
     */
    void die_dump_children (debug_info_entry& die,
                            std::ostream&     out,
                            std::string       prefix,
                            int               depth = -1,
                            int               nesting = 0);

    /**
     * Dump the DIE and all it's children and siblings.
     */
    void die_dump (debug_info_entry& die,
                   std::ostream&     out,
                   std::string       prefix,
                   int               depth = -1,
                   int               nesting = 0);

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
       * Load the types.
       */
      void load_types ();

      /**
       * Load the variables.
       */
      void load_variables ();

      /**
       * Load the functions.
       */
      void load_functions ();

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
       * Get the functions.
       */
      functions& get_functions ();

      /**
       * Is the address inside the CU? If the PC low and high attributes are
       * valid they are used or the lines are checked.
       */
      bool inside (dwarf_unsigned addr) const;

      /**
       * Copy assignment operator.
       */
      compilation_unit& operator = (const compilation_unit& rhs);

      /**
       * Output the DIE tree.
       */
      void dump_die (std::ostream&     out,
                     const std::string prefix = " ",
                     int               depth = -1);

    private:

      void load_variables (debug_info_entry& die);
      void load_functions (debug_info_entry& die);

      file&          debug;       ///< The DWARF debug handle.
      dwarf_unsigned offset_;     ///< The CU offset in .debug_info
      std::string    name_;       ///< The name of the CU.
      std::string    producer_;   ///< The producer of the CU.
      dwarf_unsigned pc_low_;     ///< The PC low address
      dwarf_unsigned pc_high_;    ///< The PC high address.
      address_ranges ranges_;     ///< Non-continous address range.

      dwarf_offset   die_offset;  ///< The offset of the DIE in the image.

      sources        source_;     ///< Sources table for this CU.
      addresses      addr_lines_; ///< Address table.

      functions      functions_;  ///< The functions in the CU.
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
       * Load the DWARF type information.
       */
      void load_types ();

      /**
       * Load the DWARF functions information.
       */
      void load_functions ();

      /**
       * Load the DWARF variables information.
       */
      void load_variables ();

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
       * Get the variable given a name. Raises an exception if not found.
       */
      variable& get_variable (std::string& name);

      /**
       * Does the function exist.
       */
      bool function_valid (std::string&name);

      /**
       * Get the function given a name. Raises an exception if not found.
       */
      function& get_function (std::string& name);

      /**
       * Get the function given an address.
       */
      bool get_function (const unsigned int address,
                         std::string&       name);

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

      /**
       * Dump the DWARF data.
       */
      void dump (std::ostream&     out,
                 const std::string prefix = " ",
                 int               depth = -1);

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
