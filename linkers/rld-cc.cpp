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
    std::string warning_cflags;
    std::string include_cflags;
    std::string machine_cflags;
    std::string spec_cflags;
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

    const std::string
    strip_cflags (const std::string& flags)
    {
      std::string  oflags;
      rld::strings flags_;
      rld::split (flags_, flags);

      for (rld::strings::iterator si = flags_.begin ();
           si != flags_.end ();
           ++si)
      {
        if (!rld::starts_with ((*si), "-O") && !rld::starts_with ((*si), "-g"))
          oflags += ' ' + *si;
      }

      return rld::trim (oflags);
    }

    const std::string
    filter_flags (const std::string& flags,
                  const std::string& ,
                  const std::string& ,
                  flag_type          type,
                  std::string&       warnings,
                  std::string&       includes,
                  std::string&       machines,
                  std::string&       specs)
    {
      /*
       * Defintion of flags to be filtered.
       */
      enum flag_group
      {
        fg_warning,
        fg_include,
        fg_machine,
        fg_specs
      };
      struct flag_def
      {
        flag_group  group;  ///< The group this flag belong to.
        const char* opt;    ///< Option start.
        int         count;  ///< Number of arguments with the option.
        bool        path;   ///< Is this a path ?
        int         out;    ///< If the flag type is set drop the opt..
      };
      const flag_def flag_defs[] =
        {
          { fg_warning, "-W",       1, false, ft_cppflags | ft_cflags | ft_ldflags },
          { fg_include, "-I",       2, true,  0 },
          { fg_include, "-isystem", 2, true,  0 },
          { fg_include, "-sysroot", 2, true,  0 },
          { fg_machine, "-O",       1, false, 0 },
          { fg_machine, "-m",       1, false, 0 },
          { fg_machine, "-f",       1, false, 0 },
          { fg_specs,   "-q",       1, false, 0 },
          { fg_specs,   "-B",       2, true,  0 },
          { fg_specs,   "--specs",  2, false, 0 }
        };
      const int flag_def_size = sizeof (flag_defs) / sizeof (flag_def);

      std::string  oflags;
      rld::strings flags_;

      rld::split (flags_, strip_cflags (flags));

      warnings.clear ();
      includes.clear ();
      machines.clear ();
      specs.clear ();

      for (rld::strings::iterator si = flags_.begin ();
           si != flags_.end ();
           ++si)
      {
        std::string  opts;
        std::string& opt = *(si);
        bool         in = true;

        for (int fd = 0; fd < flag_def_size; ++fd)
        {
          if (rld::starts_with (opt, flag_defs[fd].opt))
          {
            int opt_count = flag_defs[fd].count;
            if (opt_count > 1)
            {
              /*
               * See if the flag is just the option. If is not take one less
               * because the option's argument is joined to the option.
               */
              if (opt != flag_defs[fd].opt)
              {
                opt_count -= 1;
                /*
                 * @todo Path processing here. Not sure what it is needed for.
                 */
              }
            }
            opts += ' ' + opt;
            while (opt_count > 1)
            {
              ++si;
              /*
               * @todo Path processing here. Not sure what it is needed for.
               */
              opts += ' ' + (*si);
              --opt_count;
            }
            switch (flag_defs[fd].group)
            {
              case fg_warning:
                warnings += ' ' + opts;
                break;
              case fg_include:
                includes += ' ' + opts;
                break;
              case fg_machine:
                machines += ' ' + opts;
                break;
              case fg_specs:
                specs += ' ' + opts;
                break;
              default:
                throw rld::error ("Invalid group", "flag group");
            }
            if ((flag_defs[fd].out & type) != 0)
              in = false;
            break;
          }
        }

        if (in)
          oflags += ' ' + opts;
      }

      rld::trim (warnings);
      rld::trim (includes);
      rld::trim (machines);
      rld::trim (specs);

      return rld::trim (oflags);
    }

    const std::string
    filter_flags (const std::string& flags,
                  const std::string& arch,
                  const std::string& path,
                  flag_type          type)
    {
      if (type != ft_cflags)
      {
        std::string warnings;
        std::string includes;
        std::string machines;
        std::string specs;
        return filter_flags (flags,
                             arch,
                             path,
                             type,
                             warnings,
                             includes,
                             machines,
                             specs);
      }
      else
      {
        return filter_flags (flags,
                             arch,
                             path,
                             type,
                             warning_cflags,
                             include_cflags,
                             machine_cflags,
                             spec_cflags);
      }
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
