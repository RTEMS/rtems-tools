/*
 * Copyright (c) 2016, Chris Johns <chrisj@rtems.org>
 *
 * RTEMS Tools Project (http://www.rtems.org/)
 * This file is part of the RTEMS Tools package in 'rtems-tools'.
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
 * @brief RTEMS Init dumps the initialisation section data in a format we can
 *        read.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>

#include <cxxabi.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include <rld.h>
#include <rld-buffer.h>
#include <rld-files.h>
#include <rld-process.h>
#include <rld-rtems.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

namespace rld
{
  namespace exeinfo
  {
    /**
     * Sections we decode.
     */
    const char* init_sections[] =
    {
      ".rtemsroset",
      ".ctors",
      0
    };

    const char* fini_sections[] =
    {
      ".dtors",
      0
    };

    /**
     * An executable section's address, offset, size and alignment.
     */
    struct section
    {
      const files::section& sec;      //< The executable's section.
      buffer::buffer        data;     //< The section's data.

      /**
       * Construct the section.
       */
      section (const files::section& sec);

      /**
       * Copy construct.
       */
      section (const section& orig);

      /**
       * Clean up the section's memory.
       */
      ~section ();

    private:
      /**
       * Default constructor.
       */
      section ();
    };

    /**
     * Container of sections. Order is the address in memory.
     */
    typedef std::list < section > sections;

    /**
     * The kernel image.
     */
    struct image
    {
      files::object    exe;        //< The object file that is the executable.
      symbols::table   symbols;   //< The synbols for a map.
      symbols::addrtab addresses; //< The symbols keyed by address.
      files::sections  secs;      //< The sections in the executable.

      /**
       * Load the executable file.
       */
      image (const std::string exe_name);

      /**
       * Clean up.
       */
      ~image ();

      /*
       * Output the sections.
       */
      void output_sections ();

      /*
       * Output the init sections.
       */
      void output_init ();

      /*
       * Output the fini sections.
       */
      void output_fini ();

      /*
       * Output init/fini worker.
       */
      void output_init_fini (const char* label, const char** names);
    };

    section::section (const files::section& sec)
      : sec (sec),
        data (sec.size)
    {
    }

    section::section (const section& orig)
      : sec (orig.sec),
        data (orig.data)
    {
    }

    section::~section ()
    {
    }

    /**
     * Helper for for_each to filter and load the sections we wish to
     * dump.
     */
    class section_loader:
      public std::unary_function < const files::section, void >
    {
    public:

      section_loader (image& img, sections& secs, const char* names[]);

      ~section_loader ();

      void operator () (const files::section& fsec);

    private:

      image&       img;
      sections&    secs;
      const char** names;
    };

    section_loader::section_loader (image&      img,
                                    sections&   secs,
                                    const char* names[])
      : img (img),
        secs (secs),
        names (names)
    {
    }

    section_loader::~section_loader ()
    {
    }

    void
    section_loader::operator () (const files::section& fsec)
    {
      if (rld::verbose () >= RLD_VERBOSE_DETAILS)
        std::cout << "init:section-loader: " << fsec.name
                  << " address=" << std::hex << fsec.address << std::dec
                  << " relocs=" << fsec.relocs.size ()
                  << " fsec.size=" << fsec.size
                  << " fsec.alignment=" << fsec.alignment
                  << " fsec.rela=" << fsec.rela
                  << std::endl;

      for (int n = 0; names[n] != 0; ++n)
      {
        if (fsec.name == names[n])
        {
          section sec (fsec);
          if (rld::verbose () >= RLD_VERBOSE_DETAILS)
            std::cout << "init:section-loader: " << fsec.name
                      << " added" << std::endl;
          img.exe.seek (fsec.offset);
          sec.data.read (img.exe, fsec.size);
          secs.push_back (sec);
          break;
        }
      }
    }

    image::image (const std::string exe_name)
      : exe (exe_name)
    {
      /*
       * Open the executable file and begin the session on it.
       */
      exe.open ();
      exe.begin ();

      if (!exe.valid ())
        throw rld::error ("Not valid: " + exe.name ().full (),
                          "init::image");

      /*
       * Load the symbols and sections.
       */
      exe.load_symbols (symbols, true);
      symbols.globals (addresses);
      symbols.weaks (addresses);
      symbols.locals (addresses);
      exe.get_sections (secs);
    }

    image::~image ()
    {
      exe.close ();
    }

    void
    image::output_sections ()
    {
      std::cout << "Sections: " << secs.size () << std::endl;

      for (files::sections::const_iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        const files::section& sec = *si;

        #define SF(f, i, c) if (sec.flags & (f)) flags[i] = c

        std::string flags ("--------------");

        SF (SHF_WRITE,            0, 'W');
        SF (SHF_ALLOC,            1, 'A');
        SF (SHF_EXECINSTR,        2, 'E');
        SF (SHF_MERGE,            3, 'M');
        SF (SHF_STRINGS,          4, 'S');
        SF (SHF_INFO_LINK,        5, 'I');
        SF (SHF_LINK_ORDER,       6, 'L');
        SF (SHF_OS_NONCONFORMING, 7, 'N');
        SF (SHF_GROUP,            8, 'G');
        SF (SHF_TLS,              9, 'T');
        SF (SHF_AMD64_LARGE,     10, 'a');
        SF (SHF_ENTRYSECT,       11, 'e');
        SF (SHF_COMDEF,          12, 'c');
        SF (SHF_ORDERED,         13, 'O');

        std::cout << "  " << std::left
                  << std::setw (15) << sec.name
                  << " " << flags
                  << std::right << std::hex << std::setfill ('0')
                  << " address: 0x" << std::setw (8) << sec.address
                  << " 0x" << std::setw (8) << sec.address + sec.size
                  << std::dec << std::setfill (' ')
                  << " size: " << std::setw (7) << sec.size
                  << " align: " << std::setw (3) << sec.alignment
                  << " relocs: " << std::setw (4) << sec.relocs.size ()
                  << std::endl;
      }

      std::cout << std::endl;
    }

    void
    image::output_init ()
    {
      output_init_fini ("Init", init_sections);
    }

    void
    image::output_fini ()
    {
      output_init_fini ("Fini", fini_sections);
    }

    void
    image::output_init_fini (const char* label, const char** names)
    {
      /*
       * Load the sections.
       */
      sections ifsecs;
      std::for_each (secs.begin (), secs.end (),
                     section_loader (*this, ifsecs, names));

      std::cout << label << " sections: " << ifsecs.size () << std::endl;

      for (sections::iterator ii = ifsecs.begin ();
           ii != ifsecs.end ();
           ++ii)
      {
        section&     sec = *ii;
        const size_t machine_size = sizeof (uint32_t);
        const int    count = sec.data.level () / machine_size;

        std::cout << " " << sec.sec.name << std::endl;

        for (int i = 0; i < count; ++i)
        {
          uint32_t         address;
          symbols::symbol* sym;
          sec.data >> address;
          sym = addresses[address];
          std::cout << "  "
                    << std::hex << std::setfill ('0')
                    << "0x" << std::setw (8) << address;
          if (sym)
            std::cout << " " << sym->name ();
          else
            std::cout << " no symbol";
          std::cout << std::dec << std::setfill ('0')
                    << std::endl;
        }
      }

      std::cout << std::endl;
    }
  }
}

/**
 * RTEMS Exe Info options. This needs to be rewritten to be like cc where only
 * a single '-' and long options is present.
 */
static struct option rld_opts[] = {
  { "help",        no_argument,            NULL,           'h' },
  { "version",     no_argument,            NULL,           'V' },
  { "verbose",     no_argument,            NULL,           'v' },
  { "map",         no_argument,            NULL,           'M' },
  { "all",         no_argument,            NULL,           'a' },
  { "sections",    no_argument,            NULL,           'S' },
  { "init",        no_argument,            NULL,           'I' },
  { "fini",        no_argument,            NULL,           'F' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-exeinfo [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -M        : generate map output (also --map)" << std::endl
            << " -a        : all output excluding the map (also --all)" << std::endl
            << " -S        : show all section (also --sections)" << std::endl
            << " -I        : show init section tables (also --init)" << std::endl
            << " -F        : show fini section tables (also --fini)" << std::endl;
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
    std::string exe_name;
    bool        map = false;
    bool        all = false;
    bool        sections = false;
    bool        init = false;
    bool        fini = false;

    rld::set_cmdline (argc, argv);

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvVMaSIF", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-exeinfo (RTEMS Executable Info) " << rld::version ()
                    << ", RTEMS revision " << rld::rtems::version ()
                    << std::endl;
          ::exit (0);
          break;

        case 'v':
          rld::verbose_inc ();
          break;

        case 'M':
          map = true;
          break;

        case 'a':
          all = true;
          break;

        case 'I':
          init = true;
          break;

        case 'F':
          fini = true;
          break;

        case 'S':
          sections = true;
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

    std::cout << "RTEMS Executable Info " << rld::version () << std::endl;
    std::cout << " " << rld::get_cmdline () << std::endl;

    /*
     * All means all types of output.
     */
    if (all)
    {
      sections = true;
      init = true;
      fini = true;
    }

    /*
     * If there is no executable there is nothing to convert.
     */
    if (argc == 0)
      throw rld::error ("no executable", "options");
    if (argc > 1)
      throw rld::error ("only a single executable", "options");

    /*
     * The name of the executable.
     */
    exe_name = *argv;

    if (rld::verbose ())
      std::cout << "exe-image: " << exe_name << std::endl;

    /*
     * Open the executable and read the symbols.
     */
    rld::exeinfo::image exe (exe_name);

    std::cout << "exe: " << exe.exe.name ().full () << std::endl;

    /*
     * Generate the output.
     */
    if (sections)
      exe.output_sections ();
    if (init)
      exe.output_init ();
    if (fini)
      exe.output_fini ();

    /*
     * Map ?
     */
    if (map)
      rld::symbols::output (std::cout, exe.symbols);
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
    std::cerr << realname << "] " << e.what () << std::endl << std::flush;
    ::free (realname);
    ec = 11;
  }
  catch (...)
  {
    /*
     * Helps to know if this happens.
     */
    std::cerr << "error: unhandled exception" << std::endl;
    ec = 12;
  }

  return ec;
}
