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
 * @ingroup rtems_ld
 *
 * @brief RTEMS Linker.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

#include <sys/stat.h>

#include <rld.h>

#define RLD_VERSION_MAJOR   (1)
#define RLD_VERSION_MINOR   (0)
#define RLD_VERSION_RELEASE (0)

namespace rld
{
  static int verbose_level = 0;

  /**
   * The option container.
   */
  typedef std::vector < std::string > library_container;

  /**
   * The libraries the user provided on the command line.
   */
  static library_container libpaths;

  /**
   * The libraries pass on the command line.
   */
  static library_container libs;

  /**
   * The libraries.
   */
  static library_container libraries;

  /**
   * The output passed on the command line.
   */
  static std::string output;

  void
  verbose_inc ()
  {
    ++verbose_level;
  }

  int
  verbose ()
  {
    return verbose_level;
  }

  const std::string
  version ()
  {
    std::string v = (rld::to_string (RLD_VERSION_MAJOR) + '.' +
                     rld::to_string (RLD_VERSION_MINOR) + '.' +
                     rld::to_string (RLD_VERSION_RELEASE));
    return v;
  }

  const std::string
  rtems_version ()
  {
    return rld::to_string (RTEMS_VERSION);
  }

  void
  map (rld::files::cache& cache, rld::symbols::table& symbols)
  {
    std::cout << "Archive files    : " << cache.archive_count () << std::endl;
    std::cout << "Object files     : " << cache.object_count () << std::endl;
    std::cout << "Exported symbols : " << symbols.size () << std::endl;

    std::cout << "Archives:" << std::endl;
    cache.output_archive_files (std::cout);
    std::cout << "Objects:" << std::endl;
    cache.output_object_files (std::cout);

    std::cout << "Exported symbols:" << std::endl;
    rld::symbols::output (std::cout, symbols);
    std::cout << "Unresolved symbols:" << std::endl;
    cache.output_unresolved_symbols (std::cout);
  }

  void
  warn_unused_externals (rld::files::object_list& objects)
  {
    bool first = true;
    for (rld::files::object_list::iterator oli = objects.begin ();
         oli != objects.end ();
         ++oli)
    {
      rld::files::object&     object = *(*oli);
      rld::symbols::pointers& externals = object.external_symbols ();

      if (rld::symbols::referenced (externals) != externals.size ())
      {
        if (first)
        {
          std::cout << "Unreferenced externals in object files:" << std::endl;
          first = false;
        }

        std::cout << ' ' << object.name ().basename () << std::endl;

        for (rld::symbols::pointers::iterator sli = externals.begin ();
             sli != externals.end ();
             ++sli)
        {
          rld::symbols::symbol& sym = *(*sli);
          if (sym.references () == 0)
            std::cout << "  " << sym.name () << std::endl;
        }
      }
    }
  }

}
