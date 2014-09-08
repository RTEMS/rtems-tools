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
#include <rld-rtems.h>

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
    struct signature
    {
      std::string     name; /**< The function's name. */
      function_args   args; /**< The function's list of arguments. */
      function_return ret;  /**< The fuctions return value. */

      /**
       * The default constructor.
       */
      signature ();

      /**
       * Construct the signature loading it from the configuration.
       */
      signature (const rld::config::record& record);

      /**
       * Return the function's declaration.
       */
      const std::string decl (const std::string& prefix = "") const;
    };

    /**
     * A container of signatures.
     */
    typedef std::map < std::string, signature > signatures;

    /**
     * A function is list of function signatures headers and defines that allow
     * a function to be wrapped.
     */
    struct function
    {
      std::string  name;        /**< The name of this wrapper. */
      rld::strings headers;     /**< Include statements. */
      rld::strings defines;     /**< Define statements. */
      signatures   signatures_; /**< Signatures in this function. */

      /**
       * Load the function.
       */
      function (rld::config::config& config,
                const std::string&   name);

      /**
       * Dump the function.
       */
      void dump (std::ostream& out) const;
    };

    /**
     * A container of functions.
     */
    typedef std::vector < function > functions;

    /**
     * A generator and that contains the functions used to trace arguments and
     * return values. It also provides the implementation of those functions.
     */
    struct generator
    {
      std::string  name;            /**< The name of this wrapper. */
      rld::strings headers;         /**< Include statements. */
      rld::strings defines;         /**< Define statements. */
      std::string  arg_trace;       /**< Code template to trace an argument. */
      std::string  ret_trace;       /**< Code template to trace the return value. */
      rld::strings code;            /**< Code block inserted before the trace code. */

      /**
       * Default constructor.
       */
      generator ();

      /**
       * Load the generator.
       */
      generator (rld::config::config& config,
                 const std::string&   name);

      /**
       * Dump the generator.
       */
      void dump (std::ostream& out) const;
    };

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
       * The the functions for the trace.
       */
      void load_functions (rld::config::config&        config,
                           const rld::config::section& section);

      /**
       * The the traces for the tracer.
       */
      void load_traces (rld::config::config&        config,
                        const rld::config::section& section);

      /**
       * Generate the wrapper object file.
       */
      void generate (rld::process::tempfile& c);

      /**
       * Generate the trace functions.
       */
      void generate_traces (rld::process::tempfile& c);

      /**
       * Get the traces.
       */
      const rld::strings& get_traces () const;

      /**
       * Dump the wrapper.
       */
      void dump (std::ostream& out) const;

    private:

      std::string  name;        /**< The name of the trace. */
      std::string  bsp;         /**< The BSP we are linking to. */
      rld::strings traces;      /**< The functions to trace. */
      functions    functions_;  /**< The functions that can be traced. */
      generator    generator_;  /**< The tracer's generator. */
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
      void generate_wrapper (rld::process::tempfile& c);

      /**
       * Compile the C file.
       */
      void compile_wrapper (rld::process::tempfile& c,
                            rld::process::tempfile& o);

      /**
       * Link the application.
       */
      void link (rld::process::tempfile& o,
                 const std::string&      ld_cmds);

      /**
       * Dump the linker.
       */
      void dump (std::ostream& out) const;

    private:

      rld::config::config    config;     /**< User configuration. */
      tracer                 tracer_;    /**< The tracer */
      rld::process::tempfile c; /**< The C wrapper file */
      rld::process::tempfile o; /**< The wrapper object file */
    };

    /**
     * Recursive parser for strings.
     */
    void
    parse (rld::config::config&        config,
           const rld::config::section& section,
           const std::string&          sec_name,
           const std::string&          rec_name,
           rld::strings&               items,
           bool                        split = true,
           int                         depth = 0)
    {
      if (depth > 32)
        throw rld::error ("too deep", "parsing: " + sec_name + '/' + rec_name);

      rld::config::parse_items (section, rec_name, items, false, false, split);

      rld::strings sl;

      rld::config::parse_items (section, sec_name, sl);

      for (rld::strings::iterator sli = sl.begin ();
           sli != sl.end ();
           ++sli)
      {
        const rld::config::section& sec = config.get_section (*sli);
        parse (config, sec, sec_name, rec_name, items, split, depth + 1);
      }

      /*
       * Make the items unique.
       */
      rld::strings::iterator ii;
      ii = std::unique (items.begin (), items.end ());
      items.resize (std::distance (items.begin (), ii));
    }

    signature::signature ()
    {
    }

    signature::signature (const rld::config::record& record)
    {
      /*
       * There can only be one function signature in the configuration.
       */
      if (!record.single ())
        throw rld::error ("duplicate", "signature: " + record.name);

      name = record.name;

      /*
       * Signatures are defined as the return value then the arguments
       * delimited by a comma and white space. No checking is made of the
       * return value or arguments.
       */
      rld::strings si;
      rld::config::parse_items (record, si);

      if (si.size () == 0)
        throw rld::error ("no return value", "signature: " + record.name);
      if (si.size () == 1)
          throw rld::error ("no arguments", "signature: " + record.name);

      ret = si[0];
      args.resize (si.size () - 1);
      std::copy (si.begin ()  + 1, si.end (), args.begin ());
    }

    const std::string
    signature::decl (const std::string& prefix) const
    {
      std::string ds = ret + ' ' + prefix + name + '(';
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

    function::function (rld::config::config& config,
                        const std::string&   name)
      : name (name)
    {
      /*
       * A function section optionally contain one or more records of:
       *
       * # headers     A list of sections containing headers or header records.
       * # header      A list of include string that are single or double quoted.
       * # defines     A list of sections containing defines or define record.
       * # defines     A list of define string that are single or double quoted.
       * # signatures  A list of section names of signatures.
       * # includes    A list of files to include.
       *
       * @note The quoting and list spliting is a little weak because a delimiter
       *       in a quote should not be seen as a delimiter.
       */
      const rld::config::section& section = config.get_section (name);

      config.includes (section);

      parse (config, section, "headers", "header", headers);
      parse (config, section, "defines", "define", defines);

      rld::strings sig_list;
      section.get_record_items ("signatures", sig_list);

      for (rld::strings::const_iterator sli = sig_list.begin ();
           sli != sig_list.end ();
           ++sli)
      {
        const rld::config::section& sig_sec = config.get_section (*sli);
        for (rld::config::records::const_iterator si = sig_sec.recs.begin ();
             si != sig_sec.recs.end ();
             ++si)
        {
          signature sig (*si);
          signatures_[sig.name] = sig;
        }
      }
    }

    void
    function::dump (std::ostream& out) const
    {
      out << "   Function: " << name << std::endl
          << "    Headers: " << headers.size () << std::endl;
      for (rld::strings::const_iterator hi = headers.begin ();
           hi != headers.end ();
           ++hi)
      {
        out << "     " << (*hi) << std::endl;
      }
      out << "   Defines: " << defines.size () << std::endl;
      for (rld::strings::const_iterator di = defines.begin ();
           di != defines.end ();
           ++di)
      {
        out << "     " << (*di) << std::endl;
      }
      out << "   Signatures: " << signatures_.size () << std::endl;
      for (signatures::const_iterator si = signatures_.begin ();
           si != signatures_.end ();
           ++si)
      {
        const signature& sig = (*si).second;
        out << "     " << sig.name << ": " << sig.decl () << ';' << std::endl;
      }
    }

    generator::generator ()
    {
    }

    generator::generator (rld::config::config& config,
                          const std::string&   name)
      : name (name)
    {
      /*
       * A generator section optionally contain one or more records of:
       *
       * # headers     A list of sections containing headers or header records.
       * # header      A list of include string that are single or double quoted.
       * # defines     A list of sections containing defines or define record.
       * # defines     A list of define string that are single or double quoted.
       * # code-blocks A list of section names of code blocks.
       * # includes    A list of files to include.
       *
       * @note The quoting and list spliting is a little weak because a delimiter
       *       in a quote should not be seen as a delimiter.
       */
      const rld::config::section& section = config.get_section (name);

      config.includes (section);

      parse (config, section, "headers",     "header", headers);
      parse (config, section, "defines",     "define", defines);
      parse (config, section, "code-blocks", "code",   code, false);

      arg_trace = rld::dequote (section.get_record_item ("arg-trace"));
      ret_trace = rld::dequote (section.get_record_item ("ret-trace"));
    }

    void
    generator::dump (std::ostream& out) const
    {
      out << "  Generator: " << name << std::endl
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
      out << "   Arg Trace Code: " << arg_trace << std::endl
          << "   Return Trace Code: " << ret_trace << std::endl
          << "   Code blocks: " << std::endl;
      for (rld::strings::const_iterator ci = code.begin ();
           ci != code.end ();
           ++ci)
      {
        out << "      > "
            << rld::find_replace (*ci, "\n", "\n      | ") << std::endl;
      }
    }

    tracer::tracer ()
    {
    }

    void
    tracer::load (rld::config::config& config,
                  const std::string&   tname)
    {
      /*
       * The configuration must contain a "section" section. This is the top level
       * configuration and may contain:
       *
       *  # name      The name of trace being linked.
       *  # options   A list of options as per the long command line args.
       *  # traces    The list of sections containing function lists to trace.
       *  # functions The list of sections containing function details.
       *  # include   The list of files to include.
       *
       * The following records are required:
       *
       *  # name
       *  # bsp
       *  # trace
       *  # functions
       */
      const rld::config::section& section = config.get_section (tname);

      config.includes (section);

      name = section.get_record_item ("name");

      load_functions (config, section);
      load_traces (config, section);
    }

    void
    tracer::load_functions (rld::config::config&        config,
                            const rld::config::section& section)
    {
      rld::strings fl;
      rld::config::parse_items (section, "functions", fl, true);
      for (rld::strings::const_iterator fli = fl.begin ();
           fli != fl.end ();
           ++fli)
      {
        functions_.push_back (function (config, *fli));
      }
    }

    void
    tracer::load_traces (rld::config::config&        config,
                         const rld::config::section& section)
    {
      parse (config, section, "traces", "trace", traces);

      rld::strings gens;
      std::string  gen;

      parse (config, section, "traces", "generator", gens);

      if (gens.size () > 1)
        throw rld::error ("duplicate generators", "tracer: " + section.name);

      if (gens.size () == 0)
      {
        gen =
          config.get_section ("default-generator").get_record_item ("generator");
      }
      else
      {
        gen = gens[0];
      }

      generator_ = generator (config, gen);
    }

    void
    tracer::generate (rld::process::tempfile& c)
    {
      c.open (true);

      if (rld::verbose ())
        std::cout << "wrapper C file: " << c.name () << std::endl;

      try
      {
        c.write_line ("/*");
        c.write_line (" * RTEMS Trace Linker Wrapper");
        c.write_line (" *  Automatically generated.");
        c.write_line (" */");

        c.write_line ("");
        c.write_line ("/*");
        c.write_line (" * Generator: " + generator_.name);
        c.write_line (" */");
        c.write_lines (generator_.defines);
        c.write_lines (generator_.headers);
        c.write_line ("");
        c.write_lines (generator_.code);

        generate_traces (c);
      }
      catch (...)
      {
        c.close ();
        throw;
      }

      c.close ();
    }

    void
    tracer::generate_traces (rld::process::tempfile& c)
    {
      for (functions::const_iterator fi = functions_.begin ();
           fi != functions_.end ();
           ++fi)
      {
        const function& funcs = *fi;

        for (rld::strings::const_iterator ti = traces.begin ();
             ti != traces.end ();
             ++ti)
        {
          const std::string&         trace = *ti;
          signatures::const_iterator si = funcs.signatures_.find (trace);

          if (si != funcs.signatures_.end ())
          {
            c.write_line ("");
            c.write_line ("/*");
            c.write_line (" * Function: " + funcs.name);
            c.write_line (" */");
            c.write_lines (funcs.defines);
            c.write_lines (funcs.headers);
            break;
          }
        }
      }

      c.write_line ("");
      c.write_line ("/*");
      c.write_line (" * Wrappers.");
      c.write_line (" */");

      for (rld::strings::const_iterator ti = traces.begin ();
           ti != traces.end ();
           ++ti)
      {
        const std::string& trace = *ti;
        bool               found = false;

        for (functions::const_iterator fi = functions_.begin ();
             fi != functions_.end ();
             ++fi)
        {
          const function&            funcs = *fi;
          signatures::const_iterator si = funcs.signatures_.find (trace);

          if (si != funcs.signatures_.end ())
          {
            found = true;

            const signature& sig = (*si).second;

            c.write_line("");
            c.write_line(sig.decl ("__wrap_"));
            c.write_line("{");

            /*
             * @todo Need to define as part of the function signature if ret
             *       processing is required.
             */
            bool has_ret = sig.ret != "void";

            if (has_ret)
              c.write_line(" " + sig.ret + " ret;");

            std::string l;

            for (size_t a = 0; a < sig.args.size (); ++a)
            {
              std::string l = ' ' + generator_.arg_trace;
              std::string n = rld::to_string ((int) (a + 1));
              l = rld::find_replace (l, "@ARG_NUM@", n);
              l = rld::find_replace (l, "@ARG_TYPE@", '"' + sig.args[0] + '"');
              l = rld::find_replace (l, "@ARG_SIZE@", "sizeof(" + sig.args[0] + ')');
              l = rld::find_replace (l, "@ARG_LABEL@", "a" + n);
              c.write_line(l);
            }

            l.clear ();

            if (has_ret)
              l = " ret =";

            l += " __real_" + sig.name + '(';
            for (size_t a = 0; a < sig.args.size (); ++a)
            {
              if (a)
                l += ", ";
              l += "a" + rld::to_string ((int) (a + 1));
            }
            l += ");";
            c.write_line(l);

            if (has_ret)
            {
              std::string l = ' ' + generator_.ret_trace;
              l = rld::find_replace (l, "@RET_TYPE@", '"' + sig.ret + '"');
              l = rld::find_replace (l, "@RET_SIZE@", "sizeof(" + sig.ret + ')');
              l = rld::find_replace (l, "@RET_LABEL@", "ret");
              c.write_line(l);
              c.write_line(" return ret;");
            }

            c.write_line("}");
          }
        }

        if (!found)
          throw rld::error ("not found", "trace function: " + trace);
      }
    }

    const rld::strings&
    tracer::get_traces () const
    {
      return traces;
    }

    void
    tracer::dump (std::ostream& out) const
    {
      out << " Tracer: " << name << std::endl
          << "  BSP: " << bsp << std::endl
          << "  Traces: " << traces.size () << std::endl;
      for (rld::strings::const_iterator ti = traces.begin ();
           ti != traces.end ();
           ++ti)
      {
        out << "   " << (*ti) << std::endl;
      }
      out << "  Functions: " << functions_.size () << std::endl;
      for (functions::const_iterator fi = functions_.begin ();
           fi != functions_.end ();
           ++fi)
      {
        (*fi).dump (out);
      }
      out << "  Generator: " << std::endl;
      generator_.dump (out);
    }

    linker::linker ()
    {
    }

    void
    linker::load_config (const std::string& path,
                         const std::string& trace)
    {
      std::string sp = get_prefix ();

      rld::path::path_join (sp, "share", sp);
      rld::path::path_join (sp, "rtems", sp);
      rld::path::path_join (sp, "trace-linker", sp);

      if (rld::verbose () || true)
        std::cout << "search path: " << sp << std::endl;

      config.set_search_path (sp);
      config.clear ();
      config.load (path);
      tracer_.load (config, trace);
    }

    void
    linker::generate_wrapper (rld::process::tempfile& c)
    {
      tracer_.generate (c);
    }

    void
    linker::compile_wrapper (rld::process::tempfile& c,
                             rld::process::tempfile& o)
    {
     rld::process::arg_container args;

      if (rld::verbose ())
        std::cout << "wrapper O file: " << o.name () << std::endl;

      rld::cc::make_cc_command (args);
      rld::cc::append_flags (rld::cc::ft_cflags, args);

      args.push_back ("-O2");
      args.push_back ("-g");
      args.push_back ("-c");
      args.push_back ("-o");
      args.push_back (o.name ());
      args.push_back (c.name ());

      rld::process::tempfile out;
      rld::process::tempfile err;
      rld::process::status   status;

      status = rld::process::execute (rld::cc::get_cc (),
                                      args,
                                      out.name (),
                                      err.name ());

      if ((status.type != rld::process::status::normal) ||
          (status.code != 0))
      {
        err.output (rld::cc::get_cc (), std::cout);
        throw rld::error ("Compiler error", "compiling wrapper");
      }
    }

    void
    linker::link (rld::process::tempfile& o,
                  const std::string&      ld_cmd)
    {
     rld::process::arg_container args;

      if (rld::verbose ())
        std::cout << "linking: " << o.name () << std::endl;

      std::string wrap = " -Wl,--wrap=";

      rld::cc::make_ld_command (args);

      rld::process::args_append (args,
                                 wrap + rld::join (tracer_.get_traces (), wrap));
      args.push_back (o.name ());
      rld::process::args_append (args, ld_cmd);

      rld::process::tempfile out;
      rld::process::tempfile err;
      rld::process::status   status;

      status = rld::process::execute (rld::cc::get_ld (),
                                      args,
                                      out.name (),
                                      err.name ());

      if ((status.type != rld::process::status::normal) ||
          (status.code != 0))
      {
        err.output (rld::cc::get_ld (), std::cout);
        throw rld::error ("Linker error", "linking");
      }
      err.output (rld::cc::get_ld (), std::cout);
    }

    void
    linker::dump (std::ostream& out) const
    {
      const rld::config::paths& cpaths = config.get_paths ();
      out << " Configuration Files: " << cpaths.size () << std::endl;
      for (rld::config::paths::const_iterator pi = cpaths.begin ();
           pi != cpaths.end ();
           ++pi)
      {
        out << "  " << (*pi) << std::endl;
      }

      tracer_.dump (out);
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
  { "keep",        no_argument,            NULL,           'k' },
  { "compiler",    required_argument,      NULL,           'c' },
  { "linker",      required_argument,      NULL,           'l' },
  { "exec-prefix", required_argument,      NULL,           'E' },
  { "cflags",      required_argument,      NULL,           'f' },
  { "rtems",       required_argument,      NULL,           'r' },
  { "rtems-bsp",   required_argument,      NULL,           'B' },
  { "config",      required_argument,      NULL,           'C' },
  { "wrapper",     required_argument,      NULL,           'W' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-trace-ld [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h          : help (also --help)" << std::endl
            << " -V          : print linker version number and exit (also --version)" << std::endl
            << " -v          : verbose (trace import parts), can supply multiple times" << std::endl
            << "               to increase verbosity (also --verbose)" << std::endl
            << " -w          : generate warnings (also --warn)" << std::endl
            << " -k          : keep temporary files (also --keep)" << std::endl
            << " -c compiler : target compiler is not standard (also --compiler)" << std::endl
            << " -l linker   : target linker is not standard (also --linker)" << std::endl
            << " -E prefix   : the RTEMS tool prefix (also --exec-prefix)" << std::endl
            << " -f cflags   : C compiler flags (also --cflags)" << std::endl
            << " -r path     : RTEMS path (also --rtems)" << std::endl
            << " -B bsp      : RTEMS arch/bsp (also --rtems-bsp)" << std::endl
            << " -W wrappe r : wrapper file name without ext (also --wrapper)" << std::endl
            << " -C ini      : user configuration INI file (also --config)" << std::endl;
  ::exit (exit_code);
}

static void
fatal_signal (int signum)
{
  signal (signum, SIG_DFL);

  rld::process::temporaries_clean_up ();

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
    std::string        cc;
    std::string        ld;
    std::string        ld_cmd;
    std::string        configuration;
    std::string        trace = "tracer";
    std::string        wrapper;
    std::string        rtems_path;
    std::string        rtems_arch_bsp;

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvwkVc:l:E:f:C:r:B:W:", rld_opts, NULL);
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

        case 'k':
          rld::process::set_keep_temporary_files ();
          break;

        case 'c':
          cc = optarg;
          break;

        case 'l':
          ld = optarg;
          break;

        case 'E':
          rld::cc::set_exec_prefix (optarg);
          break;

        case 'f':
          rld::cc::append_flags (optarg, rld::cc::ft_cflags);
          break;

        case 'r':
          rtems_path = optarg;
          break;

        case 'B':
          rtems_arch_bsp = optarg;
          break;

        case 'C':
          configuration = optarg;
          break;

        case 'W':
          wrapper = optarg;
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
      std::cout << "RTEMS Trace Linker " << rld::version () << std::endl;

    /*
     * Load the arch/bsp value if provided.
     */
    if (!rtems_arch_bsp.empty ())
    {
      if (rtems_path.empty ())
        throw rld::error ("No RTEMS path provide with arch/bsp", "options");
      rld::rtems::set_path (rtems_path);
      rld::rtems::set_arch_bsp (rtems_arch_bsp);
    }

    /**
     * Set the compiler and/or linker if provided.
     */
    if (!cc.empty ())
      rld::cc::set_cc (cc);
    if (!ld.empty ())
      rld::cc::set_ld (ld);

    /*
     * Load the remaining command line arguments into the linker command line.
     */
    while (argc--)
    {
      /*
       * Create this value because 'ld_cmd += ' ' + *argv++' fails on clang.
       */
      std::string av = *argv++;
      ld_cmd += ' ' + av;
    }
    ld_cmd = rld::trim (ld_cmd);

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

      rld::process::tempfile c (".c");
      rld::process::tempfile o (".o");

      if (!wrapper.empty ())
      {
        c.override (wrapper);
        c.keep ();
        o.override (wrapper);
        o.keep ();
      }

      linker.generate_wrapper (c);
      linker.compile_wrapper (c, o);
      linker.link (o, ld_cmd);

      if (rld::verbose ())
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
