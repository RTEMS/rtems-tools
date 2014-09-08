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
  static std::string progname;

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
  //static std::string output;

  bool
  starts_with(const std::string& s1, const std::string& s2)
  {
    return s2.size () <= s1.size () && s1.compare (0, s2.size (), s2) == 0;
  }

  const std::string
  ltrim (const std::string& s)
  {
    std::string t = s;
    t.erase (t.begin (),
             std::find_if (t.begin (), t.end (),
                         std::not1 (std::ptr_fun < int, int > (std::isspace))));
    return t;
  }

  const std::string
  rtrim (const std::string& s)
  {
    std::string t = s;
    t.erase (std::find_if (t.rbegin (), t.rend (),
                           std::not1 (std::ptr_fun < int, int > (std::isspace))).base(),
             t.end());
    return t;
  }

  const std::string
  trim (const std::string& s)
  {
    return ltrim (rtrim (s));
  }

  const std::string
  dequote (const std::string& s)
  {
    if (!s.empty ())
    {
      char front = s[0];
      char back = s[s.length () - 1];
      if ((front == '"') || (front == '\''))
      {
        if (front != back)
          throw rld::error ("invalid quoting", "string: " + s);
        return s.substr (1, s.length () - (1 + 1));
      }
    }
    return s;
  }

  const std::string
  find_replace(const std::string& sin,
               const std::string& out,
               const std::string& in)
  {
    std::string s = sin;
    size_t      pos = 0;
    while ((pos = s.find (out, pos)) != std::string::npos)
    {
      s.replace (pos, out.length (), in);
      pos += in.length ();
    }
    return s;
  }

  const strings
  split (strings&           se,
         const std::string& s,
         char               delimiter,
         bool               strip_quotes,
         bool               strip_whitespace,
         bool               empty)
  {
    std::stringstream ss(s);
    std::string       e;
    se.clear ();
    while (std::getline (ss, e, delimiter))
    {
      if (strip_whitespace)
        e = trim (e);
      if (strip_quotes)
        e = dequote (e);
      if (empty || !e.empty ())
      {
        se.push_back (e);
      }
    }
    return se;
  }

  const std::string
  join (const strings& ss, const std::string& separator)
  {
    std::string s;
    for (strings::const_iterator ssi = ss.begin ();
         ssi != ss.end ();
         ++ssi)
    {
      s += *ssi;
      if ((ssi + 1) != ss.end ())
        s += separator;
    }
    return s;
  }

  const std::string
  tolower (const std::string& sin)
  {
    std::string s = sin;
    std::transform (s.begin (), s.end (), s.begin (), ::tolower);
    return s;
  }

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
  set_progname (const std::string& progname_)
  {
    progname = rld::path::path_abs (progname_);
  }

  const std::string
  get_progname ()
  {
    return progname;
  }

  const std::string
  get_program_name ()
  {
    return rld::path::basename (progname);
  }

  const std::string
  get_program_path ()
  {
    return rld::path::dirname (progname);
  }

  const std::string
  get_prefix ()
  {
    std::string pp = get_program_path ();
    std::cout << "PP=" << pp << std::endl;
    if (rld::path::basename (pp) == "bin")
      pp = rld::path::dirname (pp);
    return pp;
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
