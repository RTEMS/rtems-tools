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

#include <fstream>
#include <iomanip>
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
#include <rld-symbols.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

/**
 * Header text.
 */
static const char* c_header[] =
{
  "/*",
  " * RTEMS Global Symbol Table",
  " *  Automatically generated. Do not edit..",
  " */",
  "",
  "extern const unsigned char rtems__rtl_base_globals[];",
  "extern const unsigned int rtems__rtl_base_globals_size;",
  "",
  "void rtems_rtl_base_sym_global_add (const unsigned char* , unsigned int );",
  "",
  "asm(\".section \\\".rodata\\\"\");",
  "",
  "asm(\"  .align   4\");",
  "asm(\"  .local   rtems__rtl_base_globals\");",
  "asm(\"rtems__rtl_base_globals:\");",
  "#if __mips__",
  " asm(\"  .align 0\");",
  "#else",
  " asm(\"  .balign 1\");",
  "#endif",
  0
};

static const char* c_trailer[] =
{
  "asm(\"  .byte    0\");",
  "asm(\"  .ascii   \\\"\\xde\\xad\\xbe\\xef\\\"\");",
#if BROKEN_ON_SOME_ASSEMBLERS
  "asm(\"  .type    rtems__rtl_base_globals, #object\");",
  "asm(\"  .size    rtems__rtl_base_globals, . - rtems__rtl_base_globals\");",
#endif
  "",
  "/*",
  " * Symbol table size.",
  " */",
  "asm(\"  .align   4\");",
  "asm(\"  .local   rtems__rtl_base_globals_size\");",
#if BROKEN_ON_SOME_ASSEMBLERS
  "asm(\"  .type    rtems__rtl_base_globals_size, #object\");",
  "asm(\"  .size    rtems__rtl_base_globals_size, 4\");",
#endif
  "asm(\"rtems__rtl_base_globals_size:\");",
  "asm(\"  .long rtems__rtl_base_globals_size - rtems__rtl_base_globals\");",
  "",
  0
};

static const char* c_rtl_call_body[] =
{
  "{",
  "  rtems_rtl_base_sym_global_add ((const unsigned char*) &rtems__rtl_base_globals,",
  "                                 rtems__rtl_base_globals_size);",
  "}",
  0
};

/**
 * Paint the data to the temporary file.
 */
static void
temporary_file_paint (rld::process::tempfile& t, const char* lines[])
{
  for (int l = 0; lines[l]; ++l)
    t.write_line (lines[l]);
}

/**
 * The constructor trailer.
 */
static void
c_constructor_trailer (rld::process::tempfile& c)
{
  c.write_line ("static void init(void) __attribute__ ((constructor));");
  c.write_line ("static void init(void)");
  temporary_file_paint (c, c_rtl_call_body);
}

/**
 * The embedded trailer.
 */
static void
c_embedded_trailer (rld::process::tempfile& c)
{
  c.write_line ("void rtems_rtl_base_global_syms_init(void);");
  c.write_line ("void rtems_rtl_base_global_syms_init(void)");
  temporary_file_paint (c, c_rtl_call_body);
}

/**
 * Generate the symbol map object file for loading or linking into
 * a running RTEMS machine.
 */
static void
generate_c (rld::process::tempfile& c,
              rld::symbols::table&  symbols,
              bool                  embed)
{
  temporary_file_paint (c, c_header);

  for (rld::symbols::symtab::const_iterator si = symbols.globals ().begin ();
       si != symbols.globals ().end ();
       ++si)
  {
    const rld::symbols::symbol& sym = *((*si).second);

    c.write_line ("asm(\"  .asciz \\\"" + sym.name () + "\\\"\");");

    if (embed)
    {
      c.write_line ("asm(\"  .long " + sym.name () + "\");");
    }
    else
    {
      std::stringstream oss;
      oss << std::hex << std::setfill ('0') << std::setw (8) << sym.value ();
      c.write_line ("asm(\"  .long 0x" + oss.str () + "\");");
    }
  }

  temporary_file_paint (c, c_trailer);

  if (embed)
    c_embedded_trailer (c);
  else
    c_constructor_trailer (c);
}

static void
generate_symmap (rld::process::tempfile& c,
                 const std::string&      output,
                 rld::symbols::table&    symbols,
                 bool                    embed)
{
  c.open (true);

  if (rld::verbose ())
    std::cout << "symbol C file: " << c.name () << std::endl;

  generate_c (c, symbols, embed);

  if (rld::verbose ())
    std::cout << "symbol O file: " << output << std::endl;

  rld::process::arg_container args;

  rld::cc::make_cc_command (args);
  rld::cc::append_flags (rld::cc::ft_cflags, args);

  args.push_back ("-O2");
  args.push_back ("-c");
  args.push_back ("-o");
  args.push_back (output);
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

/**
 * RTEMS Symbols options.
 */
static struct option rld_opts[] = {
  { "help",        no_argument,            NULL,           'h' },
  { "version",     no_argument,            NULL,           'V' },
  { "verbose",     no_argument,            NULL,           'v' },
  { "warn",        no_argument,            NULL,           'w' },
  { "keep",        no_argument,            NULL,           'k' },
  { "embed",       no_argument,            NULL,           'e' },
  { "symc",        required_argument,      NULL,           'S' },
  { "output",      required_argument,      NULL,           'o' },
  { "map",         required_argument,      NULL,           'm' },
  { "cc",          required_argument,      NULL,           'C' },
  { "exec-prefix", required_argument,      NULL,           'E' },
  { "cflags",      required_argument,      NULL,           'c' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-syms [options] kernel" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -w        : generate warnings (also --warn)" << std::endl
            << " -k        : keep temporary files (also --keep)" << std::endl
            << " -e        : embedded symbol table (also --embed)" << std::endl
            << " -S        : symbol's C file (also --symc)" << std::endl
            << " -o file   : output object file (also --output)" << std::endl
            << " -m file   : output a map file (also --map)" << std::endl
            << " -C file   : execute file as the target C compiler (also --cc)" << std::endl
            << " -E prefix : the RTEMS tool prefix (also --exec-prefix)" << std::endl
            << " -c cflags : C compiler flags (also --cflags)" << std::endl;
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
    rld::files::cache   kernel;
    rld::symbols::table symbols;
    std::string         kernel_name;
    std::string         output;
    std::string         map;
    std::string         cc;
    std::string         symc;
    bool                embed = false;

    rld::set_cmdline (argc, argv);

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvVwkef:S:o:m:E:c:C:", rld_opts, NULL);
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
          break;

        case 'k':
          rld::process::set_keep_temporary_files ();
          break;

        case 'e':
          embed = true;
          break;

        case 'o':
          output = optarg;
          break;

        case 'm':
          map = optarg;
          break;

        case 'C':
          if (rld::cc::is_exec_prefix_set ())
            std::cerr << "warning: exec-prefix ignored when CC provided" << std::endl;
          rld::cc::set_cc (optarg);
          break;

        case 'E':
          if (rld::cc::is_cc_set ())
            std::cerr << "warning: exec-prefix ignored when CC provided" << std::endl;
          rld::cc::set_exec_prefix (optarg);
          break;

        case 'c':
          rld::cc::set_flags (optarg, rld::cc::ft_cflags);
          break;

        case 'S':
          symc = optarg;
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
      std::cout << "RTEMS Kernel Symbols " << rld::version () << std::endl;

    /*
     * If there are no object files there is nothing to link.
     */
    if (argc == 0)
      throw rld::error ("no kernel file", "options");
    if (argc != 1)
      throw rld::error ("only one kernel file", "options");
    if (output.empty () && map.empty ())
      throw rld::error ("no output or map", "options");

    kernel_name = *argv;

    if (rld::verbose ())
      std::cout << "kernel: " << kernel_name << std::endl;

    /*
     * Load the symbols from the kernel.
     */
    try
    {
      /*
       * Load the kernel ELF file symbol table.
       */
      kernel.open ();
      kernel.add (kernel_name);
      kernel.load_symbols (symbols, true);

      /*
       * If the full path to CC is not provided and the exec-prefix is not set
       * by the command line see if it can be detected from the object file
       * types. This must be after we have added the object files because they
       * are used when detecting.
       */
      if (!cc.empty ())
        rld::cc::set_cc (cc);
      if (!rld::cc::is_cc_set () && !rld::cc::is_exec_prefix_set ())
        rld::cc::set_exec_prefix (rld::elf::machine_type ());

      /*
       * Create a map file if asked too.
       */
      if (!map.empty ())
      {
        std::ofstream mout;
        mout.open (map.c_str());
        if (!mout.is_open ())
          throw rld::error ("map file open failed", "map");
        mout << "RTEMS Kernel Symbols Map" << std::endl
             << " kernel: " << kernel_name << std::endl
             << std::endl;
        rld::symbols::output (mout, symbols);
        mout.close ();
      }

      /*
       * Create an output file if asked too.
       */
      if (!output.empty ())
      {
        rld::process::tempfile c (".c");

        if (!symc.empty ())
        {
          c.override (symc);
          c.keep ();
        }

        /*
         * Generate and compile the symbol map.
         */
        generate_symmap (c, output, symbols, embed);
      }

      kernel.close ();
    }
    catch (...)
    {
      kernel.close ();
      throw;
    }

    kernel.close ();
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
