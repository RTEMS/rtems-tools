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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rld-process.h"

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

        symbols::symtab& unresolved = obj.unresolved_symbols ();

        int count = 0;
        for (symbols::symtab::iterator ursi = unresolved.begin ();
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
    archivera (const std::string&        name,
               const files::object_list& dependents,
               files::cache&             cache,
               bool                      ra_exist,
               bool                      ra_rap)
    {
      files::object_list dep_copy (dependents);
      files::object_list objects;

      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:archivera: " << name
                  << ", dependents: " << dependents.size () << std::endl;

      objects.clear ();

      files::object_list::iterator oli;
      for (oli = dep_copy.begin (); oli != dep_copy.end (); ++oli)
      {
        files::object& object = *(*oli);

        if (ra_rap)
          objects.push_back (&object);
        else
        {
          if (object.get_archive ())
            objects.push_back (&object);
        }
      }

      bool exist = false;
      files::object_list objects_tmp;
      files::objects& objs = cache.get_objects ();
      objects_tmp.clear ();
      for (files::objects::iterator obi = objs.begin ();
           obi != objs.end ();
           ++obi)
      {
        files::object* obj = (*obi).second;
        exist = false;

        /**
         * Replace the elf object file in ra file with elf object file
         * in collected object files, if exist.
         */
        if (!ra_rap)
        {
          for (oli = objects.begin (); oli != objects.end (); ++oli)
          {
            files::object& object = *(*oli);
            if (obj->name ().oname () == object.name ().oname ())
            {
              exist = true;
              break;
            }
          }
        }

        if (!exist)
          objects_tmp.push_back (obj);
      }

      objects.merge (objects_tmp);
      objects.unique ();

      if (objects.size ())
      {
        if (ra_exist)
        {
          std::string new_name = "rld_XXXXXX";
          struct stat sb;
          files::archive arch (new_name);
          arch.create (objects);

          if ((::stat (name.c_str (), &sb) >= 0) && S_ISREG (sb.st_mode))
          {
            if (::unlink (name.c_str ()) < 0)
              std::cerr << "error: unlinking temp file: " << name << std::endl;
          }
          if (::link (new_name.c_str (), name.c_str ()) < 0)
          {
            std::cerr << "error: linking temp file: " << name << std::endl;
          }
          if ((::stat (new_name.c_str (), &sb) >= 0) && S_ISREG (sb.st_mode))
          {
            if (::unlink (new_name.c_str ()) < 0)
              std::cerr << "error: unlinking temp file: " << new_name << std::endl;
          }
        }
        else
        {
          /* Create */
          files::archive arch (name);
          arch.create (objects);
        }
      }
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

    bool in_archive (files::object* object)
    {
      if (object->get_archive ())
        return true;
      return false;
    }

    void
    application (const std::string&        name,
                 const std::string&        entry,
                 const std::string&        exit,
                 const files::object_list& dependents,
                 const files::cache&       cache,
                 const symbols::table&     symbols,
                 bool                      one_file)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:application: " << name << std::endl;

      files::object_list dep_copy (dependents);
      files::object_list objects;
      files::image       app (name);

      if (!one_file)
        dep_copy.remove_if (in_archive);

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.unique ();

      app.open (true);

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
