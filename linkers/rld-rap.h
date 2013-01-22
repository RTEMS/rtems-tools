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
 * @ingroup rtems-ld
 *
 * @brief RTEMS Linker RTEMS Application (RAP) Format management.
 *
 */

#if !defined (_RLD_RAP_H_)
#define _RLD_RAP_H_

#include <rld-files.h>

namespace rld
{
  namespace rap
  {
    /**
     * The RAP relocation bit masks.
     */
    #define RAP_RELOC_RELA         (1UL << 31)
    #define RAP_RELOC_STRING       (1UL << 31)
    #define RAP_RELOC_STRING_EMBED (1UL << 30)

    /**
     * The sections of interest in a RAP file.
     */
    enum sections
    {
      rap_text = 0,
      rap_const = 1,
      rap_ctor = 2,
      rap_dtor = 3,
      rap_data = 4,
      rap_bss = 5,
      rap_secs = 6
    };

    /**
     * Return the name of a section.
     */
    const char* section_name (int sec);

    /**
     * Write a RAP format file.
     *
     * The symbol table is provided to allow incremental linking at some point
     * in the future. I suspect this will also require extra options being
     * added to control symbol visibility in the RAP file. For example an
     * "application" may be self contained and does not need to export any
     * symbols therefore no symbols are added and the only ones are part of the
     * relocation records to bind to base image symbols. Another case is the
     * need for an application to export symbols because it is using dlopen to
     * load modules. Here the symbols maybe 'all' or it could be a user
     * maintained list that are exported.
     *
     * @param app The application image to write too.
     * @param init The application's initialisation entry point.
     * @param fini The application's finish entry point .
     * @param objects The list of object files in the application.
     * @param symbols The symbol table used to create the application.
     */
    void write (files::image&             app,
                const std::string&        init,
                const std::string&        fini,
                const files::object_list& objects,
                const symbols::table&     symbols);
  }
}

#endif
