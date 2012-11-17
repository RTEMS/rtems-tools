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

#include <fstream>
#include <iostream>

#include <errno.h>

#include <rld.h>
#include <rld.h>

namespace rld
{
  namespace outputter
  {
    const std::string
    script_text (rld::files::object_list& dependents,
                 rld::files::cache&       cache)
    {
      std::ostringstream      out;
      rld::files::object_list objects;

      cache.get_objects (objects);

      objects.merge (dependents);
      objects.unique ();

      for (rld::files::object_list::iterator oi = objects.begin ();
           oi != objects.end ();
           ++oi)
      {
        rld::files::object& obj = *(*oi);

        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " o: " << obj.name ().full () << std::endl;

        out << "o:" << obj.name ().basename () << std::endl;

        rld::symbols::table& unresolved = obj.unresolved_symbols ();

        int count = 0;
        for (rld::symbols::table::iterator ursi = unresolved.begin ();
             ursi != unresolved.begin ();
             ++ursi)
        {
          rld::symbols::symbol& urs = *((*ursi).second);

          ++count;

          if (rld::verbose () >= RLD_VERBOSE_INFO)
            std::cout << " u: " << count << ':' << urs.name () << std::endl;

          out << " u:" << count << ':' << urs.name () << std::endl;
        }
      }

      return out.str ();
    }

    void
    archive (const std::string&       name,
             rld::files::object_list& dependents,
             rld::files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:archive: " << name << std::endl;

      rld::files::object_list objects;
      cache.get_objects (objects);

      for (rld::files::object_list::iterator oi = dependents.begin ();
           oi != dependents.end ();
           ++oi)
        objects.push_back (*oi);

      objects.unique ();

      rld::files::archive arch (name);
      arch.create (objects);
    }

    void
    script (const std::string&       name,
            rld::files::object_list& dependents,
            rld::files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:script: " << name << std::endl;

      std::fstream out (name.c_str (),
                        std::ios_base::out | std::ios_base::trunc);

      /*
       * Tag for the shell to use.
       */
      out << "!# rls" << std::endl;

      try
      {
        out << script_text (dependents, cache);
      }
      catch (...)
      {
        out.close ();
      }

      out.close ();
    }
  }
}
