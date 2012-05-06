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
 * @brief RTEMS Linker ELF module manages the ELF format images.
 *
 */

#include <string.h>

#include <rld.h>

namespace rld
{
  namespace elf
  {
    void error (const std::string& where)
    {
      throw rld::error (::elf_errmsg (-1), "elf:" + where);
    }

    /**
     * We record the first class, machine and .. type of object file we get the
     * header of and all header must match. We cannot mix object module types.
     */
    static int elf_object_class = ELFCLASSNONE;
    static int elf_object_data = ELFDATANONE;
    static int elf_object_machinetype = EM_NONE;

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
          error ("initialisation");
        libelf_initialised = true;
      }
    }

    /**
     * Return the RTEMS target type given the ELF machine type.
     */
    const std::string
    machine_type ()
    {
      struct types_and_labels
      {
        const char* name;        //< The RTEMS label.
        int         machinetype; //< The machine type.
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
        if (elf_object_machinetype == types_to_labels[m].machinetype)
          return types_to_labels[m].name;
        ++m;
      }

      std::ostringstream what;
      what << "unknown machine type: " << elf_object_machinetype;
      throw rld::error (what, "machine-type");
    }

    section::section (int          index,
                      std::string& name,
                      elf_scn*     scn, 
                      elf_shdr&    shdr)
      : index (index),
        name (name),
        scn (scn),
        shdr (shdr)
    {
      data = ::elf_getdata (scn, NULL);
      if (!data)
        error ("elf_getdata");
    }

    section::section ()
      : index (-1),
        scn (0),
        data (0)
    {
      memset (&shdr, 0, sizeof (shdr));
    }

    #define rld_archive_fhdr_size (60)

    void
    begin (rld::files::image& image)
    {
      libelf_initialise ();

      /*
       * Begin's are not nesting.
       */
      Elf* elf = image.elf ();
      if (elf)
          error ("begin: already done: " + image.name ().full ());

      /*
       * Is this image part of an archive ?
       */
      elf = image.elf (true);
      if (elf)
      {
        ssize_t offset = image.name ().offset () - rld_archive_fhdr_size;

        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << "elf::rand: " << elf << " offset:" << offset
                    << ' ' << image.name ().full () << std::endl;

        if (::elf_rand (elf, offset) != offset)
          error ("begin:" + image.name ().full ());
      }

      /*
       * Note, the elf passed is either the archive or NULL.
       */
      elf = ::elf_begin (image.fd (), ELF_C_READ, elf);
      if (!elf)
        error ("begin:" + image.name ().full ());

      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "elf::begin: " << elf 
                  << ' ' << image.name ().full () << std::endl;

      image.set_elf (elf);

      elf_kind ek = ::elf_kind (elf);
      if (image.name ().is_archive ())
      {
        if (ek != ELF_K_AR)
          throw rld::error ("File not an ar archive", "libelf:" + image.name ().full ());
      }
      else if (ek != ELF_K_ELF)
        throw rld::error ("File format not ELF", "libelf:" + image.name ().full ());

      /*
       * If an ELF file make sure they all match. On the first file that begins
       * an ELF session record its settings.
       */
      if (ek == ELF_K_ELF)
      {
        int cl = ::gelf_getclass (elf);

        if (elf_object_class == ELFCLASSNONE)
          elf_object_class = cl;
        else if (cl != elf_object_class)
          throw rld::error ("Mixed classes not allowed (32bit/64bit).",
                            "begin:" + image.name ().full ());
      
        char* ident = elf_getident (elf, NULL);

        if (elf_object_data == ELFDATANONE)
          elf_object_data = ident[EI_DATA];
        else if (elf_object_data != ident[EI_DATA])
          throw rld::error ("Mixed data types not allowed (LSB/MSB).",
                            "begin:" + image.name ().full ());
      }
    }

    void
    end (rld::files::image& image)
    {
      ::Elf* elf = image.elf ();
      if (elf)
      {
        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << "elf::end: " << elf 
                    << ' ' << image.name ().full () << std::endl;
        ::elf_end (elf);
      }
      image.set_elf (0);
    }

    void
    get_header (rld::files::image& image, elf_ehdr& ehdr)
    {
      if (::gelf_getehdr (image.elf (), &ehdr) == NULL)
        error ("get-header:" + image.name ().full ());
      
      if ((ehdr.e_type != ET_EXEC) && (ehdr.e_type != ET_REL))
        throw rld::error ("Invalid ELF type (only ET_EXEC/ET_REL supported).",
                          "get-header:" + image.name ().full ());

      if (elf_object_machinetype == EM_NONE)
        elf_object_machinetype = ehdr.e_machine;
      else if (elf_object_machinetype != ehdr.e_machine)
      {
        std::ostringstream oss;
        oss << "get-header:" << image.name ().full () 
            << ": " << elf_object_machinetype << '/' << ehdr.e_machine;
        throw rld::error ("Mixed machine types not supported.", oss.str ());
      }
    }

    void
    get_section_headers (rld::files::object& object,
                         sections&           secs,
                         unsigned int        type)
    {
      for (int sn = 0; sn < object.sections (); ++sn)
      {
        ::Elf_Scn* scn = ::elf_getscn (object.elf (), sn);
        if (!scn)
          error ("elf_getscn:" + object.name ().full ());
        ::GElf_Shdr shdr;
        if (!::gelf_getshdr (scn, &shdr))
          error ("gelf_getshdr:" + object.name ().full ());
        if (shdr.sh_type == type)
        {
          std::string name = get_string (object, 
                                         object.section_strings (),
                                         shdr.sh_name);
          secs.push_back (section (sn, name, scn, shdr));
        }
      }
    }

    void
    load_symbol_table (rld::symbols::table& exported,
                       rld::files::object&  object, 
                       section&             sec,
                       bool                 local,
                       bool                 weak,
                       bool                 global)
    {
      int count = sec.shdr.sh_size / sec.shdr.sh_entsize;
      for (int s = 0; s < count; ++s)
      {
        GElf_Sym esym;
        if (!::gelf_getsym (sec.data, s, &esym))
          error ("gelf_getsym");
        std::string name = get_string (object, sec.shdr.sh_link, esym.st_name);
        if (!name.empty ())
        {
          int stype = GELF_ST_TYPE (esym.st_info);
          int sbind = GELF_ST_BIND (esym.st_info);
          if (rld::verbose () >= RLD_VERBOSE_TRACE)
          {
            rld::symbols::symbol sym (name, esym);
            std::cout << "elf::symbol: ";
            sym.output (std::cout);
            std::cout << std::endl;
          }
          if ((stype == STT_NOTYPE) && (esym.st_shndx == SHN_UNDEF))
            object.unresolved_symbols ()[name] = rld::symbols::symbol (name, esym);
          else if (((stype == STT_NOTYPE) || 
                    (stype == STT_OBJECT) || 
                    (stype == STT_FUNC)) &&
                   ((local && (sbind == STB_LOCAL)) ||
                    (weak && (sbind == STB_WEAK)) ||
                    (global && (sbind == STB_GLOBAL))))
          {
            exported[name] = rld::symbols::symbol (name, object, esym);;
            object.external_symbols ().push_back (&exported[name]);
          }
        }
      }
    }

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

    std::string
    get_string (rld::files::object& object, 
                int                 section,
                size_t              offset)
    {
      char* s = ::elf_strptr (object.elf (), section, offset);
      if (!s)
        error ("elf_strptr");
      return s;
    }

  }
}
