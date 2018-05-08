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
#include <map>
#include <vector>

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
     * A relocation record.
     */
    class relocation
    {
    public:
      /**
       * Construct a relocation record.
       *
       * @param sym The symbol the relocation references.
       * @param offset The offset in the section the relocation applies to.
       * @param info The relocation info.
       * @param addend The constant addend value.
       */
      relocation (const symbols::symbol& sym,
                  elf_addr               offset,
                  elf_xword              info,
                  elf_sxword             addend = 0);

      /**
       * Default constructor.
       */
      relocation ();

      /**
       * The offset.
       */
      elf_addr offset () const;

      /**
       * The type of the relocation record.
       */
      uint32_t type () const;

      /**
       * The info.
       */
      elf_xword info () const;

      /**
       * The constant addend.
       */
      elf_sxword addend () const;

      /**
       * Return the symbol.
       */
      const symbols::symbol& symbol () const;

    private:
      const symbols::symbol* sym;     //< The symbol reference.
      elf_addr               offset_;  //< The offset in the section.
      elf_xword              info_;    //< The record's information.
      elf_sxword             addend_;  //< The constant addend value.
    };

    /**
     * A container of relocation records.
     */
    typedef std::vector < relocation > relocations;

    /**
     * An ELF Section. The current implementation only supports a single data
     * descriptor with a section.
     */
    class section
    {
    public:
      /**
       * Construct the section getting the details from the ELF file given the
       * section index.
       *
       * The section types are (from elf(3)):
       *
       *  Section Type         Library Type     Description
       *  ------------         ------------     -----------
       *   SHT_DYNAMIC          ELF_T_DYN        `.dynamic' section entries.
       *   SHT_DYNSYM           ELF_T_SYM        Symbols for dynamic linking.
       *   SHT_FINI_ARRAY       ELF_T_ADDR       Termination function pointers.
       *   SHT_GROUP            ELF_T_WORD       Section group marker.
       *   SHT_HASH             ELF_T_HASH       Symbol hashes.
       *   SHT_INIT_ARRAY       ELF_T_ADDR       Initialization function pointers.
       *   SHT_NOBITS           ELF_T_BYTE       Empty sections.  See elf(5).
       *   SHT_NOTE             ELF_T_NOTE       ELF note records.
       *   SHT_PREINIT_ARRAY    ELF_T_ADDR       Pre-initialization function
       *                                         pointers.
       *   SHT_PROGBITS         ELF_T_BYTE       Machine code.
       *   SHT_REL              ELF_T_REL        ELF relocation records.
       *   SHT_RELA             ELF_T_RELA       Relocation records with addends.
       *   SHT_STRTAB           ELF_T_BYTE       String tables.
       *   SHT_SYMTAB           ELF_T_SYM        Symbol tables.
       *   SHT_SYMTAB_SHNDX     ELF_T_WORD       Used with extended section
       *                                         numbering.
       *   SHT_GNU_verdef       ELF_T_VDEF       Symbol version definitions.
       *   SHT_GNU_verneed      ELF_T_VNEED      Symbol versioning requirements.
       *   SHT_GNU_versym       ELF_T_HALF       Version symbols.
       *   SHT_SUNW_move        ELF_T_MOVE       ELF move records.
       *   SHT_SUNW_syminfo     ELF_T_SYMINFO    Additional symbol flags.
       *
       * @param file_ The ELF file this section is part of.
       * @param index_ The section's index.
       * @param name The section's name.
       * @param type The section's type.
       * @param alignment The section's alignment.
       * @param flags The section's flags.
       * @param addr The section's in-memory address.
       * @param offset The section's offset in the file.
       * @param size The section's file in bytes.
       * @param link The section's header table link.
       * @param info The section's extra information.
       * @param entry_size The section's entry size.
       */
      section (file&              file_,
               int                index_,
               const std::string& name,
               elf_word           type,
               elf_xword          alignment,
               elf_xword          flags,
               elf_addr           addr,
               elf_off            offset,
               elf_xword          size,
               elf_word           link = 0,
               elf_word           info = 0,
               elf_xword          entry_size = 0);

      /**
       * Construct the section given the details. The ELF file must be
       * writable.
       *
       * @param file_ The ELF file this section is part of.
       * @param index The section's index in the ELF file.
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
       * Add a data segment descriptor to the section if the file is writable.
       *
       * These are following data types (from elf(3)):
       *
       *   ELF_T_ADDR     Machine addresses.
       *   ELF_T_BYTE     Byte data.  The library will not attempt to translate
       *                  byte data.
       *   ELF_T_CAP      Software and hardware capability records.
       *   ELF_T_DYN      Records used in a section of type SHT_DYNAMIC.
       *   ELF_T_EHDR     ELF executable header.
       *   ELF_T_HALF     16-bit unsigned words.
       *   ELF_T_LWORD    64 bit unsigned words.
       *   ELF_T_MOVE     ELF Move records.
       *   ELF_T_NOTE     ELF Note structures.
       *   ELF_T_OFF      File offsets.
       *   ELF_T_PHDR     ELF program header table entries.
       *   ELF_T_REL      ELF relocation entries.
       *   ELF_T_RELA     ELF relocation entries with addends.
       *   ELF_T_SHDR     ELF section header entries.
       *   ELF_T_SWORD    Signed 32-bit words.
       *   ELF_T_SXWORD   Signed 64-bit words.
       *   ELF_T_SYMINFO  ELF symbol information.
       *   ELF_T_SYM      ELF symbol table entries.
       *   ELF_T_VDEF     Symbol version definition records.
       *   ELF_T_VNEED    Symbol version requirement records.
       *   ELF_T_WORD     Unsigned 32-bit words.
       *   ELF_T_XWORD    Unsigned 64-bit words.
       *
       * @param type The type of data in the segment.
       * @param alignment The in-file alignment of the data. Must be a power of 2.
       * @param size The number of bytes in this data descriptor.
       * @param buffer The data in memory.
       * @param offset The offset within the containing section. Can be computed.
       */
      void add_data (elf_type  type,
                     elf_xword alignment,
                     elf_xword size,
                     void*     buffer = 0,
                     elf_off   offset = 0);

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

      /**
       * Return true if the relocation record have an addend field.
       *
       * @retval true The relocation record have the addend field.
       */
      bool get_reloc_type () const;

      /**
       * Set the name index if writable. This is normally done
       * automatically when adding the section to the file.
       */
      void set_name (unsigned int index);

      /**
       * Set the type of relocation records.
       *
       * @param rela If true the records are rela type.
       */
      void set_reloc_type (bool rela);

      /**
       * Add a relocation.
       *
       * @param reloc The relocation record to add.
       */
      void add (const relocation& reloc);

      /**
       * Get the relocations.
       */
      const relocations& get_relocations () const;

    private:

      /**
       * Check the section is valid.
       *
       * @param where Where the check is being made.
       */
      void check (const char* where) const;

      /**
       * Check the section is valid and writable.
       *
       * @param where Where the check is being made.
       */
      void check_writable (const char* where) const;

      file*       file_;  //< The ELF file.
      int         index_; //< The section header index.
      std::string name_;  //< The section's name.
      elf_scn*    scn;    //< ELF private section data.
      elf_shdr    shdr;   //< The section header.
      elf_data*   data_;  //< The section's data.
      bool        rela;   //< The type of relocation records.
      relocations relocs; //< The relocation records.
    };

    /**
     * Container of ELF section pointers.
     */
    typedef std::list < section* > sections;

    /**
     * Container of ELF section as a map, ie associative array.
     */
    typedef std::map < std::string, section > section_table;

    /**
     * An ELF program header.
     */
    class program_header
    {
    public:
      /**
       * Construct a program header.
       */
      program_header ();

      /**
       * Desctruct a program header.
       */
      ~program_header ();

      /**
       * Set the program header.
       *
       * @param type The type of segment.
       * @param flags The segment's flags.
       * @param offset The offet to segment.
       * @param filesz The segment size in the file.
       * @param memsz The segment size in memory.
       * @param align The segment alignment.
       * @param vaddr The virtual address in memory.
       * @param paddr The physical address if any.
       */
      void set (elf_word type,
                elf_word flags,
                elf_off offset,
                elf_xword filesz,
                elf_xword memsz,
                elf_xword align,
                elf_addr vaddr,
                elf_addr paddr = 0);

    private:

      elf_phdr phdr;  //< The ELF program header.
    };

    /**
     * A container of program headers.
     */
    typedef std::list < program_header > program_headers;

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
       * Write the ELF file creating it if it is writable. You should have
       * added the sections and the data segment descriptors to the sections
       * before calling write.
       */
      void write ();

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
       * Return the section with given index.
       *
       * @param index The section's index to look for.
       * @retval section The section matching the index.
       */
      section& get_section (int index);

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
       * @param filtered_syms The filtered symbols found in the file. This is a
       *                      container of pointers.
       * @param unresolved Return unresolved symbols.
       * @param local Return local symbols.
       * @param weak Return weak symbols.
       * @param global Return global symbols.
       */
      void get_symbols (rld::symbols::pointers& filtered_syms,
                        bool                    unresolved = false,
                        bool                    local = false,
                        bool                    weak = true,
                        bool                    global = true);

      /**
       * Get the symbol by index in the symtabl section.
       */
      const symbols::symbol& get_symbol (const int index) const;

      /**
       * Load the relocation records.
       */
      void load_relocations ();

      /**
       * Clear the relocation records.
       */
      void clear_relocations ();

      /**
       * Set the ELF header. Must be writable.
       *
       * The classes are:
       *   ELFCLASSNONE  This class is invalid.
       *   ELFCLASS32    This defines the 32-bit architecture.  It sup- ports
       *                 machines with files and virtual address spa- ces up to
       *                 4 Gigabytes.
       *   ELFCLASS64    This defines the 64-bit architecture.
       *
       * The types are:
       *   ET_NONE  An unknown type.
       *   ET_REL   A relocatable file.
       *   ET_EXEC  An executable file.
       *   ET_DYN   A shared object.
       *   ET_CORE  A core file.
       *
       * The machine types are:
       *   TDB
       *
       * The datatypes are:
       *   ELFDATA2LSB  Two's complement, little-endian.
       *   ELFDATA2MSB  Two's complement, big-endian.
       *
       * @param type The type of ELF file, ie executable, relocatable etc.
       * @param class_ The files ELF class.
       * @param machinetype The type of machine code present in the ELF file.
       * @param datatype The data type, ie LSB or MSB.
       */
      void set_header (elf_half      type,
                       int           class_,
                       elf_half      machinetype,
                       unsigned char datatype);

      /**
       * Add a section to the ELF file if writable.
       */
      void add (section& sec);

      /**
       * Add a program header to the ELF file if writable.
       */
      void add (program_header& phdr);

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

      /**
       * Obtain a reference to this object. End fails while references are
       * held.
       */
      void reference_obtain ();

      /**
       * Release the reference to this object.
       */
      void reference_release ();

      /**
       * Get the machine size in bytes.
       */
      size_t machine_size () const;

      /**
       * Returns true if little endian.
       */
      bool is_little_endian () const;

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
      int                  refs;       //< The reference count.
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
      section_table        secs;       //< The sections as a table.
      program_headers      phdrs;      //< The program headers when creating
                                       //  ELF files.
      rld::symbols::bucket symbols;    //< The symbols. All tables point here.
    };

    /**
     * Return the machine type label given the machine type.
     *
     * @param machinetype The ELF machine type.
     */
    const std::string machine_type (unsigned int machinetype);

    /**
     * Return the global machine type set by the check_file call as a string.
     */
    const std::string machine_type ();

    /**
     * Return the global class set by the check_file call.
     */
    unsigned int object_class ();

    /**
     * Return the global machine type set by the check_file call.
     */
    unsigned int object_machine_type ();

    /**
     * Return the global data type set by the check_file call.
     */
    unsigned int object_datatype ();

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
