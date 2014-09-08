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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <unistd.h>

#include <rld.h>

namespace rld
{
  namespace path
  {
    const std::string
    basename (const std::string& name)
    {
      size_t b = name.find_last_of (RLD_PATH_SEPARATOR);
      if (b != std::string::npos)
        return name.substr (b + 1);
      return name;
    }

    const std::string
    dirname (const std::string& name)
    {
      size_t b = name.find_last_of (RLD_PATH_SEPARATOR);
      if (b != std::string::npos)
        return name.substr (0, b);
      return name;
    }

    const std::string
    extension (const std::string& name)
    {
      size_t b = name.find_last_of ('.');
      if (b != std::string::npos)
        return name.substr (b);
      return name;
    }

    void
    path_split (const std::string& path, paths& paths)
    {
      strings ps;
      rld::split (ps, path, RLD_PATHSTR_SEPARATOR);
      if (ps.size ())
      {
        for (strings::iterator psi = ps.begin ();
             psi != ps.end ();
             ++psi)
        {
          if (check_directory (*psi))
            paths.push_back (*psi);
        }
      }
    }

    void
    path_join (const std::string& base, const std::string& part, std::string& joined)
    {
      if ((base[base.size () - 1] != RLD_PATH_SEPARATOR) &&
          (part[0] != RLD_PATH_SEPARATOR))
        joined = base + RLD_PATH_SEPARATOR + part;
      else if ((base[base.size () - 1] == RLD_PATH_SEPARATOR) &&
               (part[0] == RLD_PATH_SEPARATOR))
        joined = base + &part[1];
      else
        joined = base + part;
    }

    void
    path_join (const std::string& base, const paths& parts, std::string& joined)
    {
      joined = base;
      for (paths::const_iterator pi = parts.begin ();
           pi != parts.end ();
           ++pi)
      {
        path_join (joined, *pi, joined);
      }
    }

    const std::string
    path_abs (const std::string& path)
    {
      std::string apath;

      if (path[0] == RLD_PATH_SEPARATOR)
      {
        apath = path;
      }
      else
      {
        char* buf = 0;
        try
        {
          buf = new char[32 * 1024];
          if (!::getcwd (buf, 132 * 1024))
            throw rld::error (::strerror (errno), "get current working directory");
          path_join (buf, path, apath);
        }
        catch (...)
        {
          delete [] buf;
          throw;
        }
      }

      strings ps;
      strings aps;

      rld::split (ps, apath, RLD_PATH_SEPARATOR);

      for (strings::iterator psi = ps.begin ();
           psi != ps.end ();
           ++psi)
      {
        const std::string& dir = *psi;

        if (dir.empty () || dir == ".")
        {
          /* do nothing */
        }
        else if (dir == "..")
        {
          aps.pop_back ();
        }
        else
        {
          aps.push_back (dir);
        }
      }

      return RLD_PATH_SEPARATOR + rld::join (aps, RLD_PATH_SEPARATOR_STR);
    }

    bool
    check_file (const std::string& path)
    {
      struct stat sb;
      if (::stat (path.c_str (), &sb) == 0)
        if (S_ISREG (sb.st_mode))
          return true;
      return false;
    }

    bool
    check_directory (const std::string& path)
    {
      struct stat sb;
      if (::stat (path.c_str (), &sb) == 0)
        if (S_ISDIR (sb.st_mode))
          return true;
      return false;
    }

    void
    find_file (std::string& path, const std::string& name, paths& search_paths)
    {
      for (paths::iterator pi = search_paths.begin ();
           pi != search_paths.end ();
           ++pi)
      {
        path_join (*pi, name, path);
        if (check_file (path))
          return;
      }
      path.clear ();
    }

    void
    unlink (const std::string& path, bool not_present_error)
    {
      struct stat sb;
      if (::stat (path.c_str (), &sb) >= 0)
      {
        if (!S_ISREG (sb.st_mode))
            throw rld::error ("Not a regular file", "unlinking: " + path);

        int r;
#if _WIN32
        r = ::remove(path.c_str ());
#else
        r = ::unlink (path.c_str ());
#endif
        if (r < 0)
          throw rld::error (::strerror (errno), "unlinking: " + path);
      }
      else
      {
        if (not_present_error)
          throw rld::error ("Not found", "unlinking: " + path);
      }
    }
  }
}
