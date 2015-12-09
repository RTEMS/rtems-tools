/*
 * Copyright (c) 2014-2015, Chris Johns <chrisj@rtems.org>
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
#include <iomanip>
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
     * Dump on error user option.
     */
    bool dump_on_error;

    /**
     * A container of arguments.
     */
    typedef std::vector < std::string > function_args;

    /**
     * The return value.
     */
    typedef std::string function_return;

    /**
     * An option is a name and value pair. We consider options as global.
     */
    struct option
    {
      std::string name;      /**< The name of this option. */
      std::string value;     /**< The option's value.. */

      /**
       * Load the option.
       */
      option (const std::string& name, const std::string& value)
        : name (name),
          value (value) {
      }
    };

    /**
     * A container of options.
     */
    typedef std::vector < option > options;

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
       * Has the signature got a return value ?
       */
      bool has_ret () const;

      /**
       * Has the signature got any arguments ?
       */
      bool has_args () const;

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
      std::string  lock_model;      /**< The lock model if provided, default "alloc". */
      std::string  lock_local;      /**< Code template to declare a local lock variable. */
      std::string  lock_acquire;    /**< The lock acquire if provided. */
      std::string  lock_release;    /**< The lock release if provided. */
      std::string  buffer_local;    /**< Code template to declare a local buffer variable. */
      rld::strings headers;         /**< Include statements. */
      rld::strings defines;         /**< Define statements. */
      std::string  entry_trace;     /**< Code template to trace the function entry. */
      std::string  entry_alloc;     /**< Code template to perform a buffer allocation. */
      std::string  arg_trace;       /**< Code template to trace an argument. */
      std::string  exit_trace;      /**< Code template to trace the function exit. */
      std::string  exit_alloc;      /**< Code template to perform a buffer allocation. */
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
       * Process any script based options.
       */
      void load_options (rld::config::config&        config,
                         const rld::config::section& section);

      /**
       * The defines for the trace.
       */
      void load_defines (rld::config::config&        config,
                         const rld::config::section& section);

      /**
       * The functions for the trace.
       */
      void load_functions (rld::config::config&        config,
                           const rld::config::section& section);

      /**
       * The enables for the tracer.
       */
      void load_enables (rld::config::config&        config,
                         const rld::config::section& section);

      /**
       * The triggers for the tracer.
       */
      void load_triggers (rld::config::config&        config,
                          const rld::config::section& section);

      /**
       * The traces for the tracer.
       */
      void load_traces (rld::config::config&        config,
                        const rld::config::section& section);

      /**
       * Get option from the options section.
       */
      const std::string get_option (const std::string& name) const;

      /**
       * Generate the wrapper object file.
       */
      void generate (rld::process::tempfile& c);

      /**
       * Generate the trace names as a string table.
       */
      void generate_names (rld::process::tempfile& c);

      /**
       * Generate the trace signature tables.
       */
      void generate_signatures (rld::process::tempfile& c);

      /**
       * Generate the enabled trace bitmap.
       */
      void generate_enables (rld::process::tempfile& c);

      /**
       * Generate the triggered trace bitmap.
       */
      void generate_triggers (rld::process::tempfile& c);

      /**
       * Generate the functions.
       */
      void generate_functions (rld::process::tempfile& c);

      /**
       * Generate the trace functions.
       */
      void generate_traces (rld::process::tempfile& c);

      /**
       * Generate a bitmap.
       */
      void generate_bitmap (rld::process::tempfile& c,
                            const rld::strings&     names,
                            const std::string&      label,
                            const bool              global_set);

      /**
       * Function macro replace.
       */
      void macro_func_replace (std::string&       text,
                               const signature&   sig,
                               const std::string& index);

      /**
       * Find the function given a name.
       */
      const function& find_function (const std::string& name) const;

      /**
       * Get the traces.
       */
      const rld::strings& get_traces () const;

      /**
       * Dump the wrapper.
       */
      void dump (std::ostream& out) const;

    private:

      std::string  name;          /**< The name of the trace. */
      rld::strings defines;       /**< Define statements. */
      rld::strings enables;       /**< The default enabled functions. */
      rld::strings triggers;      /**< The default trigger functions. */
      rld::strings traces;        /**< The functions to trace. */
      options      options_;      /**< The options. */
      functions    functions_;    /**< The functions that can be traced. */
      generator    generator_;    /**< The tracer's generator. */
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
      void load_config (const std::string& name,
                        const std::string& trace,
                        const std::string& path);

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
        throw rld::error ("no return value", "signature: " + name);
      if (si.size () == 1)
          throw rld::error ("no arguments", "signature: " + name);

      ret = si[0];
      args.resize (si.size () - 1);
      std::copy (si.begin ()  + 1, si.end (), args.begin ());
    }

    bool
    signature::has_ret () const
    {
      /*
       * @todo Need to define as part of the function signature if ret
       *       processing is required.
       */
      return ret != "void";
    }

    bool
    signature::has_args () const
    {
      if (args.empty ())
          return false;
      return ((args.size() == 1) && (args[0] == "void")) ? false : true;
    }

    const std::string
    signature::decl (const std::string& prefix) const
    {
      std::string ds = ret + ' ' + prefix + name + '(';
      if (has_args ())
      {
        int arg = 0;
        for (function_args::const_iterator ai = args.begin ();
             ai != args.end ();
             ++ai)
        {
          if (ai != args.begin ())
            ds += ", ";
          ds += (*ai) + " a" + rld::to_string (++arg);
        }
      }
      else
      {
        ds += "void";
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
        rld::strings sigs;
        rld::split(sigs, *sli, ',');

        for (rld::strings::const_iterator ssi = sigs.begin ();
             ssi != sigs.end ();
             ++ssi)
        {
          const rld::config::section& sig_sec = config.get_section (*ssi);
          for (rld::config::records::const_iterator si = sig_sec.recs.begin ();
               si != sig_sec.recs.end ();
               ++si)
          {
            signature sig (*si);
            signatures_[sig.name] = sig;
          }
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
       * # headers      A list of sections containing headers or header records.
       * # header       A list of include string that are single or double quoted.
       * # defines      A list of sections containing defines or define record.
       * # define       A list of define string that are single or double quoted.
       * # entry-trace  The wrapper call made on a function's entry.  Returns `bool
       *                where `true` is the function is being traced. This call is made
       *                without the lock being held if a lock is defined.
       * # arg-trace    The wrapper call made for each argment to the trace function if
       *                the function is being traced. This call is made without the
       *                lock being held if a lock is defined.
       * # exit-trace   The wrapper call made after a function's exit. Returns `bool
       *                where `true` is the function is being traced. This call is made
       *                without the lock being held if a lock is defined.
       * # ret-trace    The wrapper call made to log the return value if the funciton
       *                is being traced. This call is made without the lock being held
       *                if a lock is defined.
       * # lock-model   The wrapper code lock model. Models are 'alloc', or capture'.
       * # lock-local   The wrapper code to declare a local lock variable.
       * # lock-acquire The wrapper code to acquire the lock.
       * # lock-release The wrapper code to release the lock.
       * # buffer-local The wrapper code to declare a buffer index local variable.
       * # buffer-alloc The wrapper call made with a lock held if defined to allocate
       *                buffer space to hold the trace data. A suitable 32bit buffer
       *                index is returned. If there is no space an invalid index is
       *                returned. The generator must handle any overhead space needed.
       *                the generator needs to make sure the space is available before
       *                making the alloc all.
       * # code-blocks  A list of code blcok section names.
       * # code         A code block in `<<CODE ... CODE` (without the single quote).
       * # includes     A list of files to include.
       *
       * The following macros can be used in specific wrapper calls. The lists of
       * where you can use them is listed before. The macros are:
       *
       * # @FUNC_NAME@            The trace function name as a quote C string.
       * # @FUNC_INDEX@           The trace function index as a held in the
       *                          sorted list of trace functions by teh trace
       *                          linker. It can be used to index the `names`,
       *                          `enables` and `triggers` data.
       * # @FUNC_LABEL@           The trace function name as a C label that can
       *                          be referenced. You can take the address of
       *                          the label.
       * # @FUNC_DATA_SIZE@       The size of the data in bytes.
       * # @FUNC_DATA_ENTRY_SIZE@ The size of the entry data in bytes.
       * # @FUNC_DATA_RET_SIZE@   The size of the return data in bytes.
       * # @ARG_NUM@              The argument number to the trace function.
       * # @ARG_TYPE@             The type of the argument as a C string.
       * # @ARG_SIZE@             The size of the type of the argument in bytes.
       * # @ARG_LABEL@            The argument as a C label that can be referenced.
       * # @RET_TYPE@             The type of the return value as a C string.
       * # @RET_SIZE@             The size of the type of the return value in bytes.
       * # @RET_LABEL@            The return value as a C label that can be referenced.
       *
       * The `buffer-alloc`, `entry-trace` and `exit-trace` can be transformed using
       *  the following  macros:
       *
       * # @FUNC_NAME@
       * # @FUNC_INDEX@
       * # @FUNC_LABEL@
       * # @FUNC_DATA_SZIE@
       * # @FUNC_DATA_ENTRY_SZIE@
       * # @FUNC_DATA_EXIT_SZIE@
       *
       * The `arg-trace` can be transformed using the following macros:
       *
       * # @ARG_NUM@
       * # @ARG_TYPE@
       * # @ARG_SIZE@
       * # @ARG_LABEL@
       *
       * The `ret-trace` can be transformed using the following macros:
       *
       * # @RET_TYPE@
       * # @RET_SIZE@
       * # @RET_LABEL@
       *
       * @note The quoting and list spliting is a little weak because a delimiter
       *       in a quote should not be seen as a delimiter.
       */
      const rld::config::section& section = config.get_section (name);

      config.includes (section);

      parse (config, section, "headers",     "header", headers);
      parse (config, section, "defines",     "define", defines);
      parse (config, section, "code-blocks", "code",   code, false);

      if (section.has_record ("lock-model"))
        lock_model = rld::dequote (section.get_record_item ("lock-model"));
      if (section.has_record ("lock-local"))
        lock_local = rld::dequote (section.get_record_item ("lock-local"));
      if (section.has_record ("lock-acquire"))
        lock_acquire = rld::dequote (section.get_record_item ("lock-acquire"));
      if (section.has_record ("lock-release"))
        lock_release = rld::dequote (section.get_record_item ("lock-release"));
      if (section.has_record ("buffer-local"))
        buffer_local = rld::dequote (section.get_record_item ("buffer-local"));
      if (section.has_record ("entry-trace"))
        entry_trace = rld::dequote (section.get_record_item ("entry-trace"));
      if (section.has_record ("entry-alloc"))
        entry_alloc = rld::dequote (section.get_record_item ("entry-alloc"));
      if (section.has_record ("arg-trace"))
        arg_trace = rld::dequote (section.get_record_item ("arg-trace"));
      if (section.has_record ("exit-trace"))
        exit_trace = rld::dequote (section.get_record_item ("exit-trace"));
      if (section.has_record ("exit-alloc"))
        exit_alloc = rld::dequote (section.get_record_item ("exit-alloc"));
      if (section.has_record ("ret-trace"))
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
       *  # options   A list of option sections.
       *  # defines   A list of sections containing defines or define record.
       *  # define    A list of define string that are single or double quoted.
       *  # enables   The list of sections containing enabled functions to trace.
       *  # triggers  The list of sections containing enabled functions to trigger trace on.
       *  # traces    The list of sections containing function lists to trace.
       *  # functions The list of sections containing function details.
       *  # include   The list of files to include.
       *
       * The following records are required:
       *
       *  # name
       *  # traces
       *  # functions
       */
      const rld::config::section& section = config.get_section (tname);

      name = section.get_record_item ("name");
      load_options (config, section);
      config.includes (section);
      load_defines (config, section);
      load_functions (config, section);
      load_enables (config, section);
      load_triggers (config, section);
      load_traces (config, section);
    }

    void
    tracer::load_options (rld::config::config&        config,
                          const rld::config::section& section)
    {
      rld::strings opts;
      rld::config::parse_items (section, "options", opts, false, true, true);

      if (rld::verbose ())
        std::cout << "options: " << section.name << ": " << opts.size () << std::endl;

      options_.clear ();

      for (rld::strings::const_iterator osi = opts.begin ();
           osi != opts.end ();
           ++osi)
      {
        const rld::config::section& osec = config.get_section (*osi);

        if (rld::verbose ())
          std::cout << " options: " << osec.name
                    << ": recs:" << osec.recs.size () << std::endl;

        for (rld::config::records::const_iterator ori = osec.recs.begin ();
             ori != osec.recs.end ();
             ++ori)
        {
          const rld::config::record& opt = *ori;

          if (!opt.single ())
              throw rld::error ("mode than one option specified", "option: " + opt.name);

          options_.push_back (option (opt.name, opt[0]));

          if (opt.name == "dump-on-error")
          {
            dump_on_error = true;
          }
          else if (opt.name == "verbose")
          {
            int level = ::strtoul(opt[0].c_str (), 0, 0);
            if (level == 0)
              level = 1;
            for (int l = 0; l < level; ++l)
              rld::verbose_inc ();
          }
          else if (opt.name == "prefix")
          {
            rld::cc::set_exec_prefix (opt[0]);
          }
          else if (opt.name == "cc")
          {
            rld::cc::set_cc (opt[0]);
          }
          else if (opt.name == "ld")
          {
            rld::cc::set_ld (opt[0]);
          }
          else if (opt.name == "cflags")
          {
            rld::cc::append_flags (opt[0], rld::cc::ft_cflags);
          }
          else if (opt.name == "rtems-path")
          {
            rld::rtems::set_path(opt[0]);
          }
          else if (opt.name == "rtems-bsp")
          {
            rld::rtems::set_arch_bsp(opt[0]);
          }
        }
      }
    }

    void
    tracer::load_defines (rld::config::config&        config,
                          const rld::config::section& section)
    {
      parse (config, section, "defines", "define", defines);
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
    tracer::load_enables (rld::config::config&        config,
                          const rld::config::section& section)
    {
      parse (config, section, "enables", "enable", enables);
    }

    void
    tracer::load_triggers (rld::config::config&        config,
                           const rld::config::section& section)
    {
      parse (config, section, "triggers", "trigger", triggers);
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
      {
        if (dump_on_error)
          dump (std::cout);
        throw rld::error ("duplicate generators", "tracer: " + section.name);
      }

      if (gens.size () == 0)
      {
        const rld::config::section& dg_section = config.get_section ("default-generator");
        gen = dg_section.get_record_item ("generator");
        config.includes (dg_section);
      }
      else
      {
        gen = gens[0];
      }

      sort (traces.begin (), traces.end ());

      generator_ = generator (config, gen);
    }

    const std::string
    tracer::get_option (const std::string& name) const
    {
      std::string value;
      for (options::const_iterator oi = options_.begin ();
           oi != options_.end ();
           ++oi)
      {
        const option& opt = *oi;
        if (opt.name == name)
        {
          value = opt.value;
          break;
        }
      }
      return value;
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
        c.write_line (" * Tracer: " + name);
        c.write_line (" */");
        c.write_lines (defines);

        c.write_line ("");
        c.write_line ("/*");
        c.write_line (" * Generator: " + generator_.name);
        c.write_line (" */");
        c.write_lines (generator_.defines);
        c.write_lines (generator_.headers);
        c.write_line ("");
        generate_functions (c);
        generate_names (c);
        generate_signatures (c);
        generate_enables (c);
        generate_triggers (c);
        c.write_line ("");
        c.write_lines (generator_.code);

        generate_traces (c);
      }
      catch (...)
      {
        c.close ();
        if (dump_on_error)
          dump (std::cout);
        throw;
      }

      c.close ();

      if (rld::verbose (RLD_VERBOSE_DETAILS))
      {
        std::cout << "Generated C file:" << std::endl;
        c.output (" ", std::cout, true);
      }
    }

    void
    tracer::generate_names (rld::process::tempfile& c)
    {
      const std::string opt = get_option ("gen-names");

      if (opt == "disable")
        return;

      c.write_line ("");
      c.write_line ("/*");
      c.write_line (" * Names.");
      c.write_line (" */");

      std::stringstream sss;
      sss << "uint32_t __rtld_trace_names_size = " << traces.size() << ";" << std::endl
          << "const char const* __rtld_trace_names[" << traces.size() << "] = " << std::endl
          << "{";
      c.write_line (sss.str ());

      int count = 0;

      for (rld::strings::const_iterator ti = traces.begin ();
           ti != traces.end ();
           ++ti)
      {
        const std::string& trace = *ti;
        sss.str (std::string ());
        sss << "  /* " << std::setw (3) << count << " */ \"" << trace << "\",";
        c.write_line (sss.str ());
        ++count;
      }

      c.write_line ("};");
    }

    void
    tracer::generate_signatures (rld::process::tempfile& c)
    {
      const std::string opt = get_option ("gen-sigs");

      if (opt == "disable")
        return;

      c.write_line ("");
      c.write_line ("/*");
      c.write_line (" * Signatures.");
      c.write_line (" */");
      c.write_line ("");
      c.write_line ("typedef struct {");
      c.write_line (" uint32_t          size;");
      c.write_line (" const char* const type;");
      c.write_line ("} __rtld_trace_sig_arg;");
      c.write_line ("");
      c.write_line ("typedef struct {");
      c.write_line (" uint32_t                    argc;");
      c.write_line (" const __rtld_trace_sig_arg* args;");
      c.write_line ("} __rtld_trace_sig;");
      c.write_line ("");

      std::stringstream sss;

      for (rld::strings::const_iterator ti = traces.begin ();
           ti != traces.end ();
           ++ti)
      {
        const std::string& trace = *ti;
        bool               found = false;

        for (functions::const_iterator fi = functions_.begin ();
             !found && (fi != functions_.end ());
             ++fi)
        {
          const function&            funcs = *fi;
          signatures::const_iterator si = funcs.signatures_.find (trace);

          if (si != funcs.signatures_.end ())
          {
            found = true;

            const signature& sig = (*si).second;

            size_t argc = 1 + (sig.args.size () == 0 ? 1 : sig.args.size ());

            sss.str (std::string ());

            sss << "const __rtld_trace_sig_arg __rtld_trace_sig_args_" << trace
                << "[" << argc << "] =" << std::endl
                << "{" << std::endl;

            if (sig.has_ret ())
              sss << "  { sizeof (" << sig.ret << "), \"" << sig.ret << "\" }," << std::endl;
            else
              sss << "  { 0, \"void\" }," << std::endl;

            if (sig.has_args ())
            {
              for (size_t a = 0; a < sig.args.size (); ++a)
              {
                sss << "  { sizeof (" << sig.args[a] << "), \"" << sig.args[a] << "\" },"
                    << std::endl;
              }
            }
            else
              sss << "  { 0, \"void\" }," << std::endl;

            sss << "};" << std::endl;

            c.write_line (sss.str ());
          }
        }

        if (!found)
          throw rld::error ("not found", "trace function: " + trace);
      }

      sss.str (std::string ());

      sss << "const __rtld_trace_sig __rtld_trace_signatures[" << traces.size () << "] = "
          << "{" << std::endl;

      for (rld::strings::const_iterator ti = traces.begin ();
           ti != traces.end ();
           ++ti)
      {
        const std::string& trace = *ti;

        for (functions::const_iterator fi = functions_.begin ();
             fi != functions_.end ();
             ++fi)
        {
          const function&            funcs = *fi;
          signatures::const_iterator si = funcs.signatures_.find (trace);

          if (si != funcs.signatures_.end ())
          {
            const signature& sig = (*si).second;
            size_t argc = 1 + (sig.args.size () == 0 ? 1 : sig.args.size ());
            sss << "  { " << argc << ", __rtld_trace_sig_args_" << trace << " }," << std::endl;
            break;
          }
        }
      }

      sss << "};" << std::endl;

      c.write_line (sss.str ());
    }

    void
    tracer::generate_enables (rld::process::tempfile& c)
    {
      const std::string opt = get_option ("gen-enables");
      bool              global_state = false;

      if (opt == "disable")
        return;

      if (opt == "global-on")
        global_state = true;

      c.write_line ("");
      c.write_line ("/*");
      c.write_line (" * Enables.");
      c.write_line (" */");

      generate_bitmap (c, enables, "enables", global_state);
    }

    void
    tracer::generate_triggers (rld::process::tempfile& c)
    {
      const std::string opt = get_option ("gen-triggers");
      bool              global_state = false;

      if (opt == "disable")
        return;

      if (opt == "global-on")
        global_state = true;

      c.write_line ("");
      c.write_line ("/*");
      c.write_line (" * Triggers.");
      c.write_line (" */");

      generate_bitmap (c, triggers, "triggers", global_state);

      c.write_line ("");
    }

    void
    tracer::generate_functions (rld::process::tempfile& c)
    {
      c.write_line ("/*");
      c.write_line (" * Functions.");
      c.write_line (" */");

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
    }

    void
    tracer::generate_traces (rld::process::tempfile& c)
    {
      c.write_line ("/*");
      c.write_line (" * Wrappers.");
      c.write_line (" */");

      size_t count = 0;

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

            std::stringstream lss;
            lss << count;

            c.write_line("");

            std::string ds;
            std::string des;
            std::string drs;
            bool        ds_added = false;
            bool        des_added = false;
            bool        drs_added = false;
            ds  = "#define FUNC_DATA_SIZE_" + sig.name + " (";
            des = "#define FUNC_DATA_ENTRY_SIZE_" + sig.name + " (";
            drs = "#define FUNC_DATA_RET_SIZE_" + sig.name + " (";

            if (sig.has_args ())
            {
              for (size_t a = 0; a < sig.args.size (); ++a)
              {
                if (ds_added)
                  ds += " + ";
                else
                  ds_added = true;
                if (des_added)
                  des += " + ";
                else
                  des_added = true;
                ds += "sizeof(" + sig.args[a] + ')';
                des += "sizeof(" + sig.args[a] + ')';
              }
            }
            if (sig.has_ret () && !generator_.ret_trace.empty ())
            {
              if (ds_added)
                ds += " + ";
              else
                ds_added = true;
              if (drs_added)
                drs += " + ";
              else
                drs_added = true;
              ds += "sizeof(" + sig.ret + ')';
              drs += "sizeof(" + sig.ret + ')';
            }
            if (!ds_added)
              ds += '0';
            ds += ')';
            if (!des_added)
              des += '0';
            des += ')';
            if (!drs_added)
              drs += '0';
            drs += ')';
            c.write_line(ds);
            c.write_line(des);
            c.write_line(drs);

            c.write_line(sig.decl () + ";");
            c.write_line(sig.decl ("__real_") + ";");
            c.write_line(sig.decl ("__wrap_"));
            c.write_line("{");

            if (!generator_.lock_local.empty ())
              c.write_line(generator_.lock_local);

            if (!generator_.buffer_local.empty ())
              c.write_line(generator_.buffer_local);

            if (sig.has_ret ())
              c.write_line(" " + sig.ret + " ret;");

            std::string l;

            if (!generator_.lock_acquire.empty ())
              c.write_line(generator_.lock_acquire);

            if (!generator_.entry_alloc.empty ())
            {
              l = " " + generator_.entry_alloc;
              macro_func_replace (l, sig, lss.str ());
              c.write_line(l);
            }

            if (!generator_.lock_release.empty () &&
                (generator_.lock_model.empty () || (generator_.lock_model == "alloc")))
              c.write_line(generator_.lock_release);

            if (!generator_.entry_trace.empty ())
            {
              l = " " + generator_.entry_trace;
              macro_func_replace (l, sig, lss.str ());
              c.write_line(l);
            }

            if (sig.has_args ())
            {
              for (size_t a = 0; a < sig.args.size (); ++a)
              {
                std::string n = rld::to_string ((int) (a + 1));
                l = " " + generator_.arg_trace;
                l = rld::find_replace (l, "@ARG_NUM@", n);
                l = rld::find_replace (l, "@ARG_TYPE@", '"' + sig.args[a] + '"');
                l = rld::find_replace (l, "@ARG_SIZE@", "sizeof(" + sig.args[a] + ')');
                l = rld::find_replace (l, "@ARG_LABEL@", "a" + n);
                c.write_line(l);
              }
            }

            if (!generator_.lock_release.empty () && generator_.lock_model == "trace")
              c.write_line(generator_.lock_release);

            l.clear ();

            if (sig.has_ret ())
              l = " ret =";

            l += " __real_" + sig.name + '(';
            if (sig.has_args ())
            {
              for (size_t a = 0; a < sig.args.size (); ++a)
              {
                if (a)
                  l += ", ";
                l += "a" + rld::to_string ((int) (a + 1));
              }
            }
            l += ");";
            c.write_line(l);

            if (!generator_.lock_acquire.empty ())
              c.write_line(generator_.lock_acquire);

            if (!generator_.exit_alloc.empty ())
            {
              l = " " + generator_.exit_alloc;
              macro_func_replace (l, sig, lss.str ());
              c.write_line(l);
            }

            if (!generator_.lock_release.empty () &&
                (generator_.lock_model.empty () || (generator_.lock_model == "alloc")))
              c.write_line(generator_.lock_release);

            if (!generator_.exit_trace.empty ())
            {
              l = " " + generator_.exit_trace;
              macro_func_replace (l, sig, lss.str ());
              c.write_line(l);
            }

            if (sig.has_ret () && !generator_.ret_trace.empty ())
            {
              std::string l = " " + generator_.ret_trace;
              l = rld::find_replace (l, "@RET_TYPE@", '"' + sig.ret + '"');
              l = rld::find_replace (l, "@RET_SIZE@", "sizeof(" + sig.ret + ')');
              l = rld::find_replace (l, "@RET_LABEL@", "ret");
              c.write_line(l);
            }

            if (!generator_.lock_release.empty ())
              c.write_line(generator_.lock_release);

            if (sig.has_ret ())
              c.write_line(" return ret;");

            c.write_line("}");
          }
        }

        if (!found)
          throw rld::error ("not found", "trace function: " + trace);

        ++count;
      }
    }

    void
    tracer::generate_bitmap (rld::process::tempfile& c,
                             const rld::strings&     names,
                             const std::string&      label,
                             const bool              global_set)
    {
      uint32_t bitmap_size = ((traces.size () - 1) / (4 * 8)) + 1;

      std::stringstream ss;

      ss << "uint32_t __rtld_trace_" << label << "_size = " << traces.size() << ";" << std::endl
         << "const uint32_t __rtld_trace_" << label << "[" << bitmap_size << "] = " << std::endl
         << "{" << std::endl;

      size_t   count = 0;
      size_t   bit = 0;
      uint32_t bitmask = 0;

      for (rld::strings::const_iterator ti = traces.begin ();
           ti != traces.end ();
           ++ti)
      {
        const std::string& trace = *ti;
        bool               set = global_set;
        if (!global_set)
        {
          for (rld::strings::const_iterator ni = names.begin ();
               ni != names.end ();
               ++ni)
          {
            const std::string& name = *ni;
            if (trace == name)
              set = true;
          }
        }
        if (set)
          bitmask |= 1 << bit;
        ++bit;
        ++count;
        if ((bit >= 32) || (count >= traces.size ()))
        {
          ss << " 0x" << std::hex << std::setfill ('0') << std::setw (8) << bitmask << ',';
          if ((count % 4) == 0)
            ss << std::endl;
          bit = 0;
          bitmask = 0;
        }
      }

      c.write_line (ss.str ());

      c.write_line ("};");
    }

    void
    tracer::macro_func_replace (std::string&      text,
                               const signature&   sig,
                               const std::string& index)
    {
      text = rld::find_replace (text, "@FUNC_NAME@", '"' + sig.name + '"');
      text = rld::find_replace (text, "@FUNC_INDEX@", index);
      text = rld::find_replace (text, "@FUNC_LABEL@", sig.name);
      text = rld::find_replace (text, "@FUNC_DATA_SIZE@", "FUNC_DATA_SIZE_" + sig.name);
      text = rld::find_replace (text, "@FUNC_DATA_ENTRY_SIZE@", "FUNC_DATA_ENTRY_SIZE_" + sig.name);
      text = rld::find_replace (text, "@FUNC_DATA_RET_SIZE@", "FUNC_DATA_RET_SIZE_" + sig.name);
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
    linker::load_config (const std::string& name,
                         const std::string& trace,
                         const std::string& path)
    {
      std::string sp = rld::get_prefix ();

      rld::path::path_join (sp, "share", sp);
      rld::path::path_join (sp, "rtems", sp);
      rld::path::path_join (sp, "trace-linker", sp);

      if (!path.empty ())
        sp = path + ':' + sp;

      if (rld::verbose ())
        std::cout << "search path: " << sp << std::endl;

      config.set_search_path (sp);
      config.clear ();
      config.load (name);
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
        if (dump_on_error)
          dump (std::cout);
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
  { "path",        required_argument,      NULL,           'P' },
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
            << " -W wrapper  : wrapper file name without ext (also --wrapper)" << std::endl
            << " -C ini      : user configuration INI file (also --config)" << std::endl
            << " -P path     : user configuration file search path (also --path)" << std::endl;
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
    std::string        path;
    std::string        trace = "tracer";
    std::string        wrapper;
    std::string        rtems_path;
    std::string        rtems_arch_bsp;

    rld::set_cmdline (argc, argv);

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvwkVc:l:E:f:C:P:r:B:W:", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-trace-ld (RTEMS Trace Linker) " << rld::version ()
                    << ", RTEMS revision " << rld::rtems::version ()
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

        case 'P':
          if (!path.empty ())
            path += ":";
          path += optarg;
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
    {
      std::cout << "RTEMS Trace Linker " << rld::version () << std::endl;
      std::cout << " " << rld::get_cmdline () << std::endl;
    }

    /*
     * Load the arch/bsp value if provided.
     */
    if (!rtems_arch_bsp.empty ())
    {
      const std::string& prefix = rld::get_prefix ();
      if (rtems_path.empty () && prefix.empty ())
        throw rld::error ("No RTEMS path provided with arch/bsp", "options");
      if (!rtems_path.empty ())
        rld::rtems::set_path (rtems_path);
      else
        rld::rtems::set_path (prefix);
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
      linker.load_config (configuration, trace, path);

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
