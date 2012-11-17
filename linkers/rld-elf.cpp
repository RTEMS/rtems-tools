/*
 * Copyright (c) 2011-2012, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS Linker ELF module manages the ELF format images.
 *
 */

#include <string.h>

#include <rld.h>

namespace rld
{
  namespace elf
  {
    /**
     * Throw an ELF error.
     *
     * @param where Where the error is raised.
     */
    void libelf_error (const std::string& where)
    {
      throw rld::error (::elf_errmsg (-1), "libelf:" + where);
    }

    /**
     * We record the first class, machine and .. type of object file we get the
     * header of and all header must match. We cannot mix object module types.
     */
    static unsigned int elf_object_class = ELFCLASSNONE;
    static unsigned int elf_object_data = ELFDATANONE;
    static unsigned int elf_object_machinetype = EM_NONE;

    /**
     * A single place to initialise the libelf library. This must be called
     * before any libelf API calls are made.
     */
    static void
    libelf_initialise ()
    {
      static bool libelf_initialised = false;
      if (!libelf_initialised)
      {
        if (::elf_version (EV_CURRENT) == EV_NONE)
          libelf_error ("initialisation");
        libelf_initialised = true;
      }
    }

    section::section (file& file_, int index_)
      : file_ (&file_),
        index_ (index_),
        scn (0),
        data_ (0)
    {
      memset (&shdr, 0, sizeof (shdr));

      scn = ::elf_getscn (file_.get_elf (), index_);
      if (!scn)
        libelf_error ("elf_getscn: " + file_.name ());

      if (!::gelf_getshdr (scn, &shdr))
        libelf_error ("gelf_getshdr: " + file_.name ());

      if (shdr.sh_type != SHT_NULL)
      {
        name_ = file_.get_string (shdr.sh_name);
        data_ = ::elf_getdata (scn, NULL);
        if (!data_)
          libelf_error ("elf_getdata: " + name_ + '(' + file_.name () + ')');
      }
    }

    section::section (const section& orig)
      : file_ (orig.file_),
        index_ (orig.index_),
        name_ (orig.name_),
        scn (orig.scn),
        shdr (orig.shdr),
        data_ (orig.data_)
    {
    }

    section::section ()
      : file_ (0),
        index_ (-1),
        scn (0),
        data_ (0)
    {
      memset (&shdr, 0, sizeof (shdr));
    }

    int
    section::index () const
    {
      check ();
      return index_;
    }

    const std::string&
    section::name () const
    {
      check ();
      return name_;
    }

    elf_data*
    section::data ()
    {
      check ();
      return data_;
    }

    elf_word
    section::type () const
    {
      check ();
      return shdr.sh_type;
    }

    elf_xword
    section::flags () const
    {
      check ();
      return shdr.sh_flags;
    }

    elf_addr
    section::address () const
    {
      check ();
      return shdr.sh_addr;
    }

    elf_xword
    section::alignment () const
    {
      check ();
      return shdr.sh_addralign;
    }

    elf_off
    section::offset () const
    {
      check ();
      return shdr.sh_offset;
    }

    elf_word
    section::link () const
    {
      check ();
      return shdr.sh_link;
    }

    elf_word
    section::info () const
    {
      check ();
      return shdr.sh_info;
    }

    elf_xword
    section::size () const
    {
      check ();
      return shdr.sh_size;
    }

    elf_xword
    section::entry_size () const
    {
      check ();
      return shdr.sh_entsize;
    }

    int
    section::entries () const
    {
      return size () / entry_size ();
    }

    void
    section::check () const
    {
      if (!file_ || (index_ < 0))
        throw rld::error ("Invalid section.", "section:check:");
    }

    file::file ()
      : fd_ (-1),
        archive (false),
        writable (false),
        elf_ (0),
        oclass (0),
        ident_str (0),
        ident_size (0)
    {
      memset (&ehdr, 0, sizeof (ehdr));
      memset (&phdr, 0, sizeof (phdr));
    }

    file::~file ()
    {
      end ();
    }

    void
    file::begin (const std::string& name__, int fd__, const bool writable_)
    {
      begin (name__, fd__, writable_, 0, 0);
    }

    void
    file::begin (const std::string& name__, file& archive_, off_t offset)
    {
      archive_.check ("begin:archive");

      if (archive_.writable)
        throw rld::error ("archive is writable", "elf:file:begin");

      begin (name__, archive_.fd_, false, &archive_, offset);
    }

    #define rld_archive_fhdr_size (60)

    void
    file::begin (const std::string& name__,
                 int                fd__,
                 const bool         writable_,
                 file*              archive_,
                 off_t              offset_)
    {
      if (fd__ < 0)
        throw rld::error ("no file descriptor", "elf:file:begin");

      /*
       * Begin's are not nesting.
       */
      if (elf_ || (fd_ >= 0))
        throw rld::error ("already called", "elf:file:begin");

      /*
       * Cannot write directly into archive. Create a file then archive it.
       */
      if (archive_ && writable_)
        throw rld::error ("cannot write into archives directly",
                          "elf:file:begin");

      libelf_initialise ();

      /*
       * Is this image part of an archive ?
       */
      if (archive_)
      {
        ssize_t offset = offset_ - rld_archive_fhdr_size;
        if (::elf_rand (archive_->elf_, offset) != offset)
          libelf_error ("rand: " + archive_->name_);
      }

      /*
       * Note, the elf passed is either the archive or NULL.
       */
      elf* elf__ = ::elf_begin (fd__,
                                writable_ ? ELF_C_WRITE : ELF_C_READ,
                                archive_ ? archive_->elf_ : 0);
      if (!elf__)
        libelf_error ("begin: " + name__);

      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "elf::begin: " << elf__ << ' ' << name__ << std::endl;

      elf_kind ek = ::elf_kind (elf__);

      /*
       * If this is inside an archive it must be an ELF file.
       */

      if (archive_ && (ek != ELF_K_ELF))
        throw rld::error ("File format in archive not ELF", "elf:file:begin: " + name__);
      else
      {
        if (ek == ELF_K_AR)
          archive = true;
        else if (ek == ELF_K_ELF)
          archive = false;
        else
          throw rld::error ("File format not ELF or archive",
                            "elf:file:begin: " + name__);
      }

      if (!writable_)
      {
        /*
         * If an ELF file make sure they all match. On the first file that
         * begins an ELF session record its settings.
         */
        if (ek == ELF_K_ELF)
        {
          oclass = ::gelf_getclass (elf__);
          ident_str = elf_getident (elf__, &ident_size);
        }
      }

      fd_ = fd__;
      name_ = name__;
      writable = writable_;
      elf_ = elf__;

      if (!archive)
        load_header ();
    }

    void
    file::end ()
    {
      if (elf_)
      {
        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << "libelf::end: " << elf_
                    << ' ' << name_ << std::endl;
        ::elf_end (elf_);
      }

      fd_ = -1;
      name_.clear ();
      archive = false;
      elf_ = 0;
      oclass = 0;
      ident_str = 0;
      ident_size = 0;
      memset (&ehdr, 0, sizeof (ehdr));
      memset (&phdr, 0, sizeof (phdr));
      stab.clear ();
      secs.clear ();
    }

    void
    file::load_header ()
    {
      check ("get_header");

      if (::gelf_getehdr (elf_, &ehdr) == NULL)
        error ("get-header");
    }

    unsigned int
    file::machinetype () const
    {
      check ("machinetype");
      return ehdr.e_machine;
    }

    unsigned int
    file::type () const
    {
      check ("type");
      return ehdr.e_type;
    }

    unsigned int
    file::object_class () const
    {
      check ("object_class");
      return oclass;
    }

    unsigned int
    file::data_type () const
    {
      check ("data_type");
      if (!ident_str)
        throw rld::error ("No ELF ident str", "elf:file:data_type: " + name_);
      return ident_str[EI_DATA];
    }

    bool
    file::is_archive () const
    {
      check ("is_archive");
      return archive;
    }

    bool
    file::is_executable () const
    {
      check ("is_executable");
      return ehdr.e_type != ET_REL;
    }

    bool
    file::is_relocatable() const
    {
      check ("is_relocatable");
      return ehdr.e_type == ET_REL;
    }

    int
    file::section_count () const
    {
      check ("section_count");
      return ehdr.e_shnum;
    }

    void
    file::load_sections ()
    {
      if (secs.empty ())
      {
        check ("load_sections_headers");
        for (int sn = 0; sn < section_count (); ++sn)
          secs.push_back (section (*this, sn));
      }
    }

    void
    file::get_sections (sections& filtered_secs, unsigned int type)
    {
      load_sections ();
      filtered_secs.clear ();
      for (sections::iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        if ((type == 0) || ((*si).type () == type))
          filtered_secs.push_back (*si);
      }
    }

    void
    file::load_symbols ()
    {
      if (symbols.empty ())
      {
        sections symbol_secs;

        get_sections (symbol_secs, SHT_SYMTAB);

        for (sections::iterator si = symbol_secs.begin ();
             si != symbol_secs.end ();
             ++si)
        {
          section& sec = *si;
          int      syms = sec.entries ();

          for (int s = 0; s < syms; ++s)
          {
            elf_sym esym;

            if (!::gelf_getsym (sec.data (), s, &esym))
             error ("gelf_getsym");

            std::string name = get_string (sec.link (), esym.st_name);

            if (!name.empty ())
            {
              symbols::symbol sym (name, esym);

              if (rld::verbose () >= RLD_VERBOSE_TRACE)
              {
                std::cout << "elf::symbol: ";
                sym.output (std::cout);
                std::cout << std::endl;
              }

              symbols.push_back (sym);
            }
          }
        }
      }
    }

    void
    file::get_symbols (symbols::pointers& filtered_syms,
                       bool               unresolved,
                       bool               local,
                       bool               weak,
                       bool               global)
    {
      if (rld::verbose () >= RLD_VERBOSE_DETAILS)
        std::cout << "elf:get-syms: unresolved:" << unresolved
                  << " local:" << local
                  << " weak:" << weak
                  << " global:" << global
                  << " " << name_
                  << std::endl;

      load_symbols ();

      filtered_syms.clear ();

      for (symbols::bucket::iterator si = symbols.begin ();
           si != symbols.end ();
           ++si)
      {
        symbols::symbol& sym = *si;

        int stype = sym.type ();
        int sbind = sym.binding ();

        /*
         * If wanting unresolved symbols and the type is no-type and the
         * section is undefined, or, the type is no-type or object or function
         * and the bind is local and we want local symbols, or the bind is weak
         * and we want weak symbols, or the bind is global and we want global
         * symbols then add the filtered symbols container.
         */
        bool add = false;

        if ((stype == STT_NOTYPE) && (sym.index () == SHN_UNDEF))
        {
          if (unresolved)
            add = true;
        }
        else if (!unresolved)
        {
          if (((stype == STT_NOTYPE) ||
               (stype == STT_OBJECT) ||
               (stype == STT_FUNC)) &&
              ((local && (sbind == STB_LOCAL)) ||
                  (weak && (sbind == STB_WEAK)) ||
               (global && (sbind == STB_GLOBAL))))
            add = true;
        }

        if (add)
          filtered_syms.push_back (&sym);
      }
    }

    int
    file::strings_section () const
    {
      check ("strings_sections");
      return ehdr.e_shstrndx;
    }

    std::string
    file::get_string (int section, size_t offset)
    {
      check ("get_string");
      char* s = ::elf_strptr (elf_, section, offset);
      if (!s)
        error ("elf_strptr");
      return s;
    }

    std::string
    file::get_string (size_t offset)
    {
      check ("get_string");
      char* s = ::elf_strptr (elf_, strings_section (), offset);
      if (!s)
        error ("elf_strptr");
      return s;
    }

#if 0
    void
    file::set_header (xxx)
    {
      elf_ehdr* ehdr_ = ::gelf_newehdr (elf_);

      if (ehdr == NULL)
        error ("set-header");

      ehdr->xx = xx;

      ::gelf_flagphdr (elf_, ELF_C_SET , ELF_F_DIRTY);
    }
#endif

    elf*
    file::get_elf ()
    {
      return elf_;
    }

    const std::string&
    file::name () const
    {
      return name_;
    }

    bool
    file::is_writable () const
    {
      return writable;
    }

    void
    file::check (const char* where) const
    {
      if (!elf_ || (fd_ < 0))
      {
        std::string w = where;
        throw rld::error ("no elf file or header", "elf:file:" + w);
      }
    }

    void
    file::check_writable (const char* where) const
    {
      check (where);
      if (!writable)
      {
        std::string w = where;
        throw rld::error ("not writable", "elf:file:" + w);
      }
    }

    void
    file::error (const char* where) const
    {
      std::string w = where;
      libelf_error (w + ": " + name_);
    }

    const std::string
    machine_type (unsigned int machinetype)
    {
      struct types_and_labels
      {
        const char*  name;        //< The RTEMS label.
        unsigned int machinetype; //< The machine type.
      };
      types_and_labels types_to_labels[] =
      {
        { "arm",     EM_ARM },
        { "avr",     EM_AVR },
        { "bfin",    EM_BLACKFIN },
        { "h8300",   EM_H8_300 },
        { "i386",    EM_386 },
     /* { "m32c",    EM_M32C }, Not in libelf I imported */
        { "m32r",    EM_M32R },
        { "m68k",    EM_68K },
        { "m68k",    EM_COLDFIRE },
        { "mips",    EM_MIPS },
        { "powerpc", EM_PPC },
        { "sh",      EM_SH },
        { "sparc",   EM_SPARC },
        { "sparc64", EM_SPARC },
        { 0,         EM_NONE }
      };

      int m = 0;
      while (types_to_labels[m].machinetype != EM_NONE)
      {
        if (machinetype == types_to_labels[m].machinetype)
          return types_to_labels[m].name;
        ++m;
      }

      std::ostringstream what;
      what << "unknown machine type: " << elf_object_machinetype;
      throw rld::error (what, "machine-type");
    }

    const std::string machine_type ()
    {
      return machine_type (elf_object_machinetype);
    }

    void
    check_file(const file& file)
    {
      if (elf_object_machinetype == EM_NONE)
        elf_object_machinetype = file.machinetype ();
      else if (file.machinetype () != elf_object_machinetype)
      {
        std::ostringstream oss;
        oss << "elf:check_file:" << file.name ()
            << ": " << elf_object_machinetype << '/' << file.machinetype ();
        throw rld::error ("Mixed machine types not supported.", oss.str ());
      }

      if (elf_object_class == ELFCLASSNONE)
        elf_object_class = file.object_class ();
      else if (file.object_class () != elf_object_class)
        throw rld::error ("Mixed classes not allowed (32bit/64bit).",
                          "elf:check_file: " + file.name ());

      if (elf_object_data == ELFDATANONE)
        elf_object_data = file.data_type ();
      else if (elf_object_data != file.data_type ())
        throw rld::error ("Mixed data types not allowed (LSB/MSB).",
                          "elf:check_file: " + file.name ());
    }

#if 0
    void
    load_symbols (rld::symbols::table& symbols,
                  rld::files::object&  object,
                  bool                 local,
                  bool                 weak,
                  bool                 global)
    {
      sections sections;
      get_section_headers (object, sections, SHT_SYMTAB);
      for (sections::iterator si = sections.begin ();
           si != sections.end ();
           ++si)
        load_symbol_table (symbols, object, *si, local, weak, global);
    }
#endif

  }
}
