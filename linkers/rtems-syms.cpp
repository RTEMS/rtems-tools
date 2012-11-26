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
 * @brief RTEMS Symbols Main manages opions, sequence of operations and exceptions.
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
  { "lib-path",    required_argument,      NULL,           'L' },
  { "lib",         required_argument,      NULL,           'l' },
  { "no-stdlibs",  no_argument,            NULL,           'n' },
  { "cc",          required_argument,      NULL,           'C' },
  { "exec-prefix", required_argument,      NULL,           'E' },
  { "march",       required_argument,      NULL,           'a' },
  { "mcpu",        required_argument,      NULL,           'c' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-syms [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can be supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -w        : generate warnings (also --warn)" << std::endl
            << " -L path   : path to a library, add multiple for more than" << std::endl
            << "             one path (also --lib-path)" << std::endl
            << " -l lib    : add lib to the libraries searched, add multiple" << std::endl
            << "             for more than one library (also --lib)" << std::endl
            << " -S        : search standard libraries (also --stdlibs)" << std::endl
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
    rld::files::cache   cache;
    rld::files::paths   libpaths;
    rld::files::paths   libs;
    rld::files::paths   objects;
    rld::files::paths   libraries;
    rld::symbols::table symbols;
    std::string         base_name;
    std::string         cc_name;
    bool                standard_libs = false;
    bool                exec_prefix_set = false;
#if HAVE_WARNINGS
    bool                warnings = false;
#endif

    libpaths.push_back (".");

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvwVSE:L:l:a:c:C:", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-syms (RTEMS Symbols) " << rld::version ()
                    << std::endl;
          ::exit (0);
          break;

        case 'v':
          rld::verbose_inc ();
          break;

        case 'w':
#if HAVE_WARNINGS
          warnings = true;
#endif
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

        case 'S':
          standard_libs = true;
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

    std::cout << "RTEMS Symbols " << rld::version () << std::endl;

    /*
     * If there are no object files there is nothing to link.
     */
    if (argc == 0)
      throw rld::error ("no object files", "options");

    /*
     * Load the remaining command line arguments into the cache as object
     * files.
     */
    while (argc--)
      objects.push_back (*argv++);

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
     * Get the standard library paths
     */
    if (standard_libs)
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
     * Begin the archive session. This opens the archives and leaves them open
     * while we the symbol table is being used. The symbols reference object
     * files and the object files may reference archives and it is assumed they
     * are open and available. It is also assumed the number of library
     * archives being managed is less than the maximum file handles this
     * process can have open at any one time. If this is not the case this
     * approach would need to be reconsidered and the overhead of opening and
     * closing archives added.
     */
    try
    {
      /*
       * Load the symbol table.
       */
      cache.load_symbols (symbols);

      rld::map (cache, symbols);
    }
    catch (...)
    {
      cache.archives_end ();
      throw;
    }

    cache.archives_end ();
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
