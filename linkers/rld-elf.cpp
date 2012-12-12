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
    static unsigned int elf_object_machinetype = EM_NONE;
    static unsigned int elf_object_datatype = ELFDATANONE;

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

    relocation::relocation (const symbols::symbol& sym,
                            elf_addr               offset,
                            elf_xword              info,
                            elf_sxword             addend)
      : sym (&sym),
        offset_ (offset),
        info_ (info),
        addend_ (addend)
    {
    }

    relocation::relocation ()
      : sym (0),
        offset_ (0),
        info_ (0),
        addend_ (0)
    {
    }

    elf_addr
    relocation::offset () const
    {
      return offset_;
    }

    uint32_t
    relocation::type () const
    {
      return GELF_R_TYPE (info_);
    }

    elf_xword
    relocation::info () const
    {
      return info_;
    }

    elf_sxword
    relocation::addend () const
    {
      return addend_;
    }

    const symbols::symbol&
    relocation::symbol () const
    {
      if (sym)
        return *sym;
      throw rld::error ("no symbol", "elf:relocation");
    }

    section::section (file&              file_,
                      int                index_,
                      const std::string& name_,
                      elf_word           type,
                      elf_xword          alignment,
                      elf_xword          flags,
                      elf_addr           addr,
                      elf_off            offset,
                      elf_xword          size,
                      elf_word           link,
                      elf_word           info,
                      elf_xword          entry_size)
      : file_ (&file_),
        index_ (index_),
        name_ (name_),
        scn (0),
        data_ (0),
        rela (false)
    {
      if (!file_.is_writable ())
        throw rld::error ("not writable",
                          "elf:section" + file_.name () + " (" + name_ + ')');

      scn = ::elf_newscn (file_.get_elf ());
      if (!scn)
        libelf_error ("elf_newscn: " + name_ + " (" + file_.name () + ')');

      if (::gelf_getshdr(scn, &shdr) == 0)
        libelf_error ("gelf_getshdr: " + name_ + " (" + file_.name () + ')');

      shdr.sh_name = 0;
      shdr.sh_type = type;
      shdr.sh_flags = flags;
      shdr.sh_addr = addr;
      shdr.sh_offset = offset;
      shdr.sh_size = size;
      shdr.sh_link = link;
      shdr.sh_info = info;
      shdr.sh_addralign = alignment;
      shdr.sh_entsize = entry_size;

      if (type == SHT_NOBITS)
        add_data (ELF_T_BYTE, alignment, size);

      if (!gelf_update_shdr (scn, &shdr))
        libelf_error ("gelf_update_shdr: " + name_ + " (" + file_.name () + ')');
    }

    section::section (file& file_, int index_)
      : file_ (&file_),
        index_ (index_),
        scn (0),
        data_ (0),
        rela (false)
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
        data_ = ::elf_getdata (scn, 0);
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
        data_ (orig.data_),
        rela (orig.rela),
        relocs (orig.relocs)
    {
    }

    section::section ()
      : file_ (0),
        index_ (-1),
        scn (0),
        data_ (0),
        rela (false)
    {
      memset (&shdr, 0, sizeof (shdr));
    }

    void
    section::add_data (elf_type  type,
                       elf_xword alignment,
                       elf_xword size,
                       void*     buffer,
                       elf_off   offset)
    {
      check_writable ("add_data");

      data_ = ::elf_newdata(scn);
      if (!data_)
        libelf_error ("elf_newdata: " + name_ + " (" + file_->name () + ')');

      data_->d_type = type;
      data_->d_off = offset;
      data_->d_size = size;
      data_->d_align = alignment;
      data_->d_version = EV_CURRENT;
      data_->d_buf = buffer;

      if (!gelf_update_shdr (scn, &shdr))
        libelf_error ("gelf_update_shdr: " + name_ + " (" + file_->name () + ')');
    }

    int
    section::index () const
    {
      check ("index");
      return index_;
    }

    const std::string&
    section::name () const
    {
      check ("name");
      return name_;
    }

    elf_data*
    section::data ()
    {
      check ("data");
      return data_;
    }

    elf_word
    section::type () const
    {
      check ("type");
      return shdr.sh_type;
    }

    elf_xword
    section::flags () const
    {
      check ("flags");
      return shdr.sh_flags;
    }

    elf_addr
    section::address () const
    {
      check ("address");
      return shdr.sh_addr;
    }

    elf_xword
    section::alignment () const
    {
      check ("alignment");
      return shdr.sh_addralign;
    }

    elf_off
    section::offset () const
    {
      check ("offset");
      return shdr.sh_offset;
    }

    elf_word
    section::link () const
    {
      check ("link");
      return shdr.sh_link;
    }

    elf_word
    section::info () const
    {
      check ("info");
      return shdr.sh_info;
    }

    elf_xword
    section::size () const
    {
      check ("size");
      return shdr.sh_size;
    }

    elf_xword
    section::entry_size () const
    {
      check ("entry_size");
      return shdr.sh_entsize;
    }

    int
    section::entries () const
    {
      return size () / entry_size ();
    }

    bool
    section::get_reloc_type () const
    {
      return rela;
    }

    void
    section::set_name (unsigned int index)
    {
      check_writable ("set_name");
      shdr.sh_name = index;
      if (!gelf_update_shdr (scn, &shdr))
        libelf_error ("gelf_update_shdr: " + name_ + " (" + file_->name () + ')');
    }

    void
    section::set_reloc_type (bool rela_)
    {
      rela = rela_;
    }

    void
    section::add (const relocation& reloc)
    {
      relocs.push_back (reloc);
    }

    const relocations&
    section::get_relocations () const
    {
      return relocs;
    }

    void
    section::check (const char* where) const
    {
      if (!file_ || (index_ < 0) || !scn)
      {
        std::string w = where;
        throw rld::error ("Section not initialised.", "section:check:" + w);
      }
    }

    void
    section::check_writable (const char* where) const
    {
      check (where);
      if (!file_->is_writable ())
      {
        std::string w = where;
        throw rld::error ("File is read-only.", "section:check:");
      }
    }

    program_header::program_header ()
    {
      memset (&phdr, 0, sizeof (phdr));
    }

    program_header::~program_header ()
    {
    }

    void
    program_header::set (elf_word type,
                         elf_word flags,
                         elf_off offset,
                         elf_xword filesz,
                         elf_xword memsz,
                         elf_xword align,
                         elf_addr vaddr,
                         elf_addr paddr)
    {
      phdr.p_type = type;
      phdr.p_flags = flags;
      phdr.p_offset = offset;
      phdr.p_vaddr = vaddr;
      phdr.p_paddr = paddr;
      phdr.p_filesz = filesz;
      phdr.p_memsz = memsz;
      phdr.p_align = align;
    }

    file::file ()
      : fd_ (-1),
        archive (false),
        writable (false),
        elf_ (0),
        oclass (0),
        ident_str (0),
        ident_size (0),
        ehdr (0),
        phdr (0)
    {
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
        throw rld::error ("No file descriptor", "elf:file:begin");

      /*
       * Begin's are not nesting.
       */
      if (elf_ || (fd_ >= 0))
        throw rld::error ("Already called", "elf:file:begin");

      /*
       * Cannot write directly into archive. Create a file then archive it.
       */
      if (archive_ && writable_)
        throw rld::error ("Cannot write into archives directly",
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
        throw rld::error ("File format in archive not ELF",
                          "elf:file:begin: " + name__);
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

      if (!archive && !writable)
      {
        load_header ();
        load_sections ();
      }
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
        elf_ = 0;
      }

      if (fd_ >= 0)
      {
        if (!writable)
        {
          if (ehdr)
          {
            delete ehdr;
            ehdr = 0;
          }
          if (phdr)
          {
            delete phdr;
            phdr = 0;
          }
        }

        fd_ = -1;
        name_.clear ();
        archive = false;
        elf_ = 0;
        oclass = 0;
        ident_str = 0;
        ident_size = 0;
        writable = false;
        secs.clear ();
      }
    }

    void
    file::write ()
    {
      check_writable ("write");

      std::string shstrtab;

      for (section_table::iterator sti = secs.begin ();
           sti != secs.end ();
           ++sti)
      {
        section& sec = (*sti).second;
        int added_at = shstrtab.size ();
        shstrtab += '\0' + sec.name ();
        sec.set_name (added_at + 1);
      }

      unsigned int shstrtab_name = shstrtab.size () + 1;

      /*
       * Done this way to clang happy on darwin.
       */
      shstrtab += '\0';
      shstrtab += ".shstrtab";

      /*
       * Create the string table section.
       */
      section shstrsec (*this,
                        secs.size () + 1,          /* index */
                        ".shstrtab",               /* name */
                        SHT_STRTAB,                /* type */
                        1,                         /* alignment */
                        SHF_STRINGS | SHF_ALLOC,   /* flags */
                        0,                         /* address */
                        0,                         /* offset */
                        shstrtab.size ());         /* size */

      shstrsec.add_data (ELF_T_BYTE,
                         1,
                         shstrtab.size (),
                         (void*) shstrtab.c_str ());

      shstrsec.set_name (shstrtab_name);

      ::elf_setshstrndx (elf_, shstrsec.index ());
      ::elf_flagehdr (elf_, ELF_C_SET, ELF_F_DIRTY);

      if (elf_update (elf_, ELF_C_NULL) < 0)
        libelf_error ("elf_update:layout: " + name_);

      ::elf_flagphdr (elf_, ELF_C_SET, ELF_F_DIRTY);

      if (::elf_update (elf_, ELF_C_WRITE) < 0)
        libelf_error ("elf_update:write: " + name_);
    }

    void
    file::load_header ()
    {
      check ("load_header");

      if (!ehdr)
      {
        if (!writable)
          ehdr = new elf_ehdr;
        else
        {
          throw rld::error ("No ELF header; set the header first",
                            "elf:file:load_header: " + name_);
        }
      }

      if (::gelf_getehdr (elf_, ehdr) == 0)
        error ("gelf_getehdr");
    }

    unsigned int
    file::machinetype () const
    {
      check_ehdr ("machinetype");
      return ehdr->e_machine;
    }

    unsigned int
    file::type () const
    {
      check_ehdr ("type");
      return ehdr->e_type;
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
      check_ehdr ("is_executable");
      return ehdr->e_type != ET_REL;
    }

    bool
    file::is_relocatable() const
    {
      check_ehdr ("is_relocatable");
      return ehdr->e_type == ET_REL;
    }

    int
    file::section_count () const
    {
      check_ehdr ("section_count");
      return ehdr->e_shnum;
    }

    void
    file::load_sections ()
    {
      if (secs.empty ())
      {
        check ("load_sections_headers");
        for (int sn = 0; sn < section_count (); ++sn)
        {
          section sec (*this, sn);
          secs[sec.name ()] = sec;
        }
      }
    }

    void
    file::get_sections (sections& filtered_secs, unsigned int type)
    {
      load_sections ();
      filtered_secs.clear ();
      for (section_table::iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        section& sec = (*si).second;
        if ((type == 0) || (sec.type () == type))
          filtered_secs.push_back (&sec);
      }
    }

    section&
    file::get_section (int index)
    {
      load_sections ();
      for (section_table::iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        section& sec = (*si).second;
        if (index == sec.index ())
          return sec;
      }

      throw rld::error ("section index '" + rld::to_string (index) + "'not found",
                        "elf:file:get_section: " + name_);
    }

    int
    file::strings_section () const
    {
      check_ehdr ("strings_sections");
      return ehdr->e_shstrndx;
    }

    void
    file::load_symbols ()
    {
      if (symbols.empty ())
      {
        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << "elf:symbol: " << name () << std::endl;

        sections symbol_secs;

        get_sections (symbol_secs, SHT_SYMTAB);

        for (sections::iterator si = symbol_secs.begin ();
             si != symbol_secs.end ();
             ++si)
        {
          section& sec = *(*si);
          int      syms = sec.entries ();

          for (int s = 0; s < syms; ++s)
          {
            elf_sym esym;

            if (!::gelf_getsym (sec.data (), s, &esym))
             error ("gelf_getsym");

            std::string     name = get_string (sec.link (), esym.st_name);
            symbols::symbol sym (s, name, esym);

            if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
            {
              std::cout << "elf:symbol: ";
              sym.output (std::cout);
              std::cout << std::endl;
            }

            symbols.push_back (sym);
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

        if ((stype == STT_NOTYPE) &&
            (sbind == STB_GLOBAL) &&
            (sym.section_index () == SHN_UNDEF))
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

    const symbols::symbol&
    file::get_symbol (const int index) const
    {
      for (symbols::bucket::const_iterator si = symbols.begin ();
           si != symbols.end ();
           ++si)
      {
        const symbols::symbol& sym = *si;
        if (index == sym.index ())
          return sym;
      }

      throw rld::error ("symbol index '" + rld::to_string (index) + "' not found",
                        "elf:file:get_symbol: " + name_);
    }

    void
    file::load_relocations ()
    {
      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "elf:reloc: " << name () << std::endl;

      sections rel_secs;

      get_sections (rel_secs, SHT_REL);
      get_sections (rel_secs, SHT_RELA);

      for (sections::iterator si = rel_secs.begin ();
           si != rel_secs.end ();
           ++si)
      {
        section& sec = *(*si);
        section& targetsec = get_section (sec.info ());
        int      rels = sec.entries ();
        bool     rela = sec.type () == SHT_RELA;

        targetsec.set_reloc_type (rela);

        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << "elf:reloc: " << sec.name ()
                    << " -> " << targetsec.name ()
                    << std::endl;

        for (int r = 0; r < rels; ++r)
        {
          if (rela)
          {
            elf_rela erela;

            if (!::gelf_getrela (sec.data (), r, &erela))
              error ("gelf_getrela");

            if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
              std::cout << "elf:reloc: rela: offset: " << erela.r_offset
                        << " sym:" << GELF_R_SYM (erela.r_info)
                        << " type:" << GELF_R_TYPE (erela.r_info)
                        << " addend:" << erela.r_addend
                        << std::endl;

            /*
             * The target section is updated with the fix up, and symbol
             * section indicates the section offset being referenced by the
             * fixup.
             */

            const symbols::symbol& sym = get_symbol (GELF_R_SYM (erela.r_info));

            relocation reloc (sym,
                              erela.r_offset,
                              erela.r_info,
                              erela.r_addend);

            targetsec.add (reloc);
          }
          else
          {
            elf_rel erel;

            if (!::gelf_getrel (sec.data (), r, &erel))
              error ("gelf_getrel");

            if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
              std::cout << "elf:reloc: rel: offset: " << erel.r_offset
                        << " sym:" << GELF_R_SYM (erel.r_info)
                        << " type:" << GELF_R_TYPE (erel.r_info)
                        << std::endl;

            const symbols::symbol& sym = get_symbol (erel.r_info);

            relocation reloc (sym, erel.r_offset, erel.r_info);

            targetsec.add (reloc);
          }
        }
      }
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

    void
    file::set_header (elf_half      type,
                      int           class_,
                      elf_half      machinetype,
                      unsigned char datatype)
    {
      check_writable ("set_header");

      if (ehdr)
          throw rld::error ("ELF header already set",
                            "elf:file:set_header: " + name_);

      ehdr = (elf_ehdr*) ::gelf_newehdr (elf_, class_);
      if (ehdr == 0)
        error ("gelf_newehdr");

      if (::gelf_getehdr (elf_, ehdr) == 0)
        error ("gelf_getehdr");

      ehdr->e_type = type;
      ehdr->e_machine = machinetype;
      ehdr->e_flags = 0;
      ehdr->e_ident[EI_DATA] = datatype;
      ehdr->e_version = EV_CURRENT;

      ::elf_flagphdr (elf_, ELF_C_SET , ELF_F_DIRTY);
    }

    void
    file::add (section& sec)
    {
      check_writable ("add");
      secs[sec.name ()] = sec;
    }

    void
    file::add (program_header& phdr)
    {
      check_writable ("add");
      phdrs.push_back (phdr);
    }

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
        throw rld::error ("No ELF file or file descriptor", "elf:file:" + w);
      }
    }

    void
    file::check_ehdr (const char* where) const
    {
      check (where);
      if (!ehdr)
      {
        std::string w = where;
        throw rld::error ("no elf header", "elf:file:" + w);
      }
    }

    void
    file::check_phdr (const char* where) const
    {
      check (where);
      if (!phdr)
      {
        std::string w = where;
        throw rld::error ("no elf program header", "elf:file:" + w);
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

    const std::string
    machine_type ()
    {
      return machine_type (elf_object_machinetype);
    }

    unsigned int
    object_class ()
    {
      return elf_object_class;
    }

    unsigned int
    object_machine_type ()
    {
      return elf_object_machinetype;
    }

    unsigned int
    object_datatype ()
    {
      return elf_object_datatype;
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

      if (elf_object_datatype == ELFDATANONE)
        elf_object_datatype = file.data_type ();
      else if (elf_object_datatype != file.data_type ())
        throw rld::error ("Mixed data types not allowed (LSB/MSB).",
                          "elf:check_file: " + file.name ());
    }

  }
}
