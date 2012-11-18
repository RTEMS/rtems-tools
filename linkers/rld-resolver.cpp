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

#include <iomanip>
#include <iostream>

#include <sys/stat.h>

#include <rld.h>
#include <rld.h>

namespace rld
{
  namespace resolver
  {
    static void
    resolve_symbols (rld::files::object_list& dependents,
                     rld::files::cache&       cache,
                     rld::symbols::table&     base_symbols,
                     rld::symbols::table&     symbols,
                     rld::symbols::table&     unresolved,
                     const std::string&       name)
    {
      static int nesting = 0;

      ++nesting;

      /*
       * Find each unresolved symbol in the symbol table pointing the
       * unresolved symbol's object file to the file that resolves the
       * symbol. Record each object file that is found and when all unresolved
       * symbols in this object file have been found iterate over the found
       * object files resolving them. The 'urs' is the unresolved symbol and
       * 'es' is the exported symbol.
       */

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "resolver:resolving: "
                  << std::setw (nesting - 1) << ' '
                  << name
                  << ", unresolved: "
                  << unresolved.size ()
                  << std::endl;

      rld::files::object_list objects;

      for (rld::symbols::table::iterator ursi = unresolved.begin ();
           (ursi != unresolved.end ()) && !((*ursi).second)->object ();
           ++ursi)
      {
        rld::symbols::symbol&         urs = *((*ursi).second);
        rld::symbols::table::iterator esi = base_symbols.find (urs.name ());
        bool                          base = true;

        if (rld::verbose () >= RLD_VERBOSE_INFO)
        {
          std::cout << "resolver:resolve  : "
                    << std::setw (nesting + 1) << ' '
                    << urs.name () << std::endl;
        }

        if (esi == base_symbols.end ())
        {
          esi = symbols.find (urs.name ());
          if (esi == symbols.end ())
            throw rld::error ("symbol referenced in '" + name +
                              "' not found: " + urs.name (), "resolving");
          base = false;
        }

        rld::symbols::symbol& es = *((*esi).second);

        if (rld::verbose () >= RLD_VERBOSE_INFO)
        {
          std::cout << "resolver:resolved : "
                    << std::setw (nesting + 1) << ' '
                    << urs.name ()
                    << " -> ";
          if (es.object())
            std::cout << es.object()->name ().basename ();
          else
            std::cout << "null";
          std::cout << std::endl;
        }

        if (!base)
        {
          urs.set_object (*es.object ());
          objects.push_back (es.object ());
        }

        es.referenced ();
      }

      /*
       * Recurse into any references object files. First remove any duplicate
       * entries.
       */
      objects.unique ();

      for (rld::files::object_list::iterator oli = objects.begin ();
           oli != objects.end ();
           ++oli)
      {
        rld::files::object& object = *(*oli);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << "resolver:resolving:    : "
                    << object.name ().basename () << std::endl;
        resolve_symbols (dependents, cache, base_symbols, symbols,
                         object.unresolved_symbols (),
                         object.name ().basename ());
      }

      --nesting;

      dependents.merge (objects);
      dependents.unique ();
    }

    void
    resolve (rld::files::object_list& dependents,
             rld::files::cache&       cache,
             rld::symbols::table&     base_symbols,
             rld::symbols::table&     symbols,
             rld::symbols::table&     undefined)
    {
      rld::files::object_list objects;
      cache.get_objects (objects);

      /*
       * First resolve any undefined symbols that are forced by the linker or
       * the user.
       */
      resolver::resolve_symbols (dependents, cache, base_symbols, symbols,
                                 undefined, "undefines");

      /*
       * Resolve the symbols in the object files.
       */
      for (rld::files::object_list::iterator oi = objects.begin ();
           oi != objects.end ();
           ++oi)
      {
        rld::files::object& object = *(*oi);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << "resolver:resolving: top: "
                    << object.name ().basename () << std::endl;
        resolver::resolve_symbols (dependents, cache, base_symbols, symbols,
                                   object.unresolved_symbols (),
                                   object.name ().basename ());
      }
    }
  }

}
