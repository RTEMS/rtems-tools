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

#include <string.h>

#include <fstream>

#include <rld.h>
#include <rld-gcc.h>
#include <rld-process.h>

namespace rld
{
  namespace gcc
  {
    std::string exec_prefix;
    std::string march;
    std::string mcpu;
    std::string install_path;
    std::string programs_path;
    std::string libraries_path;

    /**
     * The list of standard libraries.
     */
    #define RPS RLD_PATHSTR_SEPARATOR_STR
    static const char* std_lib_c         = "libgcc.a" RPS "libssp.a" RPS "libc.a";
    static const char* std_lib_cplusplus = "libstdc++.a";

    static void
    make_cc_command (rld::process::arg_container& args)
    {
      std::string cmd = "gcc";
      if (!exec_prefix.empty ())
        cmd = exec_prefix + "-rtems" + rld::rtems_version () + '-' + cmd;
      args.push_back (cmd);
      if (!march.empty ())
        args.push_back ("-march=" + march);
      if (!mcpu.empty ())
        args.push_back ("-mcpu=" + mcpu);
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
      args.push_back ("-print-search-dirs");
      
      rld::process::tempfile out;
      rld::process::tempfile err;
      rld::process::status   status;

      status = rld::process::execute ("gcc", args, out.name (), err.name ());

      if ((status.type == rld::process::status::normal) &&
          (status.code == 0))
      {
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          out.output ("gcc", std::cout, true);
        out.open ();
        while (true)
        {
          std::string line;
          out.getline (line);
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
          std::cout << "gcc::install: " << install_path << std::endl
                    << "gcc::programs: " << programs_path << std::endl
                    << "gcc::libraries: " << libraries_path << std::endl;
        }
      }
      else
      {
        err.output ("gcc", std::cout);
      }
    }

    void
    get_library_path (std::string& name, std::string& path)
    {
      rld::process::arg_container args;

      make_cc_command (args);
      args.push_back ("-print-file-name=" + name);
      
      rld::process::tempfile out;
      rld::process::tempfile err;
      rld::process::status   status;

      status = rld::process::execute ("gcc", args, out.name (), err.name ());

      if ((status.type == rld::process::status::normal) &&
          (status.code == 0))
      {
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          out.output ("gcc", std::cout, true);
        out.open ();
        out.get (path);
        out.close ();
        if (rld::verbose () >= RLD_VERBOSE_DETAILS)
          std::cout << "gcc::libpath: " << name << " -> " << path << std::endl;
      }
      else
      {
        err.output ("gcc", std::cout);
      }
    }

    void
    get_standard_libpaths (rld::files::paths& libpaths)
    {
      search_dirs ();
      rld::split (libraries_path, libpaths, RLD_PATHSTR_SEPARATOR);
    }

    void
    get_standard_libs (rld::files::paths& libs, 
                       rld::files::paths& libpaths,
                       bool               cplusplus)
    {
      strings libnames;

      rld::split (std_lib_c, libnames, RLD_PATHSTR_SEPARATOR);
      if (cplusplus)
        rld::files::path_split (std_lib_cplusplus, libnames);

      for (strings::iterator lni = libnames.begin ();
           lni != libnames.end ();
           ++lni)
      {
        if (rld::verbose () >= RLD_VERBOSE_INFO)
          std::cout << "gcc::stdlib: " << *lni << std::endl;

        std::string path;

        rld::files::find_file (path, *lni, libpaths);
        if (path.empty ())
          throw rld::error ("Library not found: " + *lni, "getting standard libs");

        libs.push_back (path);
      }
    }
  }
}
