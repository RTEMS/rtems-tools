/*
 * Copyright (c) 2011-2012, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS Linker Main manages opions, sequence of operations and exceptions.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

#include <cxxabi.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include <rld.h>
#include <rld-cc.h>
#include <rld-outputter.h>
#include <rld-process.h>
#include <rld-resolver.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

/**
 * RTEMS Linker options. This needs to be rewritten to be like cc where only a
 * single '-' and long options is present.
 */
static struct option rld_opts[] = {
  { "help",        no_argument,            NULL,           'h' },
  { "version",     no_argument,            NULL,           'V' },
  { "verbose",     no_argument,            NULL,           'v' },
  { "warn",        no_argument,            NULL,           'w' },
  { "map",         no_argument,            NULL,           'M' },
  { "output",      required_argument,      NULL,           'o' },
  { "script",      no_argument,            NULL,           'S' },
  { "lib-path",    required_argument,      NULL,           'L' },
  { "lib",         required_argument,      NULL,           'l' },
  { "no-stdlibs",  no_argument,            NULL,           'n' },
  { "entry",       required_argument,      NULL,           'e' },
  { "define",      required_argument,      NULL,           'd' },
  { "undefined",   required_argument,      NULL,           'u' },
  { "base",        required_argument,      NULL,           'b' },
  { "cc",          required_argument,      NULL,           'C' },
  { "exec-prefix", required_argument,      NULL,           'E' },
  { "march",       required_argument,      NULL,           'a' },
  { "mcpu",        required_argument,      NULL,           'c' },
  { NULL,          0,                      NULL,            0 }
};

#if TO_BE_USED_FOR_THE_UNDEFINES
void
split_on_equals (const std::string& opt, std::string& left, std::string& right)
{
  std::string::size_type eq = opt.find_first_of('=');
}
#endif

void
usage (int exit_code)
{
  std::cout << "rtems-ld [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can be supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -w        : generate warnings (also --warn)" << std::endl
            << " -M        : generate map output (also --map)" << std::endl
            << " -o file   : linker output is written to file (also --output)" << std::endl
            << " -S        : linker output is a script file (also --script)" << std::endl
            << " -L path   : path to a library, add multiple for more than" << std::endl
            << "             one path (also --lib-path)" << std::endl
            << " -l lib    : add lib to the libraries searched, add multiple" << std::endl
            << "             for more than one library (also --lib)" << std::endl
            << " -n        : do not search standard libraries (also --no-stdlibs)" << std::endl
            << " -e entry  : entry point symbol (also --entry)" << std::endl
            << " -d sym    : add the symbol definition, add multiple with" << std::endl
            << "             more than one define (also --define)" << std::endl
            << " -u sym    : add the undefined symbol definition, add multiple" << std::endl
            << "             for more than one undefined symbol (also --undefined)" << std::endl
            << " -b elf    : read the ELF file symbols as the base RTEMS kernel" << std::endl
            << "             image (also --base)" << std::endl
            << " -C file   : execute file as the target C compiler (also --cc)" << std::endl
            << " -E prefix : the RTEMS tool prefix (also --exec-prefix)" << std::endl
            << " -a march  : machine architecture (also --march)" << std::endl
            << " -c cpu    : machine architecture's CPU (also --mcpu)" << std::endl;
  ::exit (exit_code);
}

static void
fatal_signal (int signum)
{
  signal (signum, SIG_DFL);

  rld::process::temporaries.clean_up ();

  /*
   * Get the same signal again, this time not handled, so its normal effect
   * occurs.
   */
  kill (getpid (), signum);
}

static void
setup_signals (void)
{
  if (signal (SIGINT, SIG_IGN) != SIG_IGN)
    signal (SIGINT, fatal_signal);
#ifdef SIGHUP
  if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
    signal (SIGHUP, fatal_signal);
#endif
  if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
    signal (SIGTERM, fatal_signal);
#ifdef SIGPIPE
  if (signal (SIGPIPE, SIG_IGN) != SIG_IGN)
    signal (SIGPIPE, fatal_signal);
#endif
#ifdef SIGCHLD
  signal (SIGCHLD, SIG_DFL);
#endif
}

int
main (int argc, char* argv[])
{
  int ec = 0;

  setup_signals ();

  try
  {
    rld::files::cache    cache;
    rld::files::cache    base;
    rld::files::paths    libpaths;
    rld::files::paths    libs;
    rld::files::paths    objects;
    rld::files::paths    libraries;
    rld::symbols::bucket defines;
    rld::symbols::bucket undefines;
    rld::symbols::table  base_symbols;
    rld::symbols::table  symbols;
    rld::symbols::table  undefined;
    std::string          entry;
    std::string          output = "a.out";
    std::string          base_name;
    std::string          cc_name;
    bool                 script = false;
    bool                 standard_libs = true;
    bool                 exec_prefix_set = false;
    bool                 map = false;
    bool                 warnings = false;

    libpaths.push_back (".");

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvwVMnSb:E:o:L:l:a:c:e:d:u:C:", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-ld (RTEMS Linker) " << rld::version ()
                    << std::endl;
          ::exit (0);
          break;

        case 'v':
          rld::verbose_inc ();
          break;

        case 'M':
          map = true;
          break;

        case 'w':
          warnings = true;
          break;

        case 'o':
          if (output != "a.out")
            std::cerr << "warning: output already set" << std::endl;
          output = optarg;
          break;

        case 'S':
          script = true;
          break;

        case 'l':
          /*
           * The order is important. It is the search order.
           */
          libs.push_back (optarg);
          break;

        case 'L':
          if ((optarg[::strlen (optarg) - 1] == '/') ||
              (optarg[::strlen (optarg) - 1] == '\\'))
            optarg[::strlen (optarg) - 1] = '\0';
          libpaths.push_back (optarg);
          break;

        case 'n':
          standard_libs = false;
          break;

        case 'C':
          if (exec_prefix_set == true)
            std::cerr << "warning: exec-prefix ignored when CC provided" << std::endl;
          rld::cc::cc = optarg;
          break;

        case 'E':
          exec_prefix_set = true;
          rld::cc::exec_prefix = optarg;
          break;

        case 'a':
          rld::cc::march = optarg;
          break;

        case 'c':
          rld::cc::mcpu = optarg;
          break;

        case 'e':
          entry = optarg;
          break;

        case 'd':
          defines.push_back (rld::symbols::symbol (optarg));
          break;

        case 'u':
          undefines.push_back (rld::symbols::symbol (optarg));
          break;

        case 'b':
          base_name = optarg;
          break;

        case '?':
          usage (3);
          break;

        case 'h':
          usage (0);
          break;
      }
    }

    argc -= optind;
    argv += optind;

    if (rld::verbose () || map)
      std::cout << "RTEMS Linker " << rld::version () << std::endl;

    /*
     * If there are no object files there is nothing to link.
     */
    if ((argc == 0) && !map)
      throw rld::error ("no object files", "options");

    /*
     * Load the remaining command line arguments into the cache as object
     * files.
     */
    while (argc--)
      objects.push_back (*argv++);

    /*
     * Load the symbol table with the defined symbols from the defines bucket.
     */
    rld::symbols::load (defines, symbols);

    /*
     * Load the undefined table with the undefined symbols from the undefines
     * bucket.
     */
    rld::symbols::load (undefines, undefined);

    /*
     * Add the object files to the cache.
     */
    cache.add (objects);

    /*
     * Open the cache.
     */
    cache.open ();

    /*
     * If the full path to CC is not provided and the exec-prefix is not set by
     * the command line see if it can be detected from the object file
     * types. This must be after we have added the object files because they
     * are used when detecting.
     */
    if (rld::cc::cc.empty () && !exec_prefix_set)
      rld::cc::exec_prefix = rld::elf::machine_type ();

    /*
     * If we have a base image add it.
     */
    if (base_name.length ())
    {
      base.open ();
      base.add (base_name);
      base.load_symbols (base_symbols, true);
    }

    /*
     * Get the standard library paths
     */
    rld::cc::get_standard_libpaths (libpaths);

    /*
     * Get the command line libraries.
     */
    rld::files::find_libraries (libraries, libpaths, libs);

    /*
     * Are we to load standard libraries ?
     */
    if (standard_libs)
      rld::cc::get_standard_libs (libraries, libpaths);

    /*
     * Load the library to the cache.
     */
    cache.add_libraries (libraries);

    /*
     * Load the symbol table.
     */
    cache.load_symbols (symbols);

    /*
     * Map ?
     */
    if (map)
    {
      if (base_name.length ())
        rld::map (base, base_symbols);
      rld::map (cache, symbols);
    }

    if (cache.path_count ())
    {
      /*
       * This structure allows us to add different operations with the same
       * structure.
       */
      rld::files::object_list dependents;
      rld::resolver::resolve (dependents, cache, base_symbols, symbols, undefined);

      /**
       * Output the file.
       */
      if (script)
        rld::outputter::script (output, dependents, cache);
      else
        rld::outputter::archive (output, dependents, cache);

      /**
       * Check for warnings.
       */
      if (warnings)
      {
        rld::warn_unused_externals (dependents);
      }
    }
  }
  catch (rld::error re)
  {
    std::cerr << "error: "
              << re.where << ": " << re.what
              << std::endl;
    ec = 10;
  }
  catch (std::exception e)
  {
    int   status;
    char* realname;
    realname = abi::__cxa_demangle (e.what(), 0, 0, &status);
    std::cerr << "error: exception: " << realname << " [";
    ::free (realname);
    const std::type_info &ti = typeid (e);
    realname = abi::__cxa_demangle (ti.name(), 0, 0, &status);
    std::cerr << realname << "] " << e.what () << std::endl;
    ::free (realname);
    ec = 11;
  }
  catch (...)
  {
    /*
     * Helps to know if this happens.
     */
    std::cout << "error: unhandled exception" << std::endl;
    ec = 12;
  }

  return ec;
}
