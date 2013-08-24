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
 * @brief RTEMS Linker outputter handles the various output formats.
 *
 */

#if !defined (_RLD_OUTPUTTER_H_)
#define _RLD_OUTPUTTER_H_

#include <rld-files.h>

namespace rld
{
  namespace outputter
  {
    /**
     * Output the object file list as a string.
     *
     * @param entry The name of the entry point symbol.
     * @param exit The name of the exit point symbol.
     * @param dependents The list of dependent object files
     * @param cache The file cache for the link. Includes the object list
     *              the user requested.
     * @return std::string The list as a text string.
     */
    std::string script_text (const std::string&        entry,
                             const std::string&        exit,
                             const files::object_list& dependents,
                             const files::cache&       cache);
    /**
     * Output the object files as an archive format file with the metadata as
     * the first ELF file.
     *
     * @param name The name of the archive.
     * @param entry The name of the entry point symbol.
     * @param exit The name of the exit point symbol.
     * @param dependents The list of dependent object files
     * @param cache The file cache for the link. Includes the object list
     *              the user requested.
     */
    void archive (const std::string&        name,
                  const std::string&        entry,
                  const std::string&        exit,
                  const files::object_list& dependents,
                  const files::cache&       cache);

    void archivera (const std::string&        name,
                    const files::object_list& dependents,
                    files::cache&             cache,
                    bool                      ra_exist);

    /**
     * Output the object file list as a script.
     *
     * @param name The name of the script.
     * @param entry The name of the entry point symbol.
     * @param exit The name of the exit point symbol.
     * @param dependents The list of dependent object files
     * @param cache The file cache for the link. Includes the object list
     *              the user requested.
     */
    void script (const std::string&        name,
                 const std::string&        entry,
                 const std::string&        exit,
                 const files::object_list& dependents,
                 const files::cache&       cache);

    /**
     * Output the object files in an archive with the metadata.
     *
     * @param name The name of the script.
     * @param entry The name of the entry point symbol.
     * @param exit The name of the exit point symbol.
     * @param dependents The list of dependent object files
     * @param cache The file cache for the link. Includes the object list
     *              the user requested.
     */
    void elf_application (const std::string&        name,
                          const std::string&        entry,
                          const std::string&        exit,
                          const files::object_list& dependents,
                          const files::cache&       cache);

    /**
     * Output the object files in an archive with the metadata.
     *
     * @param name The name of the script.
     * @param entry The name of the entry point symbol.
     * @param exit The name of the exit point symbol.
     * @param dependents The list of dependent object files
     * @param cache The file cache for the link. Includes the object list
     *              the user requested.
     * @param symbols The symbol table used to resolve the application.
     */
    void application (const std::string&        name,
                      const std::string&        entry,
                      const std::string&        exit,
                      const files::object_list& dependents,
                      const files::cache&       cache,
                      const symbols::table&     symbols,
                      bool                      one_file);

  }
}

#endif
