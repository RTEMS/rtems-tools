/*
 * Copyright (c) 2011-2014, Chris Johns <chrisj@rtems.org>
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

#include <algorithm>
#include <fstream>
#include <string>

#include <rld.h>
#include <pkgconfig.h>

namespace pkgconfig
{
  package::package (const std::string& name)
  {
    load (name);
  }

  package::package ()
  {
  }

  void
  package::load (const std::string& name)
  {
    std::ifstream in (name.c_str (), std::ios::in);

    while (!in.eof ())
    {
      char buffer[1024];

      in.getline (buffer, sizeof (buffer));

      std::string line (buffer);
      size_t      hash;

      hash = line.find ('#');
      if (hash != std::string::npos)
        line.erase(hash);

      if (line.size () > 0)
      {
        size_t eq = line.find_first_of ('=');
        size_t dd = line.find_first_of (':');

        size_t d = std::string::npos;
        bool   def = false;

        if ((eq != std::string::npos) && (dd != std::string::npos))
        {
          if (eq < dd)
          {
            d = eq;
            def = true;
          }
          else
          {
            d = dd;
            def = false;
          }
        }
        else if (eq != std::string::npos)
        {
          d = eq;
          def = true;
        }
        else if (dd != std::string::npos)
        {
          d = dd;
          def = false;
        }

        if (d != std::string::npos)
        {
          std::string lhs = rld::tolower (line.substr (0, d));
          std::string rhs = line.substr (d + 1);
          if (def)
            defines[lhs] = rhs;
          else
            fields[lhs] = rhs;
        }
      }
    }

    in.close ();
  }

  bool
  package::get (const std::string& label, std::string& result)
  {
    result.erase ();

    std::string     ll = rld::tolower (label);
    table::iterator ti = fields.find (ll);

    if (ti == fields.end ())
      return false;

    /*
     * Take a copy so we can expand the macros in it.
     */
    std::string s = ti->second;

    /*
     * Loop until there is nothing more to expand.
     */
    bool expanded = true;
    while (expanded)
    {
      /*
       * Need to perform a regular expression search for '\$\{[^\}]+\}'. This
       * means look for every '${' then accept any character that is not a '}'
       * and finish with a '}'.
       */
      expanded = false;
      size_t p = 0;
      while (p < s.length ())
      {
        /*
         * Find the start and end of the label.
         */
        size_t ms = s.find ("${", p);
        if (ms != std::string::npos)
        {
          size_t me = s.find ('}', ms);
          if (me != std::string::npos)
          {
            std::string     ml = rld::tolower(s.substr (ms + 2, me - ms - 2));
            table::iterator di = defines.find (ml);
            if (di != defines.end ())
            {
              s = rld::find_replace (s, s.substr (ms, me - ms + 1),  di->second);
              expanded = true;
            }
            p = me + 1;
          }
          else
          {
            p = ms + 2;
          }
        }
        else
        {
          p = s.length ();
        }
      }
    }

    result = rld::trim (s);

    return true;
  }
}
