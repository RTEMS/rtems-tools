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

      /**
       * The default constructor.
       */
      function_sig ();

      /**
       * Construct the signature loading it from the configuration.
       */
      function_sig (const rld::config::record& record);

      /**
       * Copy constructor.
       */
      function_sig (const function_sig& orig);

      /**
       * Return the function's declaration.
       */
      const std::string decl () const;
    };

    /**
     * A container of function signatures.
     */
    typedef std::map < std::string, function_sig > function_sigs;

    /**
     * Wrappers hold the data used when wrapping the code. It knows how to wrap
     * a specific trace function. Wrapping a function requires specific defines
     * and header files.
     */
    struct wrapper
    {
      std::string   name;     /**< The name of this wrapper. */
      rld::strings  headers;  /**< Include statements. */
      rld::strings  defines;  /**< Define statements. */
      function_sigs sigs;     /**< The functions this wrapper wraps. */

      /**
       * Load the wrapper.
       */
      wrapper (const std::string&   name,
               rld::config::config& config);

      /**
       * Recursive parser for strings.
       */
      void parse (rld::config::config&        config,
                  const rld::config::section& section,
                  const std::string&          sec_name,
                  const std::string&          rec_name,
                  rld::strings&               items,
                  int                         depth = 0);

      /**
       * Dump the wrapper.
       */
      void dump (std::ostream& out) const;
    };

    /**
     * A container of wrappers. The order is the order we wrap.
     */
    typedef std::vector < wrapper > wrappers;

    /**
     * Tracer.
     */
    class tracer
    {
    public:
      tracer ();

      /**
       * Load the user's configuration.
       */
      void load (rld::config::config& config,
                 const std::string&   section);

      /**
       * Dump the wrapper.
       */
      void dump (std::ostream& out) const;

    private:

      std::string  name;     /**< The name of the trace. */
      std::string  bsp;      /**< The BSP we are linking to. */
      rld::strings trace;    /**< The functions to trace. */
      wrappers     wrappers; /**< Wrappers wrap trace functions. */
    };

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
      void load_config (const std::string& path,
                        const std::string& trace);

      /**
       * Generate the C file.
       */
      void generate_c ();

      /**
       * Dump the linker.
       */
      void dump (std::ostream& out) const;

    private:

      rld::config::config config;   /**< User configuration. */
      tracer              tracer;   /**< The tracer */
    };

    function_sig::function_sig ()
    {
    }

    function_sig::function_sig (const rld::config::record& record)
    {
      /*
       * There can only be one function signature in the configuration.
       */
      if (!record.single ())
        throw rld::error ("duplicate", "function signature: " + record.name);

      name = record.name;

      /*
       * Function signatures are defined as the return value then the arguments
       * delimited by a comma and white space. No checking is made of the
       * return value or arguments.
       */
      rld::strings si;
      rld::config::parse_items (record, si);

      if (si.size () == 0)
        throw rld::error ("no return value", "function signature: " + record.name);
      if (si.size () == 1)
          throw rld::error ("no arguments", "function signature: " + record.name);

      ret = si[0];
      args.resize (si.size () - 1);
      std::copy (si.begin ()  + 1, si.end (), args.begin ());
    }

    function_sig::function_sig (const function_sig& orig)
      : name (orig.name),
        args (orig.args),
        ret (orig.ret)
    {
    }

    const std::string
    function_sig::decl () const
    {
      std::string ds = ret + ' ' + name + '(';
      int         arg = 0;
      for (function_args::const_iterator ai = args.begin ();
           ai != args.end ();
           ++ai)
        {
          if (ai != args.begin ())
            ds += ", ";
          ds += (*ai) + " a" + rld::to_string (++arg);
        }
      ds += ')';

      return ds;
    }

    wrapper::wrapper (const std::string&   name,
                      rld::config::config& config)
      : name (name)
    {
      /*
       * A wrapper section optionally contain one or more records of:
       *
       * # header    A list of include string that are single or double quoted.
       * # define    A list of define string that are single or double quoted.
       * # signature A list of section names of function signatures.
       *
       * @note The quoting and list spliting is a little weak because a delimiter
       *       in a quote should not be seen as a delimiter.
       */
      const rld::config::section& section = config.get_section (name);

      parse (config, section, "headers", "header", headers);
      parse (config, section, "defines", "define", defines);

      rld::strings sig_list;

      rld::config::parse_items (section, "signatures", sig_list);

      for (rld::strings::const_iterator sli = sig_list.begin ();
           sli != sig_list.end ();
           ++sli)
      {
        const rld::config::section& sig_sec = config.get_section (*sli);
        for (rld::config::records::const_iterator ri = sig_sec.recs.begin ();
             ri != sig_sec.recs.end ();
             ++ri)
        {
          function_sig func (*ri);
          sigs[func.name] = func;
        }
      }
    }

    void
    wrapper::parse (rld::config::config&        config,
                    const rld::config::section& section,
                    const std::string&          sec_name,
                    const std::string&          rec_name,
                    rld::strings&               items,
                    int                         depth)
    {
      if (depth > 32)
        throw rld::error ("too deep", "parsing: " + sec_name + '/' + rec_name);

      rld::config::parse_items (section, rec_name, items);

      rld::strings sl;

      rld::config::parse_items (section, sec_name, sl);

      for (rld::strings::const_iterator sli = sl.begin ();
           sli != sl.end ();
           ++sli)
      {
        const rld::config::section& sec = config.get_section (*sli);
        parse (config, sec, sec_name, rec_name, items, depth + 1);
      }
    }

    void
    wrapper::dump (std::ostream& out) const
    {
      out << "  Wrapper: " << name << std::endl
          << "   Headers: " << headers.size () << std::endl;
      for (rld::strings::const_iterator hi = headers.begin ();
           hi != headers.end ();
           ++hi)
      {
        out << "    " << (*hi) << std::endl;
      }
      out << "   Defines: " << defines.size () << std::endl;
      for (rld::strings::const_iterator di = defines.begin ();
           di != defines.end ();
           ++di)
      {
        out << "    " << (*di) << std::endl;
      }
      out << "   Function Signatures: " << sigs.size () << std::endl;
      for (function_sigs::const_iterator si = sigs.begin ();
           si != sigs.end ();
           ++si)
      {
        const function_sig& sig = (*si).second;
        out << "    " << sig.name << ": " << sig.decl () << ';' << std::endl;
      }
    }

    tracer::tracer ()
    {
    }

    void
    tracer::load (rld::config::config& config,
                  const std::string&   section)
    {
      /*
       * The configuration must contain a "trace" section. This is the top level
       * configuration and must the following fields:
       *
       *  # name    The name of trace being linked.
       *  # trace   The list of sections containing functions to trace.
       *  # wrapper The list of sections containing wrapping details.
       *
       * The following record are optional:
       *
       *  # bdp     The BSP the executable is for. Can be supplied on the command
       *            line.
       *  # include Include the INI file.
       *
       * The following will throw an error is the section or records are not
       * found.
       */
      rld::strings ss;

      const rld::config::section& tsec = config.get_section (section);
      const rld::config::record&  nrec = tsec.get_record ("name");
      const rld::config::record&  brec = tsec.get_record ("bsp");
      const rld::config::record&  trec = tsec.get_record ("trace");
      const rld::config::record&  wrec = tsec.get_record ("wrapper");

      if (!nrec.single ())
        throw rld::error ("duplicate", "trace names");
      name = nrec.items[0].text;

      if (!brec.single ())
        throw rld::error ("duplicate", "trace bsp");
      bsp = brec.items[0].text;

      /*
       * Include any files.
       */
      config.includes (tsec);

      /*
       * Load the wrappers.
       */
      rld::strings wi;
      rld::config::parse_items (wrec, wi);
      for (rld::strings::const_iterator wsi = wi.begin ();
           wsi != wi.end ();
           ++wsi)
      {
        wrappers.push_back (wrapper (*wsi, config));
      }

      /*
       * Load the trace functions.
       */
      rld::strings ti;
      rld::config::parse_items (trec, ti);
      for (rld::strings::const_iterator tsi = ti.begin ();
           tsi != ti.end ();
           ++tsi)
      {
        rld::config::parse_items (config, *tsi, "trace", trace, true);
      }

    }

    void
    tracer::dump (std::ostream& out) const
    {
      out << " Tracer: " << name << std::endl
          << "  BSP: " << bsp << std::endl;
      for (wrappers::const_iterator wi = wrappers.begin ();
           wi != wrappers.end ();
           ++wi)
      {
        (*wi).dump (out);
      }
    }

    linker::linker ()
    {
    }

    void
    linker::load_config (const std::string& path,
                         const std::string& trace)
    {
      config.clear ();
      config.load (path);
      tracer.load (config, trace);
    }

    void
    linker::dump (std::ostream& out) const
    {
      const rld::config::paths& cpaths = config.get_paths ();
      out << "RTEMS Trace Linker" << std::endl
          << " Configuration Files: " << cpaths.size () << std::endl;
      for (rld::config::paths::const_iterator pi = cpaths.begin ();
           pi != cpaths.end ();
           ++pi)
      {
        out << "  " << (*pi) << std::endl;
      }

      tracer.dump (out);
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
    rld::trace::linker linker;
    std::string        ld_cmd;
    std::string        configuration;
    std::string        trace = "tracer";
    bool               exec_prefix_set = false;
#if HAVE_WARNINGS
    bool               warnings = false;
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
      linker.load_config (configuration, trace);
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
