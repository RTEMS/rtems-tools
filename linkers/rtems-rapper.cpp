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
 * @ingroup rtems_rld
 *
 * @brief RTEMS RAP Manager lets you look at and play with RAP files.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include <cxxabi.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include <rld.h>
#include <rld-compression.h>
#include <rld-files.h>
#include <rld-process.h>
#include <rld-rap.h>
#include <rld-rtems.h>

#include <rtems-utils.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

/**
 * RTEMS Application.
 */
namespace rap
{
  /**
   * The names of the RAP sections.
   */
  static const char* section_names[6] =
  {
    ".text",
    ".const",
    ".ctor",
    ".dtor",
    ".data",
    ".bss"
  };

  /**
   * A relocation record.
   */
  struct relocation
  {
    uint32_t    info;
    uint32_t    offset;
    uint32_t    addend;
    std::string symname;
    off_t       rap_off;

    relocation ();

    void output ();
  };

  typedef std::vector < relocation > relocations;

  /**
   * Relocation offset sorter for the relocations container.
   */
  class reloc_offset_compare
  {
  public:
    bool operator () (const relocation& lhs,
                      const relocation& rhs) const {
      return lhs.offset < rhs.offset;
    }
  };

  /**
   * A RAP section.
   */
  struct section
  {
    std::string name;
    uint32_t    size;
    uint32_t    alignment;
    uint8_t*    data;
    uint32_t    relocs_size;
    relocations relocs;
    bool        rela;
    off_t       rap_off;

    section ();
    ~section ();

    void load_data (rld::compress::compressor& comp);
    void load_relocs (rld::compress::compressor& comp);
  };

  /**
   * Section detail
   */
  struct section_detail
  {
    uint32_t name;   //< The offset in the strtable.
    uint32_t offset; //< The offset in the rap section.
    uint32_t id;     //< The rap id.
    uint32_t size;   //< The size of the elf section.
    uint32_t obj;    //< The obj id.

    /* Constructor */
    section_detail (const section_detail& s);
    section_detail ();
  };

  section_detail::section_detail (const section_detail& s)
    : name (s.name),
      offset (s.offset),
      id (s.id),
      size (s.size),
      obj (s.obj)
  {
  }

  section_detail::section_detail ()
    : name (0),
      offset (0),
      id (0),
      size (0),
      obj (0)
  {
  }

  /**
   * A container of section_detail
   */
  typedef std::list < section_detail > section_details;

  /**
   * A RAP file.
   */
  struct file
  {
    enum {
      rap_comp_buffer = 2 * 1024
    };

    std::string header;
    size_t      rhdr_len;
    uint32_t    rhdr_length;
    uint32_t    rhdr_version;
    std::string rhdr_compression;
    uint32_t    rhdr_checksum;

    off_t       machine_rap_off;
    uint32_t    machinetype;
    uint32_t    datatype;
    uint32_t    class_;

    off_t       layout_rap_off;
    std::string init;
    uint32_t    init_off;
    std::string fini;
    uint32_t    fini_off;

    off_t       strtab_rap_off;
    uint32_t    strtab_size;
    uint8_t*    strtab;

    off_t       symtab_rap_off;
    uint32_t    symtab_size;
    uint8_t*    symtab;

    off_t       relocs_rap_off;
    uint32_t    relocs_size; /* not used */

    off_t       detail_rap_off;
    uint32_t    obj_num;
    uint8_t**   obj_name;
    uint32_t*   sec_num;
    uint8_t*    rpath;
    uint32_t    rpathlen;
    uint8_t*    str_detail;
    section_details sec_details;

    section     secs[rld::rap::rap_secs];

    /**
     * Open a RAP file and read the header.
     */
    file (const std::string& name, bool warnings);

    /**
     * Close the RAP file.
     */
    ~file ();

    /**
     * Parse header.
     */
    void parse_header ();

    /**
     * Load the file.
     */
    void load ();

    /**
     * Expand the image.
     */
    void expand ();

    /**
     * Load details.
     */
    void load_details(rld::compress::compressor& comp);

    /**
     * The name.
     */
    const std::string name () const;

    /**
     * The number of symbols in the symbol table.
     */
    int symbols () const;

    /**
     * Return a symbol given an index.
     */
    void symbol (int       index,
                 uint32_t& data,
                 uint32_t& name,
                 uint32_t& value) const;

    /**
     * Return the string from the string table.
     */
    const char* string (int index);

  private:

    bool              warnings;
    rld::files::image image;
  };

  template < typename T > T
  get_value (const uint8_t* data)
  {
    T v = 0;
    for (size_t b = 0; b < sizeof (T); ++b)
    {
      v <<= 8;
      v |= (T) data[b];
    }
    return v;
  }

  relocation::relocation ()
    : info (0),
      offset (0),
      addend (0),
      rap_off (0)
  {
  }

  void
  relocation::output ()
  {
    std::cout << std::hex << std::setfill ('0')
              << "0x" << std::setw (8) << info
              << " 0x" << std::setw (8) << offset
              << " 0x" << std::setw(8) << addend
              << std::dec << std::setfill (' ')
              << " " << symname;
  }

  section::section ()
    : size (0),
      alignment (0),
      data (0),
      relocs_size (0),
      relocs (0),
      rela (false),
      rap_off (0)
  {
  }

  section::~section ()
  {
    if (data)
      delete [] data;
  }

  void
  section::load_data (rld::compress::compressor& comp)
  {
    rap_off = comp.offset ();
    if (size)
    {
      data = new uint8_t[size];
      if (comp.read (data, size) != size)
        throw rld::error ("Reading section data failed", "rapper");
    }
  }

  void
  section::load_relocs (rld::compress::compressor& comp)
  {
    uint32_t header;
    comp >> header;

    rela = header & RAP_RELOC_RELA ? true : false;
    relocs_size = header & ~RAP_RELOC_RELA;

    if (relocs_size)
    {
      for (uint32_t r = 0; r < relocs_size; ++r)
      {
        relocation reloc;

        reloc.rap_off = comp.offset ();

        comp >> reloc.info
             >> reloc.offset;

        if (((reloc.info & RAP_RELOC_STRING) == 0) || rela)
          comp >> reloc.addend;

        if ((reloc.info & RAP_RELOC_STRING) != 0)
        {
          if ((reloc.info & RAP_RELOC_STRING_EMBED) == 0)
          {
            size_t symname_size = (reloc.info & ~(3 << 30)) >> 8;
            reloc.symname.resize (symname_size);
            size_t symname_read = comp.read ((void*) reloc.symname.c_str (), symname_size);
            if (symname_read != symname_size)
              throw rld::error ("Reading reloc symbol name failed", "rapper");
          }
        }

        relocs.push_back (reloc);
      }

      std::stable_sort (relocs.begin (), relocs.end (), reloc_offset_compare ());
    }
  }

  file::file (const std::string& name, bool warnings)
    : rhdr_len (0),
      rhdr_length (0),
      rhdr_version (0),
      rhdr_checksum (0),
      machine_rap_off (0),
      machinetype (0),
      datatype (0),
      class_ (0),
      layout_rap_off (0),
      init_off (0),
      fini_off (0),
      strtab_rap_off (0),
      strtab_size (0),
      strtab (0),
      symtab_rap_off (0),
      symtab_size (0),
      symtab (0),
      relocs_rap_off (0),
      relocs_size (0),
      detail_rap_off (0),
      obj_num (0),
      obj_name (0),
      sec_num (0),
      rpath (0),
      rpathlen (0),
      str_detail (0),
      warnings (warnings),
      image (name)
  {
    for (int s = 0; s < rld::rap::rap_secs; ++s)
      secs[s].name = rld::rap::section_name (s);
    image.open ();
    parse_header ();
  }

  file::~file ()
  {
    image.close ();

    if (symtab)
      delete [] symtab;
    if (strtab)
      delete [] strtab;
    if (obj_name)
      delete [] obj_name;
    if (sec_num)
      delete [] sec_num;
    if (str_detail)
      delete [] str_detail;

  }

  void
  file::parse_header ()
  {
    std::string name = image.name ().full ();

    char rhdr[64];

    image.seek_read (0, (uint8_t*) rhdr, 64);

    if ((rhdr[0] != 'R') || (rhdr[1] != 'A') || (rhdr[2] != 'P') || (rhdr[3] != ','))
      throw rld::error ("Invalid RAP file", "open: " + name);

    char* sptr = rhdr + 4;
    char* eptr;

    rhdr_length = ::strtoul (sptr, &eptr, 10);

    if (*eptr != ',')
      throw rld::error ("Cannot parse RAP header", "open: " + name);

    sptr = eptr + 1;

    rhdr_version = ::strtoul (sptr, &eptr, 10);

    if (*eptr != ',')
      throw rld::error ("Cannot parse RAP header", "open: " + name);

    sptr = eptr + 1;

    if ((sptr[0] == 'N') &&
        (sptr[1] == 'O') &&
        (sptr[2] == 'N') &&
        (sptr[3] == 'E'))
    {
      rhdr_compression = "NONE";
      eptr = sptr + 4;
    }
    else if ((sptr[0] == 'L') &&
             (sptr[1] == 'Z') &&
             (sptr[2] == '7') &&
             (sptr[3] == '7'))
    {
      rhdr_compression = "LZ77";
      eptr = sptr + 4;
    }
    else
      throw rld::error ("Cannot parse RAP header", "open: " + name);

    if (*eptr != ',')
      throw rld::error ("Cannot parse RAP header", "open: " + name);

    sptr = eptr + 1;

    rhdr_checksum = ::strtoul (sptr, &eptr, 16);

    if (*eptr != '\n')
      throw rld::error ("Cannot parse RAP header", "open: " + name);

    rhdr_len = eptr - rhdr + 1;

    if (warnings && (rhdr_length != image.size ()))
      std::cout << " warning: header length does not match file size: header="
                << rhdr_length
                << " file-size=" << image.size ()
                << std::endl;

    header.insert (0, rhdr, rhdr_len);

    image.seek (rhdr_len);
  }

  void
  file::load_details (rld::compress::compressor& comp)
  {
    uint32_t tmp;

    comp >> rpathlen;

    obj_name = new uint8_t*[obj_num];

    sec_num = new uint32_t[obj_num];

    /* how many sections of each object file */
    for (uint32_t i = 0; i < obj_num; i++)
    {
      comp >> tmp;
      sec_num[i] = tmp;
    }

    /* strtable size */
    comp >> tmp;
    str_detail = new uint8_t[tmp];
    if (comp.read (str_detail, tmp) != tmp)
      throw rld::error ("Reading file str details error", "rapper");

    if (rpathlen > 0)
      rpath = (uint8_t*)str_detail;
    else rpath = NULL;

    section_detail sec;

    for (uint32_t i = 0; i < obj_num; i++)
    {
      sec.obj = i;
      for (uint32_t j = 0; j < sec_num[i]; j++)
      {
        comp >> sec.name;
        comp >> tmp;
        sec.offset = tmp & 0xfffffff;
        sec.id = tmp >> 28;
        comp >> sec.size;

        sec_details.push_back (section_detail (sec));
      }
    }
  }
  void
  file::load ()
  {
    image.seek (rhdr_len);

    rld::compress::compressor comp (image, rap_comp_buffer, false);

    /*
     * uint32_t: machinetype
     * uint32_t: datatype
     * uint32_t: class
     */
    machine_rap_off = comp.offset ();
    comp >> machinetype
         >> datatype
         >> class_;

    /*
     * uint32_t: init
     * uint32_t: fini
     * uint32_t: symtab_size
     * uint32_t: strtab_size
     * uint32_t: relocs_size
     */
    layout_rap_off = comp.offset ();
    comp >> init_off
         >> fini_off
         >> symtab_size
         >> strtab_size
         >> relocs_size;

    /*
     * Load the file details.
     */
    detail_rap_off = comp.offset ();

    comp >> obj_num;

    if (obj_num > 0)
      load_details(comp);

    /*
     * uint32_t: text_size
     * uint32_t: text_alignment
     * uint32_t: const_size
     * uint32_t: const_alignment
     * uint32_t: ctor_size
     * uint32_t: ctor_alignment
     * uint32_t: dtor_size
     * uint32_t: dtor_alignment
     * uint32_t: data_size
     * uint32_t: data_alignment
     * uint32_t: bss_size
     * uint32_t: bss_alignment
     */
    for (int s = 0; s < rld::rap::rap_secs; ++s)
      comp >> secs[s].size
           >> secs[s].alignment;

    /*
     * Load sections.
     */
    for (int s = 0; s < rld::rap::rap_secs; ++s)
      if (s != rld::rap::rap_bss)
        secs[s].load_data (comp);

    /*
     * Load the string table.
     */
    strtab_rap_off = comp.offset ();
    if (strtab_size)
    {
      strtab = new uint8_t[strtab_size];
      if (comp.read (strtab, strtab_size) != strtab_size)
        throw rld::error ("Reading string table failed", "rapper");
    }

    /*
     * Load the symbol table.
     */
    symtab_rap_off = comp.offset ();
    if (symtab_size)
    {
      symtab = new uint8_t[symtab_size];
      if (comp.read (symtab, symtab_size) != symtab_size)
        throw rld::error ("Reading symbol table failed", "rapper");
    }

    /*
     * Load the relocation tables.
     */
    relocs_rap_off = comp.offset ();
    for (int s = 0; s < rld::rap::rap_secs; ++s)
      secs[s].load_relocs (comp);
  }

  void
  file::expand ()
  {
    std::string name = image.name ().full ();
    std::string extension = rld::path::extension (image.name ().full ());

    name = name.substr (0, name.size () - extension.size ()) + ".xrap";

    image.seek (rhdr_len);

    rld::compress::compressor comp (image, rap_comp_buffer, false);
    rld::files::image         out (name);

    out.open (true);
    out.seek (0);
    while (true)
    {
      if (comp.read (out, rap_comp_buffer) != rap_comp_buffer)
        break;
    }
    out.close ();
  }

  const std::string
  file::name () const
  {
    return image.name ().full ();
  }

  int
  file::symbols () const
  {
    return symtab_size / (3 * sizeof (uint32_t));
  }

  void
  file::symbol (int index, uint32_t& data, uint32_t& name, uint32_t& value) const
  {
    if (index < symbols ())
    {
      uint8_t* sym = symtab + (index * 3 * sizeof (uint32_t));
      data  = get_value < uint32_t > (sym);
      name  = get_value < uint32_t > (sym + (1 * sizeof (uint32_t)));
      value = get_value < uint32_t > (symtab + (2 * sizeof (uint32_t)));
    }
  }

  const char*
  file::string (int index)
  {
    std::string name = image.name ().full ();

    if (strtab_size == 0)
      throw rld::error ("No string table", "string: " + name);

    uint32_t offset = 0;
    int count = 0;
    while (offset < strtab_size)
    {
      if (count++ == index)
        return (const char*) &strtab[offset];
      offset = ::strlen ((const char*) &strtab[offset]) + 1;
    }

    throw rld::error ("Invalid string index", "string: " + name);
  }
}

void
rap_show (rld::path::paths& raps,
          bool              warnings,
          bool              show_header,
          bool              show_machine,
          bool              show_layout,
          bool              show_strings,
          bool              show_symbols,
          bool              show_relocs,
          bool              show_details)
{
  for (rld::path::paths::iterator pi = raps.begin();
       pi != raps.end();
       ++pi)
  {
    std::cout  << *pi << ':' << std::endl;

    rap::file r (*pi, warnings);

    try
    {
      r.load ();
    }
    catch (rld::error re)
    {
      std::cout << " error: "
                << re.where << ": " << re.what
                << std::endl
                << " warning: file read failed, some data may be corrupt or not present."
                << std::endl;
    }

    if (show_header)
    {
      std::cout << "  Header:" << std::endl
                << "          string: " << r.header
                << "          length: " << r.rhdr_len << std::endl
                << "         version: " << r.rhdr_version << std::endl
                << "     compression: " << r.rhdr_compression << std::endl
                << std::hex << std::setfill ('0')
                << "        checksum: " << std::setw (8) << r.rhdr_checksum << std::endl
                << std::dec << std::setfill(' ');
    }

    if (show_machine)
    {
      std::cout << "  Machine: 0x"
                << std::hex << std::setfill ('0')
                << std::setw (8) << r.machine_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.machine_rap_off << ')' << std::endl
                << "     machinetype: "<< r.machinetype << std::endl
                << "        datatype: "<< r.datatype << std::endl
                << "           class: "<< r.class_ << std::endl;
    }

    if (show_layout)
    {
      std::cout << "  Layout: 0x"
                << std::hex << std::setfill ('0')
                << std::setw (8) << r.layout_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.layout_rap_off << ')' << std::endl
                << std::setw (18) << "  "
                << "  size  align offset    " << std::endl;
      uint32_t relocs_size = 0;
      for (int s = 0; s < rld::rap::rap_secs; ++s)
      {
        relocs_size += r.secs[s].relocs.size ();
        std::cout << std::setw (16) << rld::rap::section_name (s)
                  << ": " << std::setw (6) << r.secs[s].size
                  << std::setw (7)  << r.secs[s].alignment;
        if (s != rld::rap::rap_bss)
          std::cout << std::hex << std::setfill ('0')
                    << " 0x" << std::setw (8) << r.secs[s].rap_off
                    << std::setfill (' ') << std::dec
                    << " (" << r.secs[s].rap_off << ')';
        else
          std::cout << " -";
        std::cout << std::endl;
      }
      std::cout << std::setw (16) << "strtab" << ": "
                << std::setw (6) << r.strtab_size
                << std::setw (7) << '-'
                << std::hex << std::setfill ('0')
                << " 0x" << std::setw (8) << r.strtab_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.strtab_rap_off << ')' << std::endl
                << std::setw (16) << "symtab" << ": "
                << std::setw (6) << r.symtab_size
                << std::setw (7) << '-'
                << std::hex << std::setfill ('0')
                << " 0x" << std::setw (8) << r.symtab_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.symtab_rap_off << ')' << std::endl
                << std::setw (16) << "relocs" << ": "
                << std::setw (6) << (relocs_size * 3 * sizeof (uint32_t))
                << std::setw (7) << '-'
                << std::hex << std::setfill ('0')
                << " 0x" << std::setw (8) << r.relocs_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.relocs_rap_off << ')' << std::endl;
    }

    if (show_details)
    {
      std::cout << " Details: 0x"
                << std::hex << std::setfill ('0')
                << std::setw (8) << r.detail_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.detail_rap_off << ')' << std::endl;

      uint32_t pos = 0;
      if (r.rpath != NULL)
      {
        std::cout << " rpath:" << std::endl;
        while (pos < r.rpathlen)
        {
          std::cout << " " << r.rpath + pos << std::endl;
          pos = std::string ((char*)(r.rpath + pos)).length () + pos + 1;
        }
      }

      if (r.obj_num == 0)
        std::cout << " No details" << std::endl;
      else
        std::cout << ' ' << r.obj_num <<" Files" << std::endl;

      for (uint32_t i = 0; i < r.obj_num; ++i)
      {
        r.obj_name[i] = (uint8_t*) &r.str_detail[pos];
        pos += ::strlen ((char*) &r.str_detail[pos]) + 1;
      }

      for (uint32_t i = 0; i < r.obj_num; ++i)
      {
        std::cout << " File: " << r.obj_name[i] << std::endl;

        for (rap::section_details::const_iterator sd = r.sec_details.begin ();
             sd != r.sec_details.end ();
             ++sd)
        {
          rap::section_detail tmp = *sd;
          if (tmp.obj == i)
          {
            std::cout << std::setw (12) << "name:"
                      << std::setw (16) << (char*)&r.str_detail[tmp.name]
                      << " rap_section:"<< std::setw (8)
                      << rap::section_names[tmp.id]
                      << std::hex << std::setfill ('0')
                      << " offset:0x" << std::setw (8) << tmp.offset
                      << " size:0x" << std::setw (8) << tmp.size << std::dec
                      << std::setfill (' ') << std::endl;

          }
        }
      }
    }

    if (show_strings)
    {
      std::cout << "  Strings: 0x"
                << std::hex << std::setfill ('0')
                << std::setw (8) << r.strtab_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.strtab_rap_off << ')'
                << " size: " << r.strtab_size
                << std::endl;
      if (r.strtab_size)
      {
        uint32_t offset = 0;
        int count = 0;
        while (offset < r.strtab_size)
        {
          std::cout << std::setw (16) << count++
                    << std::hex << std::setfill ('0')
                    << " (0x" << std::setw (6) << offset << "): "
                    << std::dec << std::setfill (' ')
                    << (char*) &r.strtab[offset] << std::endl;
          offset += ::strlen ((char*) &r.strtab[offset]) + 1;
        }
      }
      else
      {
        std::cout << std::setw (16) << " "
                  << "No string table found." << std::endl;
      }
    }

    if (show_symbols)
    {
      std::cout << "  Symbols: 0x"
                << std::hex << std::setfill ('0')
                << std::setw (8) << r.symtab_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.symtab_rap_off << ')'
                << " size: " << r.symtab_size
                << std::endl;
      if (r.symtab_size)
      {
        std::cout << std::setw (18) << "  "
                  << "  data section  value      name" << std::endl;
        for (int s = 0; s < r.symbols (); ++s)
        {
          uint32_t data;
          uint32_t name;
          uint32_t value;
          r.symbol (s, data, name, value);
          std::cout << std::setw (16) << s << ": "
                    << std::hex << std::setfill ('0')
                    << "0x" << std::setw (4) << (data & 0xffff)
                    << std::dec << std::setfill (' ')
                    << " " << std::setw (8) << rld::rap::section_name (data >> 16)
                    << std::hex << std::setfill ('0')
                    << " 0x" << std::setw(8) << value
                    << " " << &r.strtab[name]
                    << std::dec << std::setfill (' ')
                    << std::endl;
        }
      }
      else
      {
        std::cout << std::setw (16) << " "
                  << "No symbol table found." << std::endl;
      }
    }

    if (show_relocs)
    {
      std::cout << "  Relocations: 0x"
                << std::hex << std::setfill ('0')
                << std::setw (8) << r.relocs_rap_off
                << std::setfill (' ') << std::dec
                << " (" << r.relocs_rap_off << ')' << std::endl;
      int count = 0;
      for (int s = 0; s < rld::rap::rap_secs; ++s)
      {
        if (r.secs[s].relocs.size ())
        {
          const char* rela = r.secs[s].rela ? "(A)" : "   ";
          std::cout << std::setw (16) << r.secs[s].name
                    << ": info       offset     addend "
                    << rela
                    << " symbol name" << std::endl;
          for (size_t f = 0; f < r.secs[s].relocs.size (); ++f)
          {
            rap::relocation& reloc = r.secs[s].relocs[f];
            std::cout << std::setw (16) << count++ << ": ";
            reloc.output ();
            std::cout << std::endl;
          }
        }
      }
    }
  }
}

void
rap_overlay (rld::path::paths& raps, bool warnings)
{
  std::cout << "Overlay .... " << std::endl;
  for (rld::path::paths::iterator pi = raps.begin();
       pi != raps.end();
       ++pi)
  {
    rap::file r (*pi, warnings);
    std::cout << r.name () << std::endl;

    r.load ();

    for (int s = 0; s < rld::rap::rap_secs; ++s)
    {
      rap::section& sec = r.secs[s];

      if (sec.size && sec.data)
      {
        std::cout << rld::rap::section_name (s) << ':' << std::endl;

        size_t   line_length = 16;
        uint32_t offset = 0;
        size_t   reloc = 0;

        while (offset < sec.size)
        {
          size_t length = sec.size - offset;

          if (reloc < sec.relocs.size ())
            length = sec.relocs[reloc].offset - offset;

          if ((offset + length) < sec.size)
          {
            length += line_length;
            length -= length % line_length;
          }

          rtems::utils::dump (sec.data + offset,
                              length,
                              sizeof (uint8_t),
                              false,
                              line_length,
                              offset);

          const int          indent = 8;
          std::ostringstream line;

          line << std::setw (indent) << ' ';

          while ((reloc < sec.relocs.size ()) &&
                 (sec.relocs[reloc].offset >= offset) &&
                 (sec.relocs[reloc].offset < (offset + length)))
          {
            int spaces = ((((sec.relocs[reloc].offset + 1) % line_length) * 3) +
                          indent - 1);

            spaces -= line.str ().size ();

            line << std::setw (spaces) << " ^" << reloc
                 << ':' << std::hex
                 << sec.relocs[reloc].addend
                 << std::dec;

            ++reloc;
          }

          std::cout << line.str () << std::endl;

          offset += length;
        }

        if (sec.relocs.size ())
        {
          int count = 0;
          std::cout << "     info       offset     addend     symbol name" << std::endl;
          for (size_t f = 0; f < r.secs[s].relocs.size (); ++f)
          {
            rap::relocation& reloc = r.secs[s].relocs[f];
            std::cout << std::setw (4) << count++ << ' ';
            reloc.output ();
            std::cout << std::endl;
          }
        }

      }
    }
  }
}

void
rap_expander (rld::path::paths& raps, bool warnings)
{
  std::cout << "Expanding .... " << std::endl;
  for (rld::path::paths::iterator pi = raps.begin();
       pi != raps.end();
       ++pi)
  {
    rap::file r (*pi, warnings);
    std::cout << ' ' << r.name () << std::endl;
    r.expand ();
  }
}

/**
 * RTEMS RAP options.
 */
static struct option rld_opts[] = {
  { "help",        no_argument,            NULL,           'h' },
  { "version",     no_argument,            NULL,           'V' },
  { "verbose",     no_argument,            NULL,           'v' },
  { "no-warn",     no_argument,            NULL,           'n' },
  { "all",         no_argument,            NULL,           'a' },
  { "header",      no_argument,            NULL,           'H' },
  { "machine",     no_argument,            NULL,           'm' },
  { "layout",      no_argument,            NULL,           'l' },
  { "strings",     no_argument,            NULL,           's' },
  { "symbols",     no_argument,            NULL,           'S' },
  { "relocs",      no_argument,            NULL,           'r' },
  { "overlay",     no_argument,            NULL,           'o' },
  { "expand",      no_argument,            NULL,           'x' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-rap [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -n        : no warnings (also --no-warn)" << std::endl
            << " -a        : show all (also --all)" << std::endl
            << " -H        : show header (also --header)" << std::endl
            << " -m        : show machine details (also --machine)" << std::endl
            << " -l        : show layout (also --layout)" << std::endl
            << " -s        : show strings (also --strings)" << std::endl
            << " -S        : show symbols (also --symbols)" << std::endl
            << " -r        : show relocations (also --relocs)" << std::endl
            << " -o        : linkage overlay (also --overlay)" << std::endl
            << " -x        : expand (also --expand)" << std::endl
            << " -f        : show file details" << std::endl;
  ::exit (exit_code);
}

static void
fatal_signal (int signum)
{
  signal (signum, SIG_DFL);

  rld::process::temporaries_clean_up ();

  /*
   * Get the same signal again, this time not handled, so its normal effect
   * occurs.
   */
  kill (getpid (), signum);
}

static void
setup_signals (void)
{
  if (signal (SIGINT, SIG_IGN) != SIG_IGN)
    signal (SIGINT, fatal_signal);
#ifdef SIGHUP
  if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
    signal (SIGHUP, fatal_signal);
#endif
  if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
    signal (SIGTERM, fatal_signal);
#ifdef SIGPIPE
  if (signal (SIGPIPE, SIG_IGN) != SIG_IGN)
    signal (SIGPIPE, fatal_signal);
#endif
#ifdef SIGCHLD
  signal (SIGCHLD, SIG_DFL);
#endif
}

int
main (int argc, char* argv[])
{
  int ec = 0;

  setup_signals ();

  try
  {
    rld::path::paths raps;
    bool             warnings = true;
    bool             show = false;
    bool             show_header = false;
    bool             show_machine = false;
    bool             show_layout = false;
    bool             show_strings = false;
    bool             show_symbols = false;
    bool             show_relocs = false;
    bool             show_details = false;
    bool             overlay = false;
    bool             expand = false;

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvVnaHmlsSroxf", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-rap (RTEMS RAP Manager) " << rld::version ()
                    << ", RTEMS revision " << rld::rtems::version ()
                    << std::endl;
          ::exit (0);
          break;

        case 'v':
          rld::verbose_inc ();
          break;

        case 'n':
          warnings = false;
          break;

        case 'a':
          show = true;
          show_header = true;
          show_machine = true;
          show_layout = true;
          show_strings = true;
          show_symbols = true;
          show_relocs = true;
          show_details = true;
          break;

        case 'H':
          show = true;
          show_header = true;
          break;

        case 'm':
          show = true;
          show_machine = true;
          break;

        case 'l':
          show = true;
          show_layout = true;
          break;

        case 's':
          show = true;
          show_strings = true;
          break;

        case 'S':
          show = true;
          show_symbols = true;
          break;

        case 'r':
          show = true;
          show_relocs = true;
          break;

        case 'o':
          overlay = true;
          break;

        case 'x':
          expand = true;
          break;

        case 'f':
          show_details = true;
          break;

        case '?':
        case 'h':
          usage (0);
          break;
      }
    }

    argc -= optind;
    argv += optind;

    std::cout << "RTEMS RAP " << rld::version () << std::endl << std::endl;

    /*
     * If there are no RAP files so there is nothing to do.
     */
    if (argc == 0)
      throw rld::error ("no RAP files", "options");

    /*
     * Load the remaining command line arguments into a container.
     */
    while (argc--)
      raps.push_back (*argv++);

    if (show)
      rap_show (raps,
                warnings,
                show_header,
                show_machine,
                show_layout,
                show_strings,
                show_symbols,
                show_relocs,
                show_details);

    if (overlay)
      rap_overlay (raps, warnings);

    if (expand)
      rap_expander (raps, warnings);
  }
  catch (rld::error re)
  {
    std::cerr << "error: "
              << re.where << ": " << re.what
              << std::endl;
    ec = 10;
  }
  catch (std::exception e)
  {
    int   status;
    char* realname;
    realname = abi::__cxa_demangle (e.what(), 0, 0, &status);
    std::cerr << "error: exception: " << realname << " [";
    ::free (realname);
    const std::type_info &ti = typeid (e);
    realname = abi::__cxa_demangle (ti.name(), 0, 0, &status);
    std::cerr << realname << "] " << e.what () << std::endl;
    ::free (realname);
    ec = 11;
  }
  catch (...)
  {
    /*
     * Helps to know if this happens.
     */
    std::cout << "error: unhandled exception" << std::endl;
    ec = 12;
  }

  return ec;
}
