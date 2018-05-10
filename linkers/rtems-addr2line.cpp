/*
 * Copyright (c) 2018, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS Address to Line is a version of the classic addr2line
 *        utility to test the DWARF info support in the RTEMS Toolkit.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <iostream>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include <rld.h>
#include <rld-dwarf.h>
#include <rld-files.h>
#include <rld-rtems.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

/**
 * RTEMS Symbols options.
 */
static struct option rld_opts[] = {
  { "help",         no_argument,            NULL,           'h' },
  { "version",      no_argument,            NULL,           'V' },
  { "verbose",      no_argument,            NULL,           'v' },
  { "executable",   required_argument,      NULL,           'e' },
  { "functions" ,   no_argument,            NULL,           'f' },
  { "addresses",    no_argument,            NULL,           'a' },
  { "pretty-print", no_argument,            NULL,           'p' },
  { "basenames",    no_argument,            NULL,           's' },
  { NULL,           0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-addr2line [options] addresses" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -e        : executable (also --executable)" << std::endl
            << " -f        : show function names (also --functions)" << std::endl
            << " -a        : show addresses (also --addresses)" << std::endl
            << " -p        : human readable format (also --pretty-print)" << std::endl
            << " -s        : Strip directory paths (also --basenames)" << std::endl;
  ::exit (exit_code);
}

static void
fatal_signal (int signum)
{
  signal (signum, SIG_DFL);

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

void
unhandled_exception (void)
{
  std::cerr << "error: exception handling error, please report" << std::endl;
  exit (13);
}

int
main (int argc, char* argv[])
{
  int ec = 0;

  setup_signals ();

  std::set_terminate(unhandled_exception);

  try
  {
    std::string exe_name = "a.out";
    bool        show_functions = false;
    bool        show_addresses = false;
    bool        pretty_print = false;
    bool        show_basenames = false;

    rld::set_cmdline (argc, argv);

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvVe:faps", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-addr2line (RTEMS Address To Line) " << rld::version ()
                    << ", RTEMS revision " << rld::rtems::version ()
                    << std::endl;
          ::exit (0);
          break;

        case 'v':
          rld::verbose_inc ();
          break;

        case 'e':
          exe_name = optarg;
          break;

        case 'f':
          show_functions = true;
          break;

        case 'a':
          show_addresses = true;
          break;

        case 'p':
          pretty_print = true;
          break;

        case 's':
          show_basenames = true;
          break;

        case '?':
          usage (3);
          break;

        case 'h':
          usage (0);
          break;
      }
    }

    /*
     * Set the program name.
     */
    rld::set_progname (argv[0]);

    argc -= optind;
    argv += optind;

    if (rld::verbose ())
      std::cout << "RTEMS Address To Line " << rld::version () << std::endl;

    /*
     * If there are no object files there is nothing to link.
     */
    if (argc == 0)
      throw rld::error ("no addresses provided", "options");

    if (rld::verbose ())
      std::cout << "exe: " << exe_name << std::endl;

    /*
     * Load the executable's debug info.
     */
    rld::files::object exe (exe_name);
    rld::dwarf::file   debug;

    try
    {
      /*
       * Load the executable's ELF file debug info.
       */
      exe.open ();
      exe.begin ();
      debug.begin (exe.elf ());
      debug.load_debug ();
      debug.load_types ();
      debug.load_functions ();

      for (int arg = 0; arg < argc; ++arg)
      {
        rld::dwarf::dwarf_address location;

        if (rld::verbose ())
          std::cout << "address: " << argv[arg] << std::endl;

        /*
         * Use the C routine as C++ does not have a way to automatically handle
         * different bases on the input.
         */
        location = ::strtoul (argv[arg], 0, 0);

        std::string path;
        int         line;

        debug.get_source (location, path, line);

        if (show_addresses)
        {
          std::cout << std::hex << std::setfill ('0')
                    << "0x" << location
                    << std::dec << std::setfill (' ');

          if (pretty_print)
            std::cout << ": ";
          else
            std::cout << std::endl;
        }

        if (show_functions)
        {
          std::string function;
          debug.get_function (location, function);
          std::cout << function << " at ";
        }

        if (show_basenames)
          std::cout << rld::path::basename (path);
        else
          std::cout << path;

        std::cout << ':' << line << std::endl;
      }

      debug.end ();
      exe.end ();
      exe.close ();
    }
    catch (...)
    {
      debug.end ();
      exe.end ();
      exe.close ();
      throw;
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
    rld::output_std_exception (e, std::cerr);
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
