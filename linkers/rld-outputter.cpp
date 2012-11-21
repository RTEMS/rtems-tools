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

#include "fastlz.h"

namespace rld
{
  namespace outputter
  {
    const std::string
    script_text (files::object_list& dependents,
                 files::cache&       cache)
    {
      std::ostringstream out;
      files::object_list objects;

      /*
       * The merge removes them from the dependent list.
       */
      files::object_list dep_copy (dependents);

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.unique ();

      for (files::object_list::iterator oi = objects.begin ();
           oi != objects.end ();
           ++oi)
      {
        files::object& obj = *(*oi);

        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << " o: " << obj.name ().full () << std::endl;

        out << "o:" << obj.name ().basename () << std::endl;

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
    metadata_object (files::object&      metadata,
                     files::object_list& dependents,
                     files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "metadata: " << metadata.name ().full () << std::endl;

      const std::string script = script_text (dependents, cache);

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
    archive (const std::string&  name,
             files::object_list& dependents,
             files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:archive: " << name
                  << ", dependents: " << dependents.size () << std::endl;

      std::string ext = files::extension (name);
      std::string mdname =
        name.substr (0, name.length () - ext.length ()) + "-metadata.o";

      files::object metadata (mdname);

      metadata_object (metadata, dependents, cache);

      /*
       * The merge removes them from the dependent list.
       */
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
    script (const std::string&  name,
            files::object_list& dependents,
            files::cache&       cache)
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
        throw;
      }

      out.close ();
    }

    /**
     * Append the output data to the output buffer and if full compress and
     * write to the output file. If the output buffer is 0 flush the output
     * buffer.
     */
    static void
    app_write_output (files::image&  out,
                      const uint8_t* out_buffer,
                      const size_t   out_buffer_size,
                      size_t&        out_buffer_level,
                      const void*    output_,
                      size_t         outputting,
                      uint8_t*       compress_buffer,
                      size_t&        out_total)
    {
      const uint8_t* output = static_cast <const uint8_t*> (output_);

      while (outputting)
      {
        if (output)
        {
          size_t appending;

          if (outputting > (out_buffer_size - out_buffer_level))
            appending = out_buffer_size - out_buffer_level;
          else
            appending = outputting;

          ::memcpy ((void*) (out_buffer + out_buffer_level),
                    output,
                    appending);

          out_buffer_level += appending;
          outputting -= appending;
        }
        else
        {
          outputting = 0;
        }

        if (!output || (out_buffer_level >= out_buffer_size))
        {
          int writing =
            ::fastlz_compress (out_buffer, out_buffer_level, compress_buffer);

          out.write (compress_buffer, writing);

          out_total += writing;

          out_buffer_level = 0;
        }
      }
    }

    void
    application (const std::string&  name,
                 files::object_list& dependents,
                 files::cache&       cache)
    {
      if (rld::verbose () >= RLD_VERBOSE_INFO)
        std::cout << "outputter:application: " << name << std::endl;

      files::object_list dep_copy (dependents);
      files::object_list objects;
      std::string        header;
      std::string        script;
      files::image       app (name);

      header = "RTEMS-APP,00000000,01.00.00,LZ77,00000000\n";
      header += '\0';

      script = script_text (dependents, cache);

      cache.get_objects (objects);
      objects.merge (dep_copy);
      objects.unique ();

      app.open (true);
      app.write (header.c_str (), header.size ());

      #define INPUT_BUFFER_SIZE  (64 * 1024)
      #define OUTPUT_BUFFER_SIZE (128 * 1024)
      #define FASTLZ_BUFFER_SIZE (OUTPUT_BUFFER_SIZE + ((int) (OUTPUT_BUFFER_SIZE * 0.10)))

      uint8_t* in_buffer = 0;
      uint8_t* out_buffer = 0;
      uint8_t* compress_buffer = 0;
      size_t   out_level = 0;
      size_t   in_total = 0;
      size_t   out_total = 0;

      try
      {
        in_buffer = new uint8_t[INPUT_BUFFER_SIZE];
        out_buffer = new uint8_t[OUTPUT_BUFFER_SIZE];
        compress_buffer = new uint8_t[FASTLZ_BUFFER_SIZE];

        app_write_output (app,
                          out_buffer, OUTPUT_BUFFER_SIZE, out_level,
                          script.c_str (), script.size (),
                          compress_buffer,
                          out_total);

        in_total += script.size ();

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
                in_size < INPUT_BUFFER_SIZE ? in_size : INPUT_BUFFER_SIZE;

              obj.read (in_buffer, reading);

              app_write_output (app,
                                out_buffer, OUTPUT_BUFFER_SIZE, out_level,
                                in_buffer, reading,
                                compress_buffer,
                                out_total);

              in_size -= reading;
              in_total += reading;
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
        delete [] in_buffer;
        delete [] out_buffer;
        delete [] compress_buffer;
        throw;
      }

      app_write_output (app,
                        out_buffer, OUTPUT_BUFFER_SIZE, out_level,
                        0, out_level,
                        compress_buffer,
                        out_total);

      app.close ();

      delete [] in_buffer;
      delete [] out_buffer;
      delete [] compress_buffer;

      if (rld::verbose () >= RLD_VERBOSE_INFO)
      {
        int pcent = (out_total * 100) / in_total;
        int premand = (((out_total * 1000) + 500) / in_total) % 10;
        std::cout << "outputter:application: objects: " << objects.size ()
                  << ", size: " << out_total
                  << ", compression: " << pcent << '.' << premand << '%'
                  << std::endl;
      }
    }

  }
}
