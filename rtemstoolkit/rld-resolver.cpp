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
    static files::object*
    get_object (files::cache&      cache,
                const std::string& fullname)
    {
      files::objects&          objects = cache.get_objects ();
      files::objects::iterator oi = objects.find (fullname);
      if (oi == objects.end ())
        return 0;
      return (*oi).second;
    }

    static void
    resolve_symbols (files::object_list& dependents,
                     files::cache&       cache,
                     symbols::table&     base_symbols,
                     symbols::table&     symbols,
                     symbols::symtab&    unresolved,
                     const std::string&  fullname)
    {
      const std::string name = path::basename (fullname);

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

      files::object* object = get_object (cache, fullname);

      if (object)
      {
        if (object->resolved () || object->resolving ())
        {
          if (rld::verbose () >= RLD_VERBOSE_INFO)
            std::cout << "resolver:resolving: "
                      << std::setw (nesting - 1) << ' '
                      << name
                      << " is resolved or resolving"
                      << std::endl;
          return;
        }
        object->resolve_set ();
      }

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "resolver:resolving: "
                  << std::setw (nesting - 1) << ' '
                  << name
                  << ", unresolved: "
                  << unresolved.size ()
                  << std::endl;

      files::object_list objects;

      for (symbols::symtab::iterator ursi = unresolved.begin ();
           ursi != unresolved.end ();
           ++ursi)
      {
        symbols::symbol& urs = *((*ursi).second);

        if ((urs.binding () != STB_WEAK) && urs.object ())
          continue;

        symbols::symbol* es = base_symbols.find_global (urs.name ());
        bool             base = true;

        if (rld::verbose () >= RLD_VERBOSE_INFO)
        {
          std::cout << "resolver:resolve  : "
                    << std::setw (nesting + 1) << ' '
                    << " |- " << urs.name () << std::endl;
        }

        if (!es)
        {
          es = symbols.find_global (urs.name ());
          if (!es)
          {
            es = symbols.find_weak (urs.name ());
            if (!es)
              throw rld::error ("symbol not found: " + urs.name (), name);
          }
          base = false;
        }

        symbols::symbol& esym = *es;

        if (rld::verbose () >= RLD_VERBOSE_INFO)
        {
          std::cout << "resolver:resolved : "
                    << std::setw (nesting + 1) << ' '
                    << " |   `--> ";
          if (esym.object())
          {
            std::cout << esym.object()->name ().basename ();
            if (esym.object()->resolving ())
              std::cout << " (resolving)";
            else if (esym.object()->resolved ())
              std::cout << " (resolved)";
            else if (base)
              std::cout << " (base)";
            else
              std::cout << " (unresolved: " << objects.size () + 1 << ')';
          }
          else
            std::cout << "null";
          std::cout << std::endl;
        }

        if (!base)
        {
          files::object& eobj = *esym.object ();
          urs.set_object (eobj);
          if (!eobj.resolved () && !eobj.resolving ())
          {
            objects.push_back (&eobj);
            objects.unique ();
          }
        }

        esym.referenced ();
      }

      if (object)
      {
        object->resolve_clear ();
        object->resolved_set ();
      }

      /*
       * Recurse into any references object files.
       */

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "resolver:resolved : "
                  << std::setw (nesting + 1) << ' '
                  << " +-- referenced objects: " << objects.size ()
                  << std::endl;

      for (files::object_list::iterator oli = objects.begin ();
           oli != objects.end ();
           ++oli)
      {
        files::object& obj = *(*oli);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << "resolver:resolving: "
                    << std::setw (nesting) << ' '
                    << "] " << name << " ==> "
                    << obj.name ().basename () << std::endl;
        resolve_symbols (dependents, cache, base_symbols, symbols,
                         obj.unresolved_symbols (),
                         obj.name ().full ());
      }

      --nesting;

      dependents.merge (objects);
      dependents.unique ();
    }

    void
    resolve (files::object_list& dependents,
             files::cache&       cache,
             symbols::table&     base_symbols,
             symbols::table&     symbols,
             symbols::symtab&    undefined)
    {
      files::object_list objects;
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
      for (files::object_list::iterator oi = objects.begin ();
           oi != objects.end ();
           ++oi)
      {
        files::object& object = *(*oi);
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << "resolver:resolving: top: "
                    << object.name ().basename () << std::endl;
        resolver::resolve_symbols (dependents, cache, base_symbols, symbols,
                                   object.unresolved_symbols (),
                                   object.name ().full ());
      }
    }
  }

}
