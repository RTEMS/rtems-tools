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
#include <string.h>

#include <rld.h>
#include <rld-rap.h>

namespace rld
{
  namespace outputter
  {
    const std::string
    script_text (const std::string&        entry,
                 const std::string&        exit,
                 const files::object_list& dependents,
                 const files::cache&       cache,
                 bool                      not_in_archive)
    {
      std::ostringstream out;
      files::object_list objects;
      files::object_list dep_copy (dependents);

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.unique ();

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << " E: " << entry << std::endl;

      out << "E: " << entry << std::endl;

      if (!exit.empty ())
      {
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " e: " << exit << std::endl;
        out << "e: " << exit << std::endl;
      }

      for (files::object_list::iterator oi = objects.begin ();
           oi != objects.end ();
           ++oi)
      {
        files::object& obj = *(*oi);
        std::string    name = obj.name ().basename ();

        if (not_in_archive)
        {
          size_t pos = name.find (':');
          if (pos != std::string::npos)
            name[pos] = '_';
          pos = name.find ('@');
          if (pos != std::string::npos)
            name = name.substr (0, pos);
        }

        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " o: " << name << std::endl;

        out << "o:" << name << std::endl;

        symbols::table& unresolved = obj.unresolved_symbols ();

        int count = 0;
        for (symbols::table::iterator ursi = unresolved.begin ();
             ursi != unresolved.begin ();
             ++ursi)
        {
          symbols::symbol& urs = *((*ursi).second);

          ++count;

          if (rld::verbose () >= RLD_VERBOSE_INFO)
            std::cout << " u: " << count << ':' << urs.name () << std::endl;

          out << " u:" << count << ':' << urs.name () << std::endl;
        }
      }

      return out.str ();
    }

    void
    metadata_object (files::object&            metadata,
                     const std::string&        entry,
                     const std::string&        exit,
                     const files::object_list& dependents,
                     const files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "metadata: " << metadata.name ().full () << std::endl;

      const std::string script =
        script_text (entry, exit, dependents, cache, true);

      metadata.open (true);
      metadata.begin ();

      elf::file& elf = metadata.elf ();

      elf.set_header (ET_EXEC,
                      elf::object_class (),
                      elf::object_datatype (),
                      elf::object_machine_type ());

      elf::section md (elf,
                       elf.section_count () + 1,
                       ".rtemsmd",
                       SHT_STRTAB,
                       1,
                       0,
                       0,
                       0,
                       script.length ());

      md.add_data (ELF_T_BYTE,
                   1,
                   script.length (),
                   (void*) script.c_str ());

      elf.add (md);
      elf.write ();

      metadata.end ();
      metadata.close ();
    }

    void
    archive (const std::string&        name,
             const std::string&        entry,
             const std::string&        exit,
             const files::object_list& dependents,
             const files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:archive: " << name
                  << ", dependents: " << dependents.size () << std::endl;

      std::string ext = files::extension (name);
      std::string mdname =
        name.substr (0, name.length () - ext.length ()) + "-metadata.o";

      files::object metadata (mdname);

      metadata_object (metadata, entry, exit, dependents, cache);

      files::object_list dep_copy (dependents);
      files::object_list objects;

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.push_front (&metadata);
      objects.unique ();

      files::archive arch (name);
      arch.create (objects);
    }

    void
    script (const std::string&        name,
            const std::string&        entry,
            const std::string&        exit,
            const files::object_list& dependents,
            const files::cache&       cache)
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
        out << script_text (entry, exit, dependents, cache, false);
      }
      catch (...)
      {
        out.close ();
        throw;
      }

      out.close ();
    }

    void
    elf_application (const std::string&        name,
                     const std::string&        entry,
                     const std::string&        exit,
                     const files::object_list& dependents,
                     const files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:application: " << name << std::endl;

      files::object_list dep_copy (dependents);
      files::object_list objects;
      std::string        header;
      std::string        script;
      files::image       app (name);

      header = "RELF,00000000,0001,none,00000000\n";
      header += '\0';

      script = script_text (entry, exit, dependents, cache, true);

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.unique ();

      app.open (true);
      app.write (header.c_str (), header.size ());

      #define APP_BUFFER_SIZE  (128 * 1024)

      uint8_t* buffer = 0;

      try
      {
        buffer = new uint8_t[APP_BUFFER_SIZE];

        for (files::object_list::iterator oi = objects.begin ();
             oi != objects.end ();
             ++oi)
        {
          files::object& obj = *(*oi);

          obj.open ();

          try
          {
            obj.seek (0);

            size_t in_size = obj.name ().size ();

            while (in_size)
            {
              size_t reading =
                in_size < APP_BUFFER_SIZE ? in_size : APP_BUFFER_SIZE;

              app.write (buffer, obj.read (buffer, reading));

              in_size -= reading;
            }
          }
          catch (...)
          {
            obj.close ();
            throw;
          }

          obj.close ();
        }
      }
      catch (...)
      {
        delete [] buffer;
        app.close ();
        throw;
      }

      delete [] buffer;

      app.close ();
    }

    void
    application (const std::string&        name,
                 const std::string&        entry,
                 const std::string&        exit,
                 const files::object_list& dependents,
                 const files::cache&       cache,
                 const symbols::table&     symbols)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:application: " << name << std::endl;

      files::object_list dep_copy (dependents);
      files::object_list objects;
      std::string        header;
      files::image       app (name);

      header = "RAP,00000000,0001,LZ77,00000000\n";

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.unique ();

      app.open (true);
      app.write (header.c_str (), header.size ());

      try
      {
        rap::write (app, entry, exit, objects, symbols);
      }
      catch (...)
      {
        app.close ();
        throw;
      }

      app.close ();
    }

  }
}
