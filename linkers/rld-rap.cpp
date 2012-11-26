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
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

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
     * The specific data for each object we need to collect to create the RAP
     * format file.
     */
    struct object
    {
      files::object&  obj;          //< The object file.
      files::sections text;         //< All execitable code.
      files::sections const_;       //< All read only data.
      files::sections ctor;         //< The static constructor table.
      files::sections dtor;         //< The static destructor table.
      files::sections data;         //< All initialised read/write data.
      files::sections bss;          //< All uninitialised read/write data
      files::sections relocs;       //< All relocation records.
      files::sections symtab;       //< All exported symbols.
      files::sections strtab;       //< All exported strings.
      uint32_t        text_off;     //< The text section file offset.
      uint32_t        text_size;    //< The text section size.
      uint32_t        const_off;    //< The const section file offset.
      uint32_t        const_size;   //< The const section size.
      uint32_t        ctor_off;     //< The ctor section file offset.
      uint32_t        ctor_size;    //< The ctor section size.
      uint32_t        dtor_off;     //< The dtor section file offset.
      uint32_t        dtor_size;    //< The dtor section size.
      uint32_t        data_off;     //< The data section file offset.
      uint32_t        data_size;    //< The data section size.
      uint32_t        bss_size;     //< The bss section size.
      uint32_t        symtab_off;   //< The symbols sectuint32 fioffset.
      uint32_t        symtab_size;  //< The symbol section size.
      uint32_t        strtab_off;   //< The string section file offset.
      uint32_t        strtab_size;  //< The string section size.
      uint32_t        relocs_off;   //< The reloc section file offset.
      uint32_t        relocs_size;  //< The reloc section size.

      /**
       * The constructor. Need to have an object file.
       */
      object (files::object& obj);

      /**
       * The copy constructor.
       */
      object (const object& orig);

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
       * The text section.
       */
      typedef std::vector < uint8_t > bytes;

      /**
       * Construct the image.
       */
      image ();

      /**
       * Load the layout data from the object files.
       */
      void layout (const files::object_list& app_objects);

      /**
       * Write the compressed output file.
       */
      void write (compress::compressor& comp, const std::string& metadata);

      /**
       * Write the sections to the compressed output file.
       */
      void write (compress::compressor&  comp,
                  files::object&         obj,
                  const files::sections& secs);

    private:

      objects  objs;        //< The RAP objects
      uint32_t text_size;   //< The text size.
      uint32_t data_size;   //< The data size.
      uint32_t bss_size;    //< The size of the .bss region of the image.
      uint32_t symtab_size; //< The symbols size.
      uint32_t strtab_size; //< The strings size.
      uint32_t relocs_size; //< The relocations size.
    };

    void
    output (const char* name, size_t size, const files::sections& secs)
    {
      if (size)
      {
        std::cout << ' ' << name << ": size: " << size << std::endl;

        for (files::sections::const_iterator si = secs.begin ();
             si != secs.end ();
             ++si)
        {
          files::section sec = *si;

          if (sec.size)
          {
            #define SF(f, i, c) if (sec.flags & (f)) flags[i] = c

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
                      << std::setw (15) << sec.name
                      << " " << flags
                      << " size: " << std::setw (5) << sec.size
                      << " align: " << sec.alignment
                      << std::right << std::endl;
          }
        }
      }
    }

    object::object (files::object& obj)
      : obj (obj),
        text_off (0),
        text_size (0),
        const_off (0),
        const_size (0),
        ctor_off (0),
        ctor_size (0),
        dtor_off (0),
        dtor_size (0),
        data_off (0),
        data_size (0),
        bss_size (0),
        symtab_off (0),
        symtab_size (0),
        strtab_off (0),
        strtab_size (0),
        relocs_off (0),
        relocs_size (0)
    {
      /*
       * Get from the object file the various sections we need to format a
       * memory layout.
       */

      obj.get_sections (text,   SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
      obj.get_sections (const_, SHT_PROGBITS, SHF_ALLOC | SHF_MERGE, SHF_WRITE | SHF_EXECINSTR);
      obj.get_sections (ctor,   ".ctors");
      obj.get_sections (dtor,   ".dtors");
      obj.get_sections (data,   SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
      obj.get_sections (bss,    SHT_NOBITS,   SHF_ALLOC | SHF_WRITE);
      obj.get_sections (symtab, SHT_SYMTAB);
      obj.get_sections (strtab, ".strtab");

      /*
       * Only interested in the relocation records for the text sections.
       */
      for (files::sections::const_iterator ti = text.begin ();
           ti != text.end ();
           ++ti)
      {
        files::section sec = *ti;
        obj.get_sections (relocs, ".rel" + sec.name);
        obj.get_sections (relocs, ".rela" + sec.name);
      }

      text_size = files::sum_sizes (text);
      const_size = files::sum_sizes (const_);
      ctor_size = files::sum_sizes (ctor);
      dtor_size = files::sum_sizes (dtor);
      data_size = files::sum_sizes (data);
      symtab_size = files::sum_sizes (symtab);
      strtab_size = files::sum_sizes (strtab);
      relocs_size = files::sum_sizes (relocs);

      if (rld::verbose () >= RLD_VERBOSE_TRACE)
      {
        std::cout << "rap:object: " << obj.name ().full () << std::endl;
        output ("text", text_size, text);
        output ("const", const_size, const_);
        output ("ctor", ctor_size, ctor);
        output ("dtor", dtor_size, dtor);
        output ("data", data_size, data);
        if (bss_size)
          std::cout << bss_size << std::endl;
        output ("relocs", relocs_size, relocs);
        output ("symtab", symtab_size, symtab);
        output ("strtab", strtab_size, strtab);
      }
    }

    object::object (const object& orig)
      : obj (orig.obj),
        text (orig.text),
        const_ (orig.const_),
        ctor (orig.ctor),
        dtor (orig.dtor),
        data (orig.data),
        bss (orig.bss),
        relocs (orig.relocs),
        symtab (orig.symtab),
        strtab (orig.strtab),
        text_off (orig.text_off),
        text_size (orig.text_size),
        const_off (orig.const_off),
        const_size (orig.const_size),
        ctor_off (orig.ctor_off),
        ctor_size (orig.ctor_size),
        dtor_off (orig.dtor_off),
        dtor_size (orig.dtor_size),
        data_off (orig.data_off),
        data_size (orig.data_size),
        bss_size (orig.bss_size),
        symtab_off (orig.symtab_off),
        symtab_size (orig.symtab_size),
        strtab_off (orig.strtab_off),
        strtab_size (orig.strtab_size),
        relocs_off (orig.relocs_off),
        relocs_size (orig.relocs_size)
    {
    }

    image::image ()
      : text_size (0),
        data_size (0),
        bss_size (0),
        symtab_size (0),
        strtab_size (0),
        relocs_size (0)
    {
    }

    void
    image::layout (const files::object_list& app_objects)
    {
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
                            "rap::load-sections");

        objs.push_back (object (app_obj));
      }

      text_size = 0;
      data_size = 0;
      bss_size = 0;

      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;
        obj.text_off = text_size;
        text_size += obj.text_size;
        obj.data_off = data_size;
        data_size += obj.data_size;
        bss_size += obj.bss_size;
        symtab_size += obj.symtab_size;
        strtab_size += obj.strtab_size;
        relocs_size += obj.relocs_size;
      }

      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;
        obj.const_off = text_size;
        text_size += obj.const_size;
      }

      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;
        obj.ctor_off = text_size;
        text_size += obj.ctor_size;
      }

      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;
        obj.dtor_off = text_size;
        text_size += obj.dtor_size;
      }

      if (rld::verbose () >= RLD_VERBOSE_INFO)
      {
        uint32_t total = (text_size + data_size + data_size + bss_size +
                          symtab_size + strtab_size + relocs_size);
        std::cout << "rap:image:layout: total:" << total
                  << " text:" << text_size
                  << " data:" << data_size
                  << " bss:" << bss_size
                  << " symbols:" << symtab_size
                  << " strings:" << strtab_size
                  << " relocs:" << relocs_size
                  << std::endl;
      }
    }

    void
    image::write (compress::compressor& comp, const std::string& metadata)
    {
      /*
       * Start with the number of object files to load.
       */
      comp << metadata.size ()
           << objs.size ()
           << text_size
           << data_size
           << bss_size;

      for (objects::iterator oi = objs.begin ();
           oi != objs.end ();
           ++oi)
      {
        object& obj = *oi;

        comp << obj.text_size
             << obj.ctor_size
             << obj.dtor_size
             << obj.data_size
             << obj.symtab_size
             << obj.strtab_size
             << obj.relocs_size;

        obj.obj.open ();

        try
        {
          obj.obj.begin ();

          write (comp, obj.obj, obj.text);
          write (comp, obj.obj, obj.const_);
          write (comp, obj.obj, obj.ctor);
          write (comp, obj.obj, obj.dtor);
          write (comp, obj.obj, obj.data);
          write (comp, obj.obj, obj.symtab);
          write (comp, obj.obj, obj.strtab);

          obj.obj.end ();
        }
        catch (...)
        {
          obj.obj.close ();
          throw;
        }

        obj.obj.close ();
      }
    }

    void
    image::write (compress::compressor&  comp,
                  files::object&         obj,
                  const files::sections& secs)
    {
      for (files::sections::const_iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        const files::section& sec = *si;
        comp.write (obj, sec.offset, sec.size);
      }
    }

    void
    write (files::image&             app,
           const std::string&        metadata,
           const files::object_list& app_objects,
           const symbols::table&     /* symbols */) /* Add back for incremental
                                                     * linking */
    {
      compress::compressor compressor (app, 128 * 1024);
      image                rap;

      rap.layout (app_objects);
      rap.write (compressor, metadata);

      compressor.flush ();

      if (rld::verbose () >= RLD_VERBOSE_INFO)
      {
        int pcent = (compressor.compressed () * 100) / compressor.transferred ();
        int premand = (((compressor.compressed () * 1000) + 500) /
                       compressor.transferred ()) % 10;
        std::cout << "rap: objects: " << app_objects.size ()
                  << ", size: " << compressor.compressed ()
                  << ", compression: " << pcent << '.' << premand << '%'
                  << std::endl;
      }
    }

  }
}
