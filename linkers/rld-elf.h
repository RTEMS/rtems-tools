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
     * Sections.
     */
    struct section
    {
      int         index; //< The section header index.
      std::string name;  //< The section's name.
      elf_scn*    scn;   //< ELF private section data.
      elf_shdr    shdr;  //< The section header.
      elf_data*   data;  //< The section's data.
      
      /**
       * Construct the section and get the data value.
       */
      section (int          index,
               std::string& name,
               elf_scn*     scn,
               elf_shdr&    sdhr);

      /**
       * Default constructor.
       */
      section ();
    };

    /**
     * Container of section headers.
     */
    typedef std::list < section > sections;

    /**
     * Get the machine type detected in the object files as their headers are read.
     */
    const std::string machine_type ();

    /**
     * Begin a libelf session with the image.
     */
    void begin (rld::files::image& image);

    /**
     * End the libelf session with the image.
     */
    void end (rld::files::image& image);

    /**
     * Get the ELF header.
     */
    void get_header (rld::files::image& image, elf_ehdr& ehdr);

    /**
     * Get the section headers for an object file.
     */
    void get_section_headers (rld::files::object& object, 
                              sections&           secs,
                              unsigned int        type = SHT_NULL);
 
    /**
     * Load the symbol table with the symbols.
     */
    void load_symbol_table (rld::symbols::table& exported,
                            rld::files::object&  object, 
                            section&             sec,
                            bool                 local = false,
                            bool                 weak = true,
                            bool                 global = true);
    
    /**
     * Load the symbol table with an object's symbols.
     */
    void load_symbols (rld::symbols::table& symbols,
                       rld::files::object&  object, 
                       bool                 local = false,
                       bool                 weak = true,
                       bool                 global = true);

    /**
     * Get a string.
     */
    std::string get_string (rld::files::object& object, 
                            int                 section,
                            size_t              offset);
  }
}

#endif
