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

#include <string.h>

#include <fstream>

#include <rld.h>
#include <rld-cc.h>
#include <rld-process.h>

namespace rld
{
  namespace cc
  {
    std::string cc;
    std::string cc_name = "gcc";
    std::string exec_prefix;
    std::string cppflags;
    std::string cflags;
    std::string cxxflags;
    std::string ldflags;
    std::string install_path;
    std::string programs_path;
    std::string libraries_path;

    /**
     * The list of standard libraries.
     */
    #define RPS RLD_PATHSTR_SEPARATOR_STR
    static const char* std_lib_c         = "libgcc.a" RPS "libssp.a" RPS "libc.a";
    static const char* std_lib_cplusplus = "libstdc++.a";

    void
    make_cc_command (rld::process::arg_container& args)
    {
      /*
       * Use the absolute path to CC if provided.
       */
      if (!cc.empty ())
        args.push_back (cc);
      else
      {
        std::string cmd = cc_name;
        if (!exec_prefix.empty ())
          cmd = exec_prefix + "-rtems" + rld::rtems_version () + '-' + cmd;
        args.push_back (cmd);
      }
    }

    void
    add_cppflags (rld::process::arg_container& args)
    {
      if (!cppflags.empty ())
        args.push_back (cppflags);
    }

    void
    add_cflags (rld::process::arg_container& args)
    {
      if (!cflags.empty ())
        args.push_back (cflags);
    }

    void
    add_cxxflags (rld::process::arg_container& args)
    {
      if (!cxxflags.empty ())
        args.push_back (cxxflags);
    }

    void
    add_ldflags (rld::process::arg_container& args)
    {
      if (!ldflags.empty ())
        args.push_back (ldflags);
    }

    static bool
    match_and_trim (const char* prefix, std::string& line, std::string& result)
    {
      std::string::size_type pos = ::strlen (prefix);
      if (line.substr (0, pos) == prefix)
      {
        if (line[pos] == '=')
          ++pos;
        result = line.substr (pos, line.size () - pos - 1);
        return true;
      }
      return false;
    }

    static void
    search_dirs ()
    {
      rld::process::arg_container args;

      make_cc_command (args);
      add_cppflags (args);
      add_cflags (args);
      args.push_back ("-print-search-dirs");

      rld::process::tempfile out;
      rld::process::tempfile err;
      rld::process::status   status;

      status = rld::process::execute (cc_name, args, out.name (), err.name ());

      if ((status.type == rld::process::status::normal) &&
          (status.code == 0))
      {
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          out.output (cc_name, std::cout, true);
        out.open ();
        while (true)
        {
          std::string line;
          out.read_line (line);
          if (line.size () == 0)
            break;
          if (match_and_trim ("install: ", line, install_path))
            continue;
          if (match_and_trim ("programs: ", line, programs_path))
            continue;
          if (match_and_trim ("libraries: ", line, libraries_path))
            continue;
        }
        out.close ();
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
        {
          std::cout << "cc::install: " << install_path << std::endl
                    << "cc::programs: " << programs_path << std::endl
                    << "cc::libraries: " << libraries_path << std::endl;
        }
      }
      else
      {
        err.output (cc_name, std::cout);
      }
    }

    void
    get_library_path (std::string& name, std::string& path)
    {
      rld::process::arg_container args;

      make_cc_command (args);
      add_cflags (args);
      add_ldflags (args);
      args.push_back ("-print-file-name=" + name);

      rld::process::tempfile out;
      rld::process::tempfile err;
      rld::process::status   status;

      status = rld::process::execute (cc_name, args, out.name (), err.name ());

      if ((status.type == rld::process::status::normal) &&
          (status.code == 0))
      {
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          out.output ("cc", std::cout, true);
        out.open ();
        out.read (path);
        out.close ();
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          std::cout << "cc::libpath: " << name << " -> " << path << std::endl;
      }
      else
      {
        err.output ("cc", std::cout);
      }
    }

    void
    get_standard_libpaths (rld::path::paths& libpaths)
    {
      search_dirs ();
      rld::split (libraries_path, libpaths, RLD_PATHSTR_SEPARATOR);
    }

    void
    get_standard_libs (rld::path::paths& libs,
                       rld::path::paths& libpaths,
                       bool              cplusplus)
    {
      strings libnames;

      rld::split (std_lib_c, libnames, RLD_PATHSTR_SEPARATOR);
      if (cplusplus)
        rld::path::path_split (std_lib_cplusplus, libnames);

      for (strings::iterator lni = libnames.begin ();
           lni != libnames.end ();
           ++lni)
      {
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << "cc::stdlib: " << *lni << std::endl;

        std::string path;

        rld::path::find_file (path, *lni, libpaths);
        if (path.empty ())
          throw rld::error ("Library not found: " + *lni, "getting standard libs");

        libs.push_back (path);
      }
    }
  }
}
