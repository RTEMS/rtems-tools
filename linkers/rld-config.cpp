/*
 * Copyright (c) 2014, Chris Johns <chrisj@rtems.org>
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
 * @ingroup rtems_rld
 *
 * @brief INI Configuration reader.
 *
 */

#include <errno.h>

#include <rld-config.h>
#include <rld.h>

#include <SimpleIni.h>

namespace rld
{
  namespace config
  {
    item::item (const std::string& text)
      : text (text)
    {
    }

    item::item (const char* text)
      : text (text)
    {
    }

    const record&
    section::get_record (const std::string& name) const
    {
      for (records::const_iterator ri = recs.begin ();
           ri != recs.end ();
           ++ri)
      {
        if ((*ri).name == name)
          return *ri;
      }

      throw error ("not found", "config record: " + this->name + '/' + name);
    }

    config::config()
    {
    }

    config::~config()
    {
    }

    void
    config::clear ()
    {
      secs.clear ();
    }

    void
    config::load (const std::string& path)
    {
      CSimpleIniCaseA ini (false, true, true);

      if (ini.LoadFile (path.c_str ()) != SI_OK)
        throw rld::error (::strerror (errno), "load config: " + path);

      paths.push_back (path);

      /*
       * Merge the loaded configuration into our configuration.
       */

      CSimpleIniCaseA::TNamesDepend skeys;

      ini.GetAllSections(skeys);

      for (CSimpleIniCaseA::TNamesDepend::const_iterator si = skeys.begin ();
           si != skeys.end ();
           ++si)
      {
        section sec;

        sec.name = (*si).pItem;

        CSimpleIniCaseA::TNamesDepend rkeys;

        ini.GetAllKeys((*si).pItem, rkeys);

        for (CSimpleIniCaseA::TNamesDepend::const_iterator ri = rkeys.begin ();
             ri != rkeys.end ();
             ++ri)
        {
          record rec;

          rec.name = (*ri).pItem;

          CSimpleIniCaseA::TNamesDepend vals;

          ini.GetAllValues((*si).pItem, (*ri).pItem, vals);

          for (CSimpleIniCaseA::TNamesDepend::const_iterator vi = vals.begin ();
               vi != vals.end ();
               ++vi)
          {
            rec.items.push_back (item ((*vi).pItem));
          }

          sec.recs.push_back (rec);
        }

        secs.push_back (sec);
      }
    }

    const section&
    config::get_section (const std::string& name) const
    {
      for (sections::const_iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        if ((*si).name == name)
          return *si;
      }

      throw error ("not found", "config section: " + name);
    }

    const paths&
    config::get_paths () const
    {
      return paths;
    }
  }
}
