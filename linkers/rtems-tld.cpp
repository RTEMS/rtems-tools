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
 * @brief RTEMS Trace Linker manages creating a tracable RTEMS executable.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>

#include <cxxabi.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include <rld.h>
#include <rld-cc.h>
#include <rld-config.h>
#include <rld-process.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

namespace rld
{

  /**
   * Trim from start.
   */
  inline std::string &ltrim (std::string &s)
  {
    s.erase (s.begin (),
            std::find_if (s.begin (), s.end (),
                         std::not1 (std::ptr_fun < int, int > (std::isspace))));
    return s;
  }

  /**
   * Trim from end.
   */
  inline std::string &rtrim (std::string &s)
  {
    s.erase (std::find_if (s.rbegin (), s.rend (),
                           std::not1 (std::ptr_fun < int, int > (std::isspace))).base(),
             s.end());
    return s;
  }

  /**
   * Trim from both ends.
   */
  inline std::string &trim (std::string &s)
  {
    return ltrim (rtrim (s));
  }
}

/**
 * RTEMS Trace Linker.
 */
namespace trace
{
  /**
   * A container of arguments.
   */
  typedef std::vector < std::string > function_args;

  /**
   * The return value.
   */
  typedef std::string function_return;

  /**
   * A function's signature.
   */
  struct function_sig
  {
    std::string     name; /**< The function's name. */
    function_args   args; /**< The function's list of arguments. */
    function_return ret;  /**< The fuctions return value. */
  };

  /**
   * A container of function signatures.
   */
  typedef std::map < std::string, function_sig > function_sigs;

  /**
   * Trace Linker.
   */
  class linker
  {
  public:
    linker ();

    /**
     * Load the user's configuration.
     */
    void load_config (const std::string& path);

    /**
     * Dump the linker status.
     */
    void dump (std::ostream& out);

  private:

    rld::config::config config; /**< User configuration. */
    function_sigs       sigs;   /**< Function signatures. */
  };

  linker::linker ()
  {
  }

  void
  linker::load_config (const std::string& path)
  {
    config.clear ();
    config.load (path);

    /*
     * The configuration must contain a "trace" section. This is the top level
     * configuration and must the following fields:
     *
     *   # < add here >
     *
     * The 'trace" section may optionally contain a number of 'include' records
     * and these configuration files are included.
     */

    const rld::config::section& tsec = config.get_section ("trace");
    bool                        have_includes = false;

    try
    {
      const rld::config::record& irec = tsec.get_record ("include");

      have_includes = true;

      /*
       * Include records are a path which we can just load.
       *
       * @todo Add a search path. See 'rld::files' for details. We can default
       *       the search path to the install $prefix of this tool and we can
       *       then provide a default set of function signatures for RTEMS
       *       APIs.
       */

      for (rld::config::items::const_iterator ii = irec.items.begin ();
           ii != irec.items.end ();
           ++ii)
      {
        config.load ((*ii).text);
      }
    }
    catch (rld::error re)
    {
      /*
       * No include records, must be all inlined. If we have includes it must
       * be another error so throw it.
       */
      if (have_includes)
        throw;
    }

    /*
     * Get the function signatures from the configuration and load them into
     * the sig map.
     */

    const rld::config::section& fssec = config.get_section ("function-signatures");

    for (rld::config::records::const_iterator ri = fssec.recs.begin ();
         ri != fssec.recs.end ();
         ++ri)
    {
      /*
       * There can only be one function signature in the configuration.
       */
      if ((*ri).items.size () > 1)
        throw rld::error ("duplicate", "function signature: " + (*ri).name);

      function_sig sig;
      sig.name = (*ri).name;

      /*
       * Function signatures are defined as the return value then the arguments
       * delimited by a comma and white space. No checking is made of the
       * return value or arguments.
       */
      rld::config::items::const_iterator ii = (*ri).items.begin ();
      std::stringstream                  ts((*ii).text);
      std::string                        arg;
      while (std::getline (ts, arg, ','))
      {
        rld::trim (arg);
        if (!arg.empty ())
        {
          if (sig.ret.empty ())
            sig.ret = arg;
          else
            sig.args.push_back(arg);
        }
      }

      if (sig.ret.empty ())
        throw rld::error ("no return value", "function signature: " + (*ri).name);

      if (sig.args.empty ())
        throw rld::error ("no arguments", "function signature: " + (*ri).name);

      sigs[sig.name] = sig;
    }
  }

  void
  linker::dump (std::ostream& out)
  {
    const rld::config::paths& cpaths = config.get_paths ();
    out << "Configuration Files: " << cpaths.size () << std::endl;
    for (rld::config::paths::const_iterator pi = cpaths.begin ();
         pi != cpaths.end ();
         ++pi)
    {
      out << " " << (*pi) << std::endl;
    }

    out << std::endl
        << "Function Signatures: " << sigs.size () << std::endl;
    for (function_sigs::const_iterator si = sigs.begin ();
         si != sigs.end ();
         ++si)
    {
      const function_sig& sig = (*si).second;
      out << " " << sig.name << ": " << sig.ret << ' ' << sig.name << '(';
      for (function_args::const_iterator fai = sig.args.begin ();
           fai != sig.args.end ();
           ++fai)
      {
        if (fai != sig.args.begin ())
          out << ", ";
        out << (*fai);
      }
      out << ");" << std::endl;
    }
  }
}

/**
 * RTEMS Trace Linker options. This needs to be rewritten to be like cc where
 * only a single '-' and long options is present. Anything after '--' is passed
 * to RTEMS's real linker.
 */
static struct option rld_opts[] = {
  { "help",        no_argument,            NULL,           'h' },
  { "version",     no_argument,            NULL,           'V' },
  { "verbose",     no_argument,            NULL,           'v' },
  { "warn",        no_argument,            NULL,           'w' },
  { "exec-prefix", required_argument,      NULL,           'E' },
  { "march",       required_argument,      NULL,           'a' },
  { "mcpu",        required_argument,      NULL,           'c' },
  { "config",      required_argument,      NULL,           'C' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-trace-ld [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -w        : generate warnings (also --warn)" << std::endl
            << " -E prefix : the RTEMS tool prefix (also --exec-prefix)" << std::endl
            << " -a march  : machine architecture (also --march)" << std::endl
            << " -c cpu    : machine architecture's CPU (also --mcpu)" << std::endl
            << " -C ini    : user configuration INI file (also --config)" << std::endl;
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
    trace::linker linker;
    std::string   ld_cmd;
    std::string   configuration;
    bool          exec_prefix_set = false;
#if HAVE_WARNINGS
    bool          warnings = false;
#endif

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvwVE:a:c:C:", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-trace-ld (RTEMS Trace Linker) " << rld::version ()
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

        case 'C':
          configuration = optarg;
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

    if (rld::verbose ())
      std::cout << "RTEMS Trace Linker " << rld::version () << std::endl;

    /*
     * Load the remaining command line arguments into the linker command line.
     */
    while (argc--)
    {
      if (ld_cmd.length () != 0)
        ld_cmd += " ";
      ld_cmd += *argv++;
    }

    /*
     * If there are no object files there is nothing to link.
     */
    if (ld_cmd.empty ())
      throw rld::error ("no trace linker options", "options");

    /*
     * Perform a trace link.
     */
    try
    {
      linker.load_config (configuration);
      linker.dump (std::cout);
    }
    catch (...)
    {
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
