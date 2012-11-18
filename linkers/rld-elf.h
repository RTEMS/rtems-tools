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
 * @brief RTEMS Linker ELF module manages the libelf interface.
 *
 */

#if !defined (_RLD_ELF_H_)
#define _RLD_ELF_H_

#include <list>

#include <rld.h>

namespace rld
{
  namespace elf
  {
    /**
     * Forward decl.
     */
    class file;

    /**
     * An ELF Section.
     */
    class section
    {
    public:
      /**
       * Construct the section getting the details.
       *
       * @param elf The ELF file this section is part of.
       * @param index The sections index in the ELF file.
       */
      section (file& file_, int index);

      /**
       * Copy constructor.
       */
      section (const section& orig);

      /**
       * Default constructor.
       */
      section ();

      /**
       * The section's index in the ELF file.
       *
       * @return int The section number.
       */
      int index () const;

      /**
       * The name of the section.
       *
       * @return const std::string& The section's name.
       */
      const std::string& name () const;

      /**
       * The section's data.
       */
      elf_data* data ();

      /**
       * Get the type of the section.
       */
      elf_word type () const;

      /**
       * The section flags.
       */
      elf_xword flags () const;

      /**
       * In-memory address of the section.
       */
      elf_addr address () const;

      /**
       * Alignment constraint.
       */
      elf_xword alignment () const;

      /**
       * The file offset of the section.
       */
      elf_off offset () const;

      /**
       * The header table link.
       */
      elf_word link () const;

      /**
       * Extra information.
       */
      elf_word info () const;

      /**
       * Size of the section.
       */
      elf_xword size () const;

      /**
       * Size of the entries in the section.
       */
      elf_xword entry_size () const;

      /**
       * Number of entries.
       */
      int entries () const;

    private:

      /**
       * Check the section is acrtual valid.
       */
      void check () const;

      file*       file_;  //< The ELF file.
      int         index_; //< The section header index.
      std::string name_;  //< The section's name.
      elf_scn*    scn;    //< ELF private section data.
      elf_shdr    shdr;   //< The section header.
      elf_data*   data_;  //< The section's data.
    };

    /**
     * Container of ELF sections.
     */
    typedef std::list < section > sections;

    /**
     * An ELF file.
     */
    class file
    {
    public:
      /**
       * Construct an ELF file.
       */
      file ();

      /**
       * Destruct the ELF file object.
       */
      ~file ();

      /**
       * Begin using the ELF file.
       *
       * @param name The full name of the file.
       * @param fd The file descriptor to read or write the file.
       * @param writable The file is writeable. The default is false.
       */
      void begin (const std::string& name, int fd, const bool writable = false);

      /**
       * Begin using the ELF file in an archive.
       *
       * @param name The full name of the file.
       * @param archive The file that is the archive.
       * @param offset The offset of the ELF file in the archive.
       */
      void begin (const std::string& name, file& archive, off_t offset);

      /**
       * End using the ELF file.
       */
      void end ();

      /**
       * Load the header. Done automatically.
       */
      void load_header ();

      /**
       * Get the machine type.
       */
      unsigned int machinetype () const;

      /**
       * Get the type of ELF file.
       */
      unsigned int type () const;

      /**
       * Get the class of the object file.
       */
      unsigned int object_class () const;

      /**
       * Get the data type, ie LSB or MSB.
       */
      unsigned int data_type () const;

      /**
       * Is the file an archive format file ?
       */
      bool is_archive () const;

      /**
       * Is the file an executable ?
       */
      bool is_executable () const;

      /**
       * Is the file relocatable ?
       */
      bool is_relocatable() const;

      /**
       * The number of sections in the file.
       */
      int section_count () const;

      /**
       * Load the sections.
       */
      void load_sections ();

      /**
       * Get a filtered container of the sections. The key is the section
       * type. If the sections are not loaded they are loaded. If the type is 0
       * all sections are returned.
       *
       * @param filtered_secs The container the copy of the filtered sections
       *                      are placed in.
       * @param type The type of sections to filter on. If 0 all sections are
       *             matched.
       */
      void get_sections (sections& filtered_secs, unsigned int type);

      /**
       * Return the index of the string section.
       */
      int strings_section () const;

      /**
       * Get the string from the specified section at the requested offset.
       *
       * @param section The section to search for the string.
       * @param offset The offset in the string section.
       * @return std::string The string.
       */
      std::string get_string (int section, size_t offset);

      /**
       * Get the string from the ELF header declared string section at the
       * requested offset.
       *
       * @param offset The offset in the string section.
       * @return std::string The string.
       */
      std::string get_string (size_t offset);

      /**
       * Load the symbols.
       */
      void load_symbols ();

      /**
       * Get a filtered container of symbols given the various types. If the
       * symbols are not loaded they are loaded.
       *
       * @param filter_syms The filtered symbols found in the file. This is a
       *                    container of pointers.
       * @param local Return local symbols.
       * @param weak Return weak symbols.
       * @param global Return global symbols.
       * @param unresolved Return unresolved symbols.
       */
      void get_symbols (rld::symbols::pointers& filtered_syms,
                        bool                    unresolved = false,
                        bool                    local = false,
                        bool                    weak = true,
                        bool                    global = true);

      /**
       * Set the ELF header. Must be writable.
       *
       * @param type The type of ELF file, ie executable, relocatable etc.
       * @param machinetype The type of machine code present in the ELF file.
       * @param datatype The data type, ie LSB or MSB.
       */
      void set_header (elf_half      type,
                       int           class_,
                       elf_half      machinetype,
                       unsigned char datatype);

      /**
       * Get the ELF reference.
       */
      elf* get_elf ();

      /**
       * Get the name of the file.
       */
      const std::string& name () const;

      /**
       * Is the file writable ?
       */
      bool is_writable () const;

    private:

      /**
       * Begin using the ELF file.
       *
       * @param name The full name of the file.
       * @param fd The file descriptor to read or write the file.
       * @param writable The file is writeable. It cannot be part of an archive.
       * @param archive The archive's ELF handle or 0 if not an archive.
       * @param offset The offset of the ELF file in the archive if elf is non-zero.
       */
      void begin (const std::string& name,
                  int                fd,
                  const bool         writable,
                  file*              archive,
                  off_t              offset);

      /**
       * Check if the file is usable. Throw an exception if not.
       *
       * @param where Where the check is performed.
       */
      void check (const char* where) const;

      /**
       * Check if the file is usable and writable. Throw an exception if not.
       *
       * @param where Where the check is performed.
       */
      void check_writable (const char* where) const;

      /**
       * Check if the ELF header is valid. Throw an exception if not.
       *
       * @param where Where the check is performed.
       */
      void check_ehdr (const char* where) const;

      /**
       * Check if the ELF program header is valid. Throw an exception if not.
       *
       * @param where Where the check is performed.
       */
      void check_phdr (const char* where) const;

      /**
       * Generate libelf error.
       *
       * @param where Where the error is generated.
       */
      void error (const char* where) const;

      int                  fd_;        //< The file handle.
      std::string          name_;      //< The name of the file.
      bool                 archive;    //< The ELF file is part of an archive.
      bool                 writable;   //< The file is writeable.
      elf*                 elf_;       //< The ELF handle.
      unsigned int         mtype;      //< The machine type.
      unsigned int         oclass;     //< The object class.
      const char*          ident_str;  //< The ELF file's ident string.
      size_t               ident_size; //< The size of the ident.
      elf_ehdr*            ehdr;       //< The ELF header.
      elf_phdr*            phdr;       //< The ELF program header.
      std::string          stab;       //< The string table.
      sections             secs;       //< The sections.
      rld::symbols::bucket symbols;    //< The symbols. All tables point here.
    };

    /**
     * Return the machine type label given the machine type.
     *
     * @param machinetype The ELF machine type.
     */
    const std::string machine_type (unsigned int machinetype);

    /**
     * Return the global machine type set by the check_file call.
     */
    const std::string machine_type ();

    /**
     * Check the file against the global machine type, object class and data
     * type. If this is the first file checked it becomes the default all
     * others are checked against. This is a simple way to make sure all files
     * are the same type.
     *
     * @param file The check to check.
     */
    void check_file(const file& file);

  }
}

#endif
