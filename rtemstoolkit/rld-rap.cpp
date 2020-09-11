/*
 * Copyright (c) 2012, Chris Johns <chrisj@rtems.org>
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
 * @ingroup rtems_ld
 *
 * @brief RTEMS Linker.
 *
 * @todo Set the RAP alignment as the max of all alignments.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <algorithm>
#include <list>
#include <iomanip>

#include <rld.h>
#include <rld-compression.h>
#include <rld-rap.h>

namespace rld
{
  namespace rap
  {

    /**
     * Output details or not.
     */
    bool add_obj_details = true;

    /**
     * Store the path of object files.
     */
    std::string rpath;

    /**
     * The names of the RAP sections.
     */
    static const char* section_names[rap_secs] =
    {
      ".text",
      ".const",
      ".ctor",
      ".dtor",
      ".data",
      ".bss"
    };

    /**
     * RAP relocation record. This one does not have const fields.
     */
    struct relocation
    {
      uint32_t    offset;    //< The offset in the section to apply the fixup.
      uint32_t    info;      //< The ELF info record.
      uint32_t    addend;    //< The ELF constant addend.
      std::string symname;   //< The symbol name if there is one.
      uint32_t    symtype;   //< The type of symbol.
      int         symsect;   //< The symbol's RAP section.
      uint32_t    symvalue;  //< The symbol's default value.
      uint32_t    symbinding;//< The symbol's binding.

      /**
       * Construct the relocation using the file relocation, the offset of the
       * section in the target RAP section and the RAP section of the symbol.
       */
      relocation (const files::relocation& reloc, const uint32_t offset);
    };

    /**
     * Relocation records.
     */
    typedef std::vector < relocation > relocations;

    /**
     * Relocation symname sorter for the relocations container.
     */
    class reloc_symname_compare
    {
    public:
      bool operator () (const relocation& lhs,
                        const relocation& rhs) const {
        return lhs.symname < rhs.symname;
      }
    };

    /**
     * Relocation offset sorter for the relocations container.
     */
    class reloc_offset_compare
    {
    public:
      bool operator () (const relocation& lhs,
                        const relocation& rhs) const {
        if (lhs.symname == rhs.symname)
          return lhs.offset < rhs.offset;
        else return false;
      }
    };

    /**
     * An object section's offset, size and alignment.
     */
    struct osection
    {
      std::string name;     //< The name of the section.
      uint32_t    offset;   //< The offset in the object file.
      uint32_t    size;     //< The size of this section.
      uint32_t    align;    //< The alignment.
      uint32_t    relocs;   //< The number of relocations.
      uint64_t    flags;    //< The flags.

      /**
       * Construct the object section.
       */
      osection (const std::string& name,
                uint32_t           offset,
                uint32_t           size,
                uint32_t           align,
                uint32_t           relocs,
                uint64_t           flags);

      /**
       * Default constructor.
       */
      osection ();
    };

    /**
     * Map of object file section offsets keyed by the object file section
     * index. This is used when adding the external symbols so the symbol's
     * value can be adjusted by the offset of the section in the RAP section.
     */
    typedef std::map < int, osection > osections;

    /**
     * An ordered container of object section indexes. We need the same
     * order so the alignments match up with the layout.
     */
    typedef std::vector < int > osecindexes;

    /**
     * Section detail will be written into RAP file
     */
    struct section_detail
    {
      uint32_t name;   //< The offset in the strtable.
      uint32_t offset; //< The offset in the rap section.
      uint32_t id;     //< The rap id.
      uint32_t size;   //< The size of the section.

      /* Constructor */
      section_detail (uint32_t name, uint32_t offset, uint32_t id, uint32_t size);
    };

    /*
     * A container of section detail
     */
    typedef std::list < section_detail > section_details;

    /**
     * The RAP section data.
     */
    struct section
    {
      std::string name;      //< The name of the section.
      uint32_t    offset;    //< The offset of the section.
      bool        rela;      //< The relocation record has an addend field.
      relocations relocs;    //< The relocations for this section.
      osections   osecs;     //< The object section index.
      osecindexes osindexes; //< The object section indexes in order.

      /**
       * Default constructor.
       */
      section ();

      /**
       * Clear the section.
       */
      void clear ();

      /**
       * The size of the section given the offset.
       */
      uint32_t size (uint32_t offset = 0) const;

      /**
       * The alignment of the first section.
       */
      uint32_t alignment () const;

      /**
       * The alignment of the object section given its index.
       */
      uint32_t alignment (int index) const;

      /**
       * Set the offset of this section based on the previous section.
       */
      void set_offset (const section& sec);

      /**
       * Return the object section given the index.
       */
      const osection& get_osection (int index) const;

      /**
       * Output helper function to report the sections in an object file. This
       * is useful when seeing the flags in the sections.
       */
      void output ();
    };

    /**
     * A symbol. This matches the symbol structure 'rtems_rtl_obj_sym_t' in the
     * target code.
     */
    struct external
    {
      /**
       * Size of an external in the RAP file.
       */
      static const uint32_t rap_size = sizeof (uint32_t) * 3;

      const uint32_t name;  //< The string table's name index.
      const sections sec;   //< The section the symbols belongs to.
      const uint32_t value; //< The offset from the section base.
      const uint32_t data;  //< The ELF st.info field.

      /**
       * The constructor.
       */
      external (const uint32_t name,
                const sections sec,
                const uint32_t value,
                const uint32_t data);

      /**
       * Copy constructor.
       */
      external (const external& orig);

    };

    /**
     * A container of externals.
     */
    typedef std::list < external > externals;

    /**
     * The specific data for each object we need to collect to create the RAP
     * format file.
     */
    struct object
    {
      files::object&  obj;            //< The object file.
      files::sections text;           //< All executable code.
      files::sections const_;         //< All read only data.
      files::sections ctor;           //< The static constructor table.
      files::sections dtor;           //< The static destructor table.
      files::sections data;           //< All initialised read/write data.
      files::sections bss;            //< All uninitialised read/write data
      files::sections symtab;         //< All exported symbols.
      files::sections strtab;         //< All exported strings.
      section         secs[rap_secs]; //< The sections of interest.

      /**
       * The constructor. Need to have an object file to create.
       */
      object (files::object& obj);

      /**
       * The copy constructor.
       */
      object (const object& orig);

      /**
       * Find the section type that matches the section index.
       */
      sections find (const uint32_t index) const;

      /**
       * The total number of relocations in the object file.
       */
      uint32_t get_relocations () const;

      /**
       * The total number of relocations for a specific RAP section in the
       * object file.
       */
      uint32_t get_relocations (int sec) const;

      /**
       * Output the object file details..
       */
      void output ();

    private:
      /**
       * No default constructor allowed.
       */
      object ();
    };

    /**
     * A container of objects.
     */
    typedef std::list < object > objects;

    /**
     * The RAP image.
     */
    class image
    {
    public:
      /**
       * Construct the image.
       */
      image ();

      /**
       * Load the layout data from the object files.
       *
       * @param app_objects The object files in the application.
       * @param init The initialisation entry point label.
       * @param fini The finish entry point label.
       */
      void layout (const files::object_list& app_objects,
                   const std::string&        init,
                   const std::string&        fini);

      /**
       * Collection the symbols from the object file.
       *
       * @param obj The object file to collection the symbol from.
       */
      void collect_symbols (object& obj);

      /**
       * Write the compressed output file. This is the top level write
       * interface.
       *
       * @param comp The compressor.
       */
      void write (compress::compressor& comp);

      /**
       * Write the RAP section to the compressed output file given the object files.
       * Check to make sure the size in the layout and the size written match.
       *
       * @param comp The compressor.
       * @param sec The RAP setion to write.
       */
      void write (compress::compressor& comp, sections sec);

      /**
       * Write the sections to the compressed output file. The file sections
       * are used to ensure the alignment. The offset is used to ensure the
       * alignment of the first section of the object when it is written.
       *
       * @param comp The compressor.
       * @param obj The object file the sections are part of.
       * @param secs The container of file sections to write.
       * @param offset The current offset in the RAP section.
       */
      void write (compress::compressor&  comp,
                  files::object&         obj,
                  const files::sections& secs,
                  uint32_t&              offset);

      /**
       * Write the external symbols.
       */
      void write_externals (compress::compressor& comp);

      /**
       * Write the relocation records for all the object files.
       */
      void write_relocations (compress::compressor& comp);

      /**
       * Write the details of the files.
       */
      void write_details (compress::compressor& comp);

      /**
       * The total number of relocations for a specific RAP section in the
       * image.
       */
      uint32_t get_relocations (int sec) const;

      /**
       * Clear the image values.
       */
      void clear ();

      /**
       * Report the RAP section's size.
       */
      uint32_t section_size (sections sec) const;

      /**
       * Find a symbol name in the string table.
       */
      std::size_t find_in_strtab (const std::string& symname);

    private:

      objects     objs;                //< The RAP objects
      uint32_t    sec_size[rap_secs];  //< The sections of interest.
      uint32_t    sec_align[rap_secs]; //< The sections of interest.
      bool        sec_rela[rap_secs];  //< The sections of interest.
      externals   externs;             //< The symbols in the image
      uint32_t    symtab_size;         //< The size of the symbols.
      std::string strtab;              //< The strings table.
      uint32_t    relocs_size;         //< The relocations size.
      uint32_t    init_off;            //< The strtab offset to the init label.
      uint32_t    fini_off;            //< The strtab offset to the fini label.
    };

    /*
     * Per machine specific special handling.
     */
    bool
    machine_symbol_check (const symbols::symbol& sym)
    {
      int symsec = sym.section_index ();

      /*
       * Ignore section index 0
       */
      if (symsec == 0)
        return false;

      /*
       * Ignore sparc common section
       */
      if ((elf::object_machine_type () == EM_SPARC) && (symsec == 65522))
        return false;

      return true;
    }

    bool
    machine_relocation_check (const files::relocation& reloc)
    {
      /*
       * Drop some ARM relocations.
       */
      if (elf::object_machine_type () == EM_ARM)
      {
        switch (reloc.type)
        {
          case 40:  /* R_ARM_V4BX */
            return false;
          default:
            break;
        }
      }

      return true;
    }

    const char*
    section_name (int sec)
    {
      if (sec < rap_secs)
        return section_names[sec];
      throw rld::error ("Invalid section '" + rld::to_string (sec) + "'",
                        "rap::section-name");
    }

    /**
     * Update the offset taking into account the alignment.
     *
     * @param offset The current offset.
     * @param size The size to move the offset by.
     * @param alignment The alignment of the offset.
     * @return uint32_t The new aligned offset.
     */
    uint32_t align_offset (uint32_t offset, uint32_t size, uint32_t alignment)
    {
      offset += size;

      if (alignment > 1)
      {
        uint32_t mask = alignment - 1;
        if (offset & mask)
        {
          offset &= ~mask;
          offset += alignment;
        }
      }

      return offset;
    }

    relocation::relocation (const files::relocation& reloc,
                            const uint32_t           offset)
      : offset (reloc.offset + offset),
        info (reloc.info),
        addend (reloc.addend),
        symname (reloc.symname),
        symtype (reloc.symtype),
        symsect (reloc.symsect),
        symvalue (reloc.symvalue),
        symbinding (reloc.symbinding)
    {
    }

    section_detail::section_detail (uint32_t name,
                                    uint32_t offset,
                                    uint32_t id,
                                    uint32_t size)
      : name (name),
        offset (offset),
        id (id),
        size (size)
    {
    }

    osection::osection (const std::string& name,
                        uint32_t           offset,
                        uint32_t           size,
                        uint32_t           align,
                        uint32_t           relocs,
                        uint64_t           flags)
      : name (name),
        offset (offset),
        size (size),
        align (align),
        relocs (relocs),
        flags (flags)
    {
    }

    osection::osection ()
      : offset (0),
        size (0),
        align (0),
        relocs (0),
        flags (0)
    {
    }

    section::section ()
      : offset (0),
        rela (false)
    {
    }

    void
    section::clear ()
    {
      offset = 0;
      rela = false;
    }

    uint32_t
    section::size (uint32_t offset_) const
    {
      uint32_t end = offset_;
      if (end == 0)
        end = offset;
      for (size_t si = 0; si < osindexes.size (); ++si)
      {
        const osection& osec = get_osection (osindexes[si]);
        end = align_offset (end, 0, osec.align);
        end += osec.size;
      }
      return end - offset;
    }

    uint32_t
    section::alignment () const
    {
      if (!osindexes.empty ())
      {
        const osection& osec = get_osection (osindexes[0]);
        return osec.align;
      }
      return 0;
    }

    uint32_t
    section::alignment (int index) const
    {
      const osection& osec = get_osection (index);
      return osec.align;
    }

    void
    section::set_offset (const section& sec)
    {
      uint32_t align = alignment ();
      offset = align_offset (sec.offset, sec.size (), align);
      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "rap:section::set-offset: " << name
                  << " offset=" << offset
                  << " size=" << size ()
                  << " align=" << align
                  << " sec.offset=" << sec.offset
                  << " sec.size=" << sec.size (sec.offset)
                  << std::endl;
    }

    const osection&
    section::get_osection (int index) const
    {
      osections::const_iterator osi = osecs.find (index);
      if (osi == osecs.end ())
        throw rld::error ("Invalid object seciton index in '" + name +"': index=" +
                          rld::to_string (index),
                          "rap::section");
      return (*osi).second;
    }

    /**
     * Output helper function to report the sections in an object file. This is
     * useful when seeing the flags in the sections.
     */
    void
    section::output ()
    {
      if (!osindexes.empty ())
      {
        std::cout << ' ' << name
                  << ": size: " << size (offset)
                  << " offset: " << offset
                  << " rela: " << (rela ? "yes" : "no")
                  << std::endl;

        for (osecindexes::const_iterator osi = osindexes.begin ();
             osi != osindexes.end ();
             ++osi)
        {
          const osection& osec = get_osection (*osi);

          if (osec.size)
          {
            #define SF(f, i, c) if (osec.flags & (f)) flags[i] = c

            std::string flags ("--------------");

            SF (SHF_WRITE,            0, 'W');
            SF (SHF_ALLOC,            1, 'A');
            SF (SHF_EXECINSTR,        2, 'E');
            SF (SHF_MERGE,            3, 'M');
            SF (SHF_STRINGS,          4, 'S');
            SF (SHF_INFO_LINK,        5, 'I');
            SF (SHF_LINK_ORDER,       6, 'L');
            SF (SHF_OS_NONCONFORMING, 7, 'N');
            SF (SHF_GROUP,            8, 'G');
            SF (SHF_TLS,              9, 'T');
            SF (SHF_AMD64_LARGE,     10, 'a');
            SF (SHF_ENTRYSECT,       11, 'e');
            SF (SHF_COMDEF,          12, 'c');
            SF (SHF_ORDERED,         13, 'O');

            std::cout << "  " << std::left
                      << std::setw (15) << osec.name
                      << " " << flags
                      << " size: " << std::setw (5) << osec.size
                      << " align: " << std::setw (3) << osec.align
                      << " relocs: " << std::setw (4) << osec.relocs
                      << " offset: " << std::setw (5) << osec.offset
                      << std::hex
                      << " image: 0x" << offset + osec.offset
                      << std::dec << std::right << std::endl;
          }
        }
      }
    }

    /**
     * Helper for for_each to merge the related object sections into the RAP
     * section.
     */
    class section_merge:
      public std::unary_function < const files::section, void >
    {
    public:

      section_merge (object& obj, section& sec);

      ~section_merge ();

      void operator () (const files::section& fsec);

    private:

      object&  obj;
      section& sec;
    };

    section_merge::section_merge (object& obj, section& sec)
      : obj (obj),
        sec (sec)
    {
      sec.offset = 0;
      sec.rela = false;
    }

    section_merge::~section_merge ()
    {
      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "rap:section-merge: " << sec.name
                  << " size=" << sec.size ()
                  << " offset=" << sec.offset
                  << " " << obj.obj.name ().full ()  << std::endl;
    }

    void
    section_merge::operator () (const files::section& fsec)
    {
      /*
       * Align the size up to the next alignment boundary and use that as the
       * offset for this object file section.
       */
      uint32_t offset = align_offset (sec.size (), 0, fsec.alignment);

      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "rap:section-merge: " << fsec.name
                  << " sec-size=" << sec.size ()
                  << " relocs=" << fsec.relocs.size ()
                  << " offset=" << offset
                  << " fsec.size=" << fsec.size
                  << " fsec.alignment=" << fsec.alignment
                  << " fsec.rela=" << fsec.rela
                  << " " << obj.obj.name ().full ()  << std::endl;

      /*
       * Add the object file's section offset to the map. This is needed
       * to adjust the external symbol offsets.
       */
      osection osec (fsec.name,
                     offset,
                     fsec.size,
                     fsec.alignment,
                     fsec.relocs.size (),
                     fsec.flags);
      sec.osecs[fsec.index] = osec;
      sec.osindexes.push_back (fsec.index);

      uint32_t rc = 0;

      for (files::relocations::const_iterator fri = fsec.relocs.begin ();
           fri != fsec.relocs.end ();
           ++fri, ++rc)
      {
        const files::relocation& freloc = *fri;
        bool                     merge_reloc = machine_relocation_check (freloc);

        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << " " << std::setw (2) << sec.relocs.size ()
                    << '/' << std::setw (2) << rc << ':'
                    << " merge=" << merge_reloc
                    << std::hex
                    << " reloc.type=" << freloc.type
                    << " reloc.info=0x" << freloc.info
                    << std::dec
                    << " reloc.offset=" << freloc.offset
                    << " reloc.addend=" << freloc.addend
                    << " reloc.symtype=" << freloc.symtype
                    << " reloc.symsect=" << freloc.symsect
                    << " reloc.symbinding=" << freloc.symbinding
                    << std::endl;
        if (merge_reloc)
          sec.relocs.push_back (relocation (freloc, offset));
      }

      std::stable_sort (sec.relocs.begin (),
                        sec.relocs.end (),
                        reloc_symname_compare ());
      std::stable_sort (sec.relocs.begin (),
                        sec.relocs.end (),
                        reloc_offset_compare ());

      if (fsec.rela == true)
        sec.rela = fsec.rela;
    }

    external::external (const uint32_t name,
                        const sections sec,
                        const uint32_t value,
                        const uint32_t data)
      : name (name),
        sec (sec),
        value (value),
        data (data)
    {
    }

    external::external (const external& orig)
      : name (orig.name),
        sec (orig.sec),
        value (orig.value),
        data (orig.data)
    {
    }

    object::object (files::object& obj)
      : obj (obj)
    {
      /*
       * Set up the names of the sections.
       */
      for (int s = 0; s < rap_secs; ++s)
        secs[s].name = section_names[s];

      /*
       * Get the relocation records. Collect the various section types from the
       * object file into the RAP sections. Merge those sections into the RAP
       * sections.
       */

      obj.open ();
      try
      {
        obj.begin ();
        obj.load_relocations ();
        obj.end ();
      }
      catch (...)
      {
        obj.close ();
        throw;
      }
      obj.close ();

      obj.get_sections (text,   SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
      obj.get_sections (const_, SHT_PROGBITS, SHF_ALLOC, SHF_WRITE | SHF_EXECINSTR);
      obj.get_sections (ctor,   ".ctors");
      obj.get_sections (dtor,   ".dtors");
      obj.get_sections (data,   SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
      obj.get_sections (bss,    SHT_NOBITS,   SHF_ALLOC | SHF_WRITE);
      obj.get_sections (symtab, SHT_SYMTAB);
      obj.get_sections (strtab, ".strtab");

      std::for_each (text.begin (), text.end (),
                     section_merge (*this, secs[rap_text]));
      std::for_each (const_.begin (), const_.end (),
                     section_merge (*this, secs[rap_const]));
      std::for_each (ctor.begin (), ctor.end (),
                     section_merge (*this, secs[rap_ctor]));
      std::for_each (dtor.begin (), dtor.end (),
                     section_merge (*this, secs[rap_dtor]));
      std::for_each (data.begin (), data.end (),
                     section_merge (*this, secs[rap_data]));
      std::for_each (bss.begin (), bss.end (),
                     section_merge (*this, secs[rap_bss]));
    }

    object::object (const object& orig)
      : obj (orig.obj),
        text (orig.text),
        const_ (orig.const_),
        ctor (orig.ctor),
        dtor (orig.dtor),
        data (orig.data),
        bss (orig.bss),
        symtab (orig.symtab),
        strtab (orig.strtab)
    {
      for (int s = 0; s < rap_secs; ++s)
        secs[s] = orig.secs[s];
    }

    sections
    object::find (const uint32_t index) const
    {
      const files::section* sec;

      sec = files::find (text, index);
      if (sec)
        return rap_text;

      sec = files::find (const_, index);
      if (sec)
        return rap_const;

      sec = files::find (ctor, index);
      if (sec)
        return rap_ctor;

      sec = files::find (dtor, index);
      if (sec)
        return rap_dtor;

      sec = files::find (data, index);
      if (sec)
        return rap_data;

      sec = files::find (bss, index);
      if (sec)
        return rap_bss;

      throw rld::error ("Section index '" + rld::to_string (index) +
                        "' not found: " + obj.name ().full (), "rap::object");
    }

    uint32_t
    object::get_relocations () const
    {
      uint32_t relocs = 0;
      for (int s = 0; s < rap_secs; ++s)
        relocs += get_relocations (s);
      return relocs;
    }

    uint32_t
    object::get_relocations (int sec) const
    {
      if ((sec < 0) || (sec >= rap_secs))
        throw rld::error ("Invalid section index '" + rld::to_string (sec),
                          "rap::relocations");
      return secs[sec].relocs.size ();
    }

    void
    object::output ()
    {
      std::cout << "rap:object: " << obj.name ().full () << std::endl;
      secs[rap_text].output ();
      secs[rap_const].output ();
      secs[rap_ctor].output ();
      secs[rap_dtor].output ();
      secs[rap_data].output ();
      if (secs[rap_bss].size ())
        std::cout << " bss: size: " << secs[rap_bss].size () << std::endl;
    }

    image::image ()
    {
      clear ();
    }

    void
    image::layout (const files::object_list& app_objects,
                   const std::string&        init,
                   const std::string&        fini)
    {
      clear ();

      /*
       * Create the local objects which contain the layout information.
       */
      for (files::object_list::const_iterator aoi = app_objects.begin ();
           aoi != app_objects.end ();
           ++aoi)
      {
        files::object& app_obj = *(*aoi);

        if (!app_obj.valid ())
          throw rld::error ("Not valid: " + app_obj.name ().full (),
                            "rap::layout");

        objs.push_back (object (app_obj));
      }

      for (objects::iterator oi = objs.begin (), poi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;

        /*
         * Update the offsets in the object file. We need the object's offset
         * to set the relocation offset's correctly as they are relative to the
         * object file.
         */
        if (oi != objs.begin ())
        {
          object& pobj = *poi;
          for (int s = 0; s < rap_secs; ++s)
          {
            obj.secs[s].set_offset (pobj.secs[s]);
            sec_size[s] = obj.secs[s].offset + obj.secs[s].size ();
            sec_align[s] = obj.secs[s].alignment ();
            if (obj.secs[s].rela == true)
              sec_rela[s] = obj.secs[s].rela;
          }
          ++poi;
        }
        else
        {
          for (int s = 0; s < rap_secs; ++s)
          {
            sec_size[s] =  obj.secs[s].size ();
            sec_align[s] = obj.secs[s].alignment ();
            if (obj.secs[s].rela == true)
              sec_rela[s] = true;
          }
        }

        collect_symbols (obj);

        relocs_size += obj.get_relocations ();

        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          obj.output ();
      }

      init_off = strtab.size () + 1;
      strtab += '\0';
      strtab += init;

      fini_off = strtab.size () + 1;
      strtab += '\0';
      strtab += fini;

      if (rld::verbose () >= RLD_VERBOSE_INFO)
      {
        uint32_t total = (sec_size[rap_text] + sec_size[rap_const] +
                          sec_size[rap_data] + sec_size[rap_bss] +
                          symtab_size + strtab.size() + relocs_size);
        std::cout << "rap::layout: total:" << total
                  << " text:" << sec_size[rap_text]
                  << " const:" << sec_size[rap_const]
                  << " ctor:" << sec_size[rap_ctor]
                  << " dtor:" << sec_size[rap_dtor]
                  << " data:" << sec_size[rap_data]
                  << " bss:" << sec_size[rap_bss]
                  << " symbols:" << symtab_size << " (" << externs.size () << ')'
                  << " strings:" << strtab.size () + 1
                  << " relocs:" << relocs_size
                  << std::endl;
      }
    }

    void
    image::collect_symbols (object& obj)
    {
      symbols::pointers& esyms = obj.obj.external_symbols ();
      for (symbols::pointers::const_iterator ei = esyms.begin ();
           ei != esyms.end ();
           ++ei)
      {
        const symbols::symbol& sym = *(*ei);

        if ((sym.type () == STT_OBJECT) || (sym.type () == STT_FUNC) || (sym.type () == STT_NOTYPE))
        {
          if ((sym.binding () == STB_GLOBAL) || (sym.binding () == STB_WEAK))
          {
            int symsec = sym.section_index ();

            /*
             * Do not noting if the symbol is reject at the machine level.
             */
            if (!machine_symbol_check (sym))
              continue;

            sections    rap_sec = obj.find (symsec);
            section&    sec = obj.secs[rap_sec];
            std::size_t name;

            /*
             * See if the name is already in the string table.
             */
            name = find_in_strtab (sym.name ());

            if (name == std::string::npos)
            {
              name = strtab.size () + 1;
              strtab += '\0';
              strtab += sym.name ();
            }

            /*
             * The symbol's value is the symbols value plus the offset of the
             * object file's section offset in the RAP section.
             */
            externs.push_back (external (name,
                                         rap_sec,
                                         sec.offset + sec.osecs[symsec].offset +
                                         sym.value (),
                                         sym.info ()));

            symtab_size += external::rap_size;
          }
        }
      }
    }

    void
    image::write (compress::compressor& comp)
    {
      /*
       * Start with the machine type so the target can check the applicatiion
       * is ok and can be loaded. Add the init and fini labels to the string
       * table and add the references to the string table next. Follow this
       * with the section details then the string table and symbol table then
       * finally the relocation records.
       */

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "rap:output: machine=" << comp.transferred () << std::endl;

      comp << elf::object_machine_type ()
           << elf::object_datatype ()
           << elf::object_class ();

      /*
       * The init and fini label offsets. Then the symbol table and string
       * table sizes.
       */

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "rap:output: header=" << comp.transferred () << std::endl;

      comp << init_off
           << fini_off
           << symtab_size
           << (uint32_t) strtab.size () + 1
           << (uint32_t) 0;

      /*
       * Output file details
       */
      if (add_obj_details)
      {
        write_details (comp);
      }
      else
      {
        comp << (uint32_t)0; /* No file details */
      }

      /*
       * The sections.
       */
      for (int s = 0; s < rap_secs; ++s)
        comp << sec_size[s]
             << sec_align[s];

      /*
       * Output the sections from each object file.
       */
      write (comp, rap_text);
      write (comp, rap_const);
      write (comp, rap_ctor);
      write (comp, rap_dtor);
      write (comp, rap_data);

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "rap:output: strtab=" << comp.transferred () << std::endl;

      strtab += '\0';
      comp << strtab;

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "rap:output: symbols=" << comp.transferred () << std::endl;

      write_externals (comp);

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "rap:output: relocs=" << comp.transferred () << std::endl;

      write_relocations (comp);
    }

    /**
     * Helper for for_each to write out the various sections.
     */
    class section_writer:
      public std::unary_function < object, void >
    {
    public:

      section_writer (image&                img,
                      compress::compressor& comp,
                      sections              sec);

      void operator () (object& obj);

    private:

      image&                img;
      compress::compressor& comp;
      sections              sec;
      uint32_t              offset;
    };

    section_writer::section_writer (image&                img,
                                    compress::compressor& comp,
                                    sections              sec)
      : img (img),
        comp (comp),
        sec (sec),
        offset (0)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "rap:output: " << section_names[sec]
                  << ": offset=" << comp.transferred ()
                  << " size=" << img.section_size (sec) << std::endl;
    }

    void
    section_writer::operator () (object& obj)
    {
      if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
        std::cout << "rap:writing: " << section_names[sec] << std::endl;

      switch (sec)
      {
        case rap_text:
          img.write (comp, obj.obj, obj.text, offset);
          break;
        case rap_const:
          img.write (comp, obj.obj, obj.const_, offset);
          break;
        case rap_ctor:
          img.write (comp, obj.obj, obj.ctor, offset);
          break;
        case rap_dtor:
          img.write (comp, obj.obj, obj.dtor, offset);
          break;
        case rap_data:
          img.write (comp, obj.obj, obj.data, offset);
          break;
        default:
          break;
      }
    }

    void
    image::write (compress::compressor& comp, sections sec)
    {
      uint32_t image_offset = comp.transferred ();

      std::for_each (objs.begin (), objs.end (),
                     section_writer (*this, comp, sec));

      uint32_t written = comp.transferred () - image_offset;

      if (written != sec_size[sec])
      {
        std::string msg = "Image output size does not match layout size: ";
        msg += section_names[sec];
        msg += ": layout-size=" + rld::to_string (sec_size[sec]);
        msg += " image-size=" + rld::to_string (written);
        throw rld::error (msg, "rap::write");
      }
    }

    void
    image::write (compress::compressor&  comp,
                  files::object&         obj,
                  const files::sections& secs,
                  uint32_t&              offset)
    {
      uint32_t size = 0;

      obj.open ();

      try
      {
        obj.begin ();

        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << "rap:write sections: " << obj.name ().full () << std::endl;

        for (files::sections::const_iterator si = secs.begin ();
             si != secs.end ();
             ++si)
        {
          const files::section& sec = *si;
          uint32_t              unaligned_offset = offset + size;

          offset = align_offset (offset, size, sec.alignment);

          if (offset != unaligned_offset)
          {
            char ee = '\xee';
            for (uint32_t p = 0; p < (offset - unaligned_offset); ++p)
              comp.write (&ee, 1);
          }

          comp.write (obj, sec.offset, sec.size);

          if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
            std::cout << " sec: " << sec.index << ' ' << sec.name
                      << " offset=" << offset
                      << " size=" << sec.size
                      << " align=" << sec.alignment
                      << " padding=" << (offset - unaligned_offset)  << std::endl;

          size = sec.size;
        }

        offset += size;

        if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
          std::cout << " total size=" << offset << std::endl;

        obj.end ();
      }
      catch (...)
      {
        obj.close ();
        throw;
      }

      obj.close ();
    }

    void
    image::write_externals (compress::compressor& comp)
    {
      int count = 0;
      for (externals::const_iterator ei = externs.begin ();
           ei != externs.end ();
           ++ei, ++count)
      {
        const external& ext = *ei;

        if (rld::verbose () >= RLD_VERBOSE_TRACE)
          std::cout << "rap:externs: " << count
                    << " name=" << &strtab[ext.name] << " (" << ext.name << ')'
                    << " section=" << section_names[ext.sec]
                    << " data=" << ext.data
                    << " value=0x" << std::hex << ext.value << std::dec
                    << std::endl;

        if ((ext.data & 0xffff0000) != 0)
          throw rld::error ("Data value has data in bits higher than 15",
                            "rap::write-externs");

        comp << (uint32_t) ((ext.sec << 16) | ext.data)
             << ext.name
             << ext.value;
      }
    }

    void
    image::write_relocations (compress::compressor& comp)
    {
      uint32_t rr = 0;

      for (int s = 0; s < rap_secs; ++s)
      {
        uint32_t count = get_relocations (s);
        uint32_t sr = 0;
        uint32_t header;

        if (rld::verbose () >= RLD_VERBOSE_TRACE)
          std::cout << "rap:relocation: section:" << section_names[s]
                    << " relocs=" << count
                    << " rela=" << (char*) (sec_rela[s] ? "yes" : "no")
                    << std::endl;

        header = count;
        header |= sec_rela[s] ? RAP_RELOC_RELA : 0;

        comp << header;

        for (objects::iterator oi = objs.begin ();
             oi != objs.end ();
             ++oi)
        {
          object&      obj = *oi;
          section&     sec = obj.secs[s];
          relocations& relocs = sec.relocs;
          uint32_t     rc = 0;

          if (rld::verbose () >= RLD_VERBOSE_TRACE)
            std::cout << " relocs=" << sec.relocs.size ()
                      << " sec.offset=" << sec.offset
                      << " sec.size=" << sec.size ()
                      << " sec.align=" << sec.alignment ()
                      << "  " << obj.obj.name ().full ()  << std::endl;

          for (relocations::const_iterator ri = relocs.begin ();
               ri != relocs.end ();
               ++ri)
          {
            const relocation& reloc = *ri;
            uint32_t          info = GELF_R_TYPE (reloc.info);
            uint32_t          offset;
            uint32_t          addend = reloc.addend;
            bool              write_addend = sec.rela;
            bool              write_symname = false;

            if (reloc.symsect == 0)
              continue;

            offset = sec.offset + reloc.offset;

            if (rld::verbose () >= RLD_VERBOSE_TRACE)
              std::cout << "  " << std::setw (2) << sr
                        << '/' << std::setw (2) << rc << ':'
                        << std::hex
                        << " reloc.info=0x" << reloc.info
                        << std::dec
                        << " reloc.offset=" << reloc.offset
                        << " reloc.addend=" << reloc.addend
                        << " reloc.symtype=" << reloc.symtype
                        << " reloc.symsect=" << reloc.symsect
                        << " (" << obj.obj.get_section (reloc.symsect).name << ')'
                        << " reloc.symvalue=" << reloc.symvalue
                        << " reloc.symbinding=" << reloc.symbinding
                        << std::endl;

            if ((reloc.symtype == STT_SECTION) || (reloc.symbinding == STB_LOCAL))
            {
              int rap_symsect = obj.find (reloc.symsect);

              /*
               * Bit 31 clear, bits 30:8 RAP section index.
               */
              info |= rap_symsect << 8;

              addend += (obj.secs[rap_symsect].offset +
                         obj.secs[rap_symsect].osecs[reloc.symsect].offset +
                         reloc.symvalue);

              write_addend = true;

              if (rld::verbose () >= RLD_VERBOSE_TRACE)
                std::cout << "  " << std::setw (2) << sr
                          << '/' << std::setw (2) << rc << ':'
                          << " rsym: sect=" << section_names[rap_symsect]
                          << " rap_symsect=" << rap_symsect
                          << " sec.offset=" << obj.secs[rap_symsect].offset
                          << " sec.osecs=" << obj.secs[rap_symsect].osecs[reloc.symsect].offset
                          << " addend=" << addend
                          << std::endl;
            }
            else
            {
              /*
               * Bit 31 must be set. Bit 30 determines the type of string and
               * bits 29:8 the strtab offset or the size of the appended
               * string.
               */

              info |= RAP_RELOC_STRING;

              std::size_t size = find_in_strtab (reloc.symname);

              if (size == std::string::npos)
              {
                /*
                 * Bit 30 clear, the size of the symbol name.
                 */
                info |= reloc.symname.size () << 8;
                write_symname = true;
              }
              else
              {
                /*
                 * Bit 30 set, the offset in the strtab.
                 */
                info |= RAP_RELOC_STRING_EMBED | (size << 8);
              }
            }

            if (rld::verbose () >= RLD_VERBOSE_TRACE)
            {
              std::cout << "  " << std::setw (2) << sr << '/'
                        << std::setw (2) << rc
                        << std::hex << ": reloc: info=0x" << info << std::dec
                        << " offset=" << offset;
              if (write_addend)
                std::cout << " addend=" << addend;
              if ((info & RAP_RELOC_STRING) != 0)
              {
                std::cout << " symname=" << reloc.symname;
                if (write_symname)
                  std::cout << " (appended)";
              }
              std::cout << std::hex
                        << " reloc.info=0x" << reloc.info << std::dec
                        << " reloc.offset=" << reloc.offset
                        << " reloc.symtype=" << reloc.symtype
                        << std::endl;
            }

            comp << info << offset;

            if (write_addend)
              comp << addend;

            if (write_symname)
              comp << reloc.symname;

            ++rc;
            ++sr;
            ++rr;
          }
        }
      }
    }

    void image::write_details (compress::compressor& comp)
    {

      std::string strtable;
      uint32_t pos = 0;

      section_details s_details;


      if (rld::verbose () >= RLD_VERBOSE_TRACE)
      {
        std::cout << "rap:file details" << std::endl
                  << "  total " << objs.size () <<" files" << std::endl;
      }

      comp << (uint32_t)(objs.size ());

      /* rpath for rap file */
      if (rld::verbose () >= RLD_VERBOSE_TRACE)
      {
        std::cout << "rap:file rpath=" << rld::rap::rpath << std::endl;
      }

      comp << (uint32_t)rld::rap::rpath.length ();

      if (rld::rap::rpath.length () > 0)
      {
        strtable += rld::rap::rpath;
      }

      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
          object& obj = *oi;

          /* obj full name */
          strtable += obj.obj.name ().full ();
          strtable += '\0';
      }

      pos = strtable.length ();

      uint32_t sec_num = 0;
      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;

        if (rld::verbose () >= RLD_VERBOSE_TRACE)
          std::cout << "file:" << obj.obj.name ().full () << std::endl;

        for (int s = 0; s < rap_secs; ++s)
        {
          section& sec = obj.secs[s];

          if (rld::verbose () >= RLD_VERBOSE_TRACE)
          {
            std::cout << "rap:section: " << sec.name << " "
                         "offset= " << sec.offset << std::endl;
          }

          for (size_t si = 0; si < sec.osindexes.size (); ++si)
          {
            const osection& osec = sec.get_osection (sec.osindexes[si]);

            strtable += osec.name;
            strtable += '\0';

            /* sec.offset + osec.offset is the offset in the rap section */
            s_details.push_back (section_detail (pos,
                                                 sec.offset + osec.offset,
                                                 s,
                                                 osec.size));

            pos = strtable.length ();

            if (rld::verbose () >= RLD_VERBOSE_TRACE)
            {
              std::cout << "osec.name=" << osec.name << " "
                        << "osec.offset=" << osec.offset << " "
                        << "osec.size=" << osec.size << std::endl;
            }
          }
        }

        /* Output section numbers*/
        comp << (uint32_t)((s_details.size () - sec_num));
        if (rld::verbose () >= RLD_VERBOSE_TRACE)
          std::cout << "sec_num:" << s_details.size () - sec_num  << std::endl;
        sec_num = s_details.size ();
      }

      comp << (uint32_t)(strtable.size ());
      if (rld::verbose () >= RLD_VERBOSE_TRACE)
        std::cout << "total detail size:" << strtable.size () << std::endl;

      comp << strtable;

      for (section_details::const_iterator si = s_details.begin ();
           si != s_details.end ();
           ++si)
      {
        const section_detail& sec_detail = *si;
        comp << (uint32_t)(sec_detail.name);

        if (sec_detail.id > 0xf)
          std::cout << "Out max rap section id 15\n" << std::endl;

        comp << (uint32_t)((sec_detail.id << 28) | sec_detail.offset);
        comp << (uint32_t)(sec_detail.size);
      }
    }

    uint32_t
    image::get_relocations (int sec) const
    {
      if ((sec < 0) || (sec >= rap_secs))
        throw rld::error ("Invalid section index '" + rld::to_string (sec),
                          "rap::image::relocations");

      uint32_t relocs = 0;

      for (objects::const_iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        const object& obj = *oi;
        relocs += obj.get_relocations (sec);
      }

      return relocs;
    }

    void
    image::clear ()
    {
      for (int s = 0; s < rap_secs; ++s)
      {
        sec_size[s] = 0;
        sec_align[s] = 0;
        sec_rela[s] = false;
      }
      symtab_size = 0;
      strtab.clear ();
      relocs_size = 0;
      init_off = 0;
      fini_off = 0;
    }

    uint32_t
    image::section_size (sections sec) const
    {
      if ((sec < 0) || (sec >= rap_secs))
        throw rld::error ("Invalid section index '" + rld::to_string (sec),
                          "rap::image::section_size");
      return sec_size[sec];
    }

    std::size_t
    image::find_in_strtab (const std::string& symname)
    {
      std::size_t pos = 0;
      while (pos < strtab.size ())
      {
        std::size_t off = strtab.find (symname, pos);
        if (off == std::string::npos)
          break;
        if (::strlen (strtab.c_str () + off) == symname.size ())
          return off;
        pos = off + 1;
      }
      return std::string::npos;
    }

    void
    write (files::image&             app,
           const std::string&        init,
           const std::string&        fini,
           const files::object_list& app_objects,
           const symbols::table&     /* symbols */) /* Add back for incremental
                                                     * linking */
    {
      std::string header;

      header = "RAP,00000000,0002,LZ77,00000000\n";
      app.write (header.c_str (), header.size ());

      compress::compressor compressor (app, 2 * 1024);
      image                rap;

      rap.layout (app_objects, init, fini);
      rap.write (compressor);

      compressor.flush ();

      std::ostringstream length;

      length << std::setfill ('0') << std::setw (8)
             << header.size () + compressor.compressed ();

      header.replace (4, 8, length.str ());

      app.seek (0);
      app.write (header.c_str (), header.size ());

      if (rld::verbose () >= RLD_VERBOSE_INFO)
      {
        int pcent = (compressor.compressed () * 100) / compressor.transferred ();
        int premand = (((compressor.compressed () * 1000) + 500) /
                       compressor.transferred ()) % 10;
        std::cout << "rap: objects: " << app_objects.size ()
                  << ", size: " << compressor.compressed ()
                  << ", expanded: " << compressor.transferred ()
                  << ", compressed: " << pcent << '.' << premand << '%'
                  << std::endl;
      }
    }

  }
}
