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

    bool
    section::has_record (const std::string& name) const
    {
      for (records::const_iterator ri = recs.begin ();
           ri != recs.end ();
           ++ri)
      {
        if ((*ri).name == name)
          return true;
      }
      return false;
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

      throw rld::error ("not found", "config record: " + this->name + '/' + name);
    }

    const std::string
    section::get_record_item (const std::string& rec_name) const
    {
      const record& rec = get_record (rec_name);
      if (rec.items_.size () != 1)
        throw rld::error ("duplicate", "record item: " + name + '/' + rec_name);
      return rec.items_[0].text;
    }

    void
    section::get_record_items (const std::string& rec_name, rld::strings& items) const
    {
      const record& rec = get_record (rec_name);
      items.clear ();
      for (rld::config::items::const_iterator ii = rec.items_.begin ();
           ii != rec.items_.end ();
           ++ii)
      {
        items.push_back ((*ii).text);
      }
    }

    config::config(const std::string& search_path)
    {
      set_search_path (search_path);
    }

    config::~config()
    {
    }

    void
    config::set_search_path (const std::string& search_path)
    {
      if (!search_path.empty ())
        rld::path::path_split (search_path, search);
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

      std::string checked_path;

      if (rld::path::check_file (path))
      {
        checked_path = path;
      }
      else
      {
        bool found = false;
        for (rld::path::paths::const_iterator spi = search.begin ();
             spi != search.end ();
             ++spi)
        {
          rld::path::path_join (*spi, path, checked_path);
          if (rld::path::check_file (checked_path))
          {
            found = true;
            break;
          }
        }
        if (!found)
          throw rld::error ("Not found.", "load config: " + path);
      }

      if (ini.LoadFile (checked_path.c_str ()) != SI_OK)
        throw rld::error (::strerror (errno), "load config: " + path);

      paths_.push_back (checked_path);

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
            rec.items_.push_back (item ((*vi).pItem));
          }

          sec.recs.push_back (rec);
        }

        secs.push_back (sec);
      }
    }


    void
    config::includes (const section& sec, bool must_exist)
    {
      bool have_includes = false;

      try
      {
        rld::strings is;
        parse_items (sec, "include", is);

        have_includes = true;

        /*
         * Include records are a paths which we can load.
         */

        for (rld::strings::const_iterator isi = is.begin ();
             isi != is.end ();
             ++isi)
        {
          load (*isi);
        }
      }
      catch (rld::error re)
      {
        /*
         * No include records, must be all inlined. If we have includes it must
         * be another error so throw it.
         */
        if (have_includes || (!have_includes && must_exist))
          throw;
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
      return paths_;
    }
  }
}
