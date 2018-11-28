/*
 * Copyright (c) 2016-2018, Chris Johns <chrisj@rtems.org>
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
#include <rld-dwarf.h>
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
     * Default section list.
     */
    const char* default_init[] =
    {
      ".rtemsroset",
      ".ctors",
      ".init",
      0
    };

    const char* default_fini[] =
    {
      ".dtors",
      ".fini",
      0
    };

    /**
     * ARM section list.
     */
    const char* arm_init[] =
    {
      ".rtemsroset",
      ".init_array",
      0
    };

    const char* arm_fini[] =
    {
      ".fini_array",
      0
    };

    /**
     * An executable section's address, offset, size and alignment.
     */
    struct section
    {
      const files::section& sec;        //< The executable's section.
      buffer::buffer        data;       //< The section's data.
      files::byteorder      byteorder;  //< The image's byteorder.

      /**
       * Construct the section.
       */
      section (const files::section& sec, files::byteorder byteorder);

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
      files::object    exe;         //< The object file that is the executable.
      dwarf::file      debug;       //< The executable's DWARF details.
      symbols::table   symbols;     //< The synbols for a map.
      symbols::addrtab addresses;   //< The symbols keyed by address.
      files::sections  secs;        //< The sections in the executable.
      const char**     init;        //< The init section's list for the machinetype.
      const char**     fini;        //< The fini section's list for the machinetype.


      /**
       * Load the executable file.
       */
      image (const std::string exe_name);

      /**
       * Clean up.
       */
      ~image ();

      /*
       * Check the compiler and flags match.
       */
      void output_compilation_unit (bool objects);

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

      /*
       * Output the inlined functions.
       */
      void output_inlined ();

      /*
       * Output the DWARF data.
       */
      void output_dwarf ();
    };

    section::section (const files::section& sec, files::byteorder byteorder)
      : sec (sec),
        data (sec.size, byteorder == rld::files::little_endian),
        byteorder (byteorder)
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
          if (rld::verbose () >= RLD_VERBOSE_DETAILS)
            std::cout << "init:section-loader: " << fsec.name
                      << " added" << std::endl;

          section sec (fsec, img.exe.get_byteorder ());
          img.exe.seek (fsec.offset);
          sec.data.read (img.exe, fsec.size);
          secs.push_back (sec);
          break;
        }
      }
    }

    image::image (const std::string exe_name)
      : exe (exe_name),
        init (0),
        fini (0)
    {
      /*
       * Open the executable file and begin the session on it.
       */
      exe.open ();
      exe.begin ();
      debug.begin (exe.elf ());

      if (!exe.valid ())
        throw rld::error ("Not valid: " + exe.name ().full (),
                          "init::image");

      /*
       * Set up the section lists for the machiner type.
       */
      switch (exe.elf ().machinetype ())
      {
        case EM_ARM:
          init = arm_init;
          fini = arm_fini;
          break;
        default:
          init  = default_init;
          fini  = default_fini;
          break;
      }

      /*
       * Load the symbols and sections.
       */
      exe.load_symbols (symbols, true);
      debug.load_debug ();
      debug.load_types ();
      debug.load_functions ();
      symbols.globals (addresses);
      symbols.weaks (addresses);
      symbols.locals (addresses);
      exe.get_sections (secs);
    }

    image::~image ()
    {
    }

    void
    image::output_compilation_unit (bool objects)
    {
      dwarf::compilation_units& cus = debug.get_cus ();

      std::cout << "Compilation: " << std::endl;

      rld::strings flag_exceptions = { "-O",
                                       "-g",
                                       "-mtune=",
                                       "-fno-builtin",
                                       "-fno-inline",
                                       "-fexceptions",
                                       "-fnon-call-exceptions",
                                       "-fvisibility=",
                                       "-fno-stack-protector",
                                       "-fbuilding-libgcc",
                                       "-fno-implicit-templates",
                                       "-fimplicit-templates",
                                       "-ffunction-sections",
                                       "-fdata-sections",
                                       "-frandom-seed=",
                                       "-fno-common",
                                       "-fno-keep-inline-functions" };

      dwarf::producer_sources producers;

      debug.get_producer_sources (producers);

      /*
       * Find which flags are common to the building of all source. We are only
       * interested in files that have any flags. This filters out things like
       * the assembler which does not have flags.
       */

      rld::strings all_flags;

      size_t source_max = 0;

      for (auto& p : producers)
      {
        dwarf::source_flags_compare compare;
        std::sort (p.sources.begin (), p.sources.end (), compare);

        for (auto& s : p.sources)
        {
          size_t len = rld::path::basename (s.source).length ();
          if (len > source_max)
            source_max = len;

          if (!s.flags.empty ())
          {
            for (auto& f : s.flags)
            {
              bool add = true;
              for (auto& ef : flag_exceptions)
              {
                if (rld::starts_with (f, ef))
                {
                  add = false;
                  break;
                }
              }
              if (add)
              {
                for (auto& af : all_flags)
                {
                  if (f == af)
                  {
                    add = false;
                    break;
                  }
                }
                if (add)
                  all_flags.push_back (f);
              }
            }
          }
        }
      }

      rld::strings common_flags;

      for (auto& flag : all_flags)
      {
        bool found_in_all = true;
        for (auto& p : producers)
        {
          for (auto& s : p.sources)
          {
            if (!s.flags.empty ())
            {
              bool flag_found = false;
              for (auto& f : s.flags)
              {
                if (flag == f)
                {
                  flag_found = true;
                  break;
                }
              }
              if (!flag_found)
              {
                found_in_all = false;
                break;
              }
            }
            if (!found_in_all)
              break;
          }
        }
        if (found_in_all)
          common_flags.push_back (flag);
      }

      std::cout << " Producers: " << producers.size () << std::endl;

      for (auto& p : producers)
      {
        std::cout << "  | " << p.producer
                  << ": " << p.sources.size () << " objects" << std::endl;
      }

      std::cout << " Common flags: " << common_flags.size () << std::endl
                << "  |";

      for (auto& f : common_flags)
        std::cout << ' ' << f;
      std::cout << std::endl;

      if (objects)
      {
        std::cout << " Object files: " << cus.size () << std::endl;

        rld::strings filter_flags = common_flags;
        filter_flags.insert (filter_flags.end (),
                             flag_exceptions.begin (),
                             flag_exceptions.end());

        for (auto& p : producers)
        {
          std::cout << ' ' << p.producer
                    << ": " << p.sources.size () << " objects" << std::endl;
          for (auto& s : p.sources)
          {
            std::cout << "   | "
                      << std::setw (source_max + 1) << std::left
                      << rld::path::basename (s.source);
            if (!s.flags.empty ())
            {
              bool first = true;
              for (auto& f : s.flags)
              {
                bool present = false;
                for (auto& ff : filter_flags)
                {
                  if (rld::starts_with(f, ff))
                  {
                    present = true;
                    break;
                  }
                }
                if (!present)
                {
                  if (first)
                  {
                    std::cout << ':';
                    first = false;
                  }
                  std::cout << ' ' << f;
                }
              }
            }
            std::cout << std::endl;
          }
        }
      }

      std::cout << std::endl;
    }

    void
    image::output_sections ()
    {
      std::cout << "Sections: " << secs.size () << std::endl;

      size_t max_section_name = 0;

      for (files::sections::const_iterator si = secs.begin ();
           si != secs.end ();
           ++si)
      {
        const files::section& sec = *si;
        if (sec.name.length() > max_section_name)
          max_section_name = sec.name.length();
      }

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
                  << std::setw (max_section_name) << sec.name
                  << " " << flags
                  << std::right << std::hex << std::setfill ('0')
                  << " addr: 0x" << std::setw (8) << sec.address
                  << " 0x" << std::setw (8) << sec.address + sec.size
                  << std::dec << std::setfill (' ')
                  << " size: " << std::setw (10) << sec.size
                  << " align: " << std::setw (3) << sec.alignment
                  << " relocs: " << std::setw (6) << sec.relocs.size ()
                  << std::endl;
      }

      std::cout << std::endl;
    }

    void
    image::output_init ()
    {
      output_init_fini ("Init", init);
    }

    void
    image::output_fini ()
    {
      output_init_fini ("Fini", fini);
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

      for (auto& sec : ifsecs)
      {
        const size_t machine_size = exe.elf ().machine_size ();
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
                    << "0x" << std::setw (8) << address
                    << std::dec << std::setfill ('0');
          if (sym)
          {
            std::string label = sym->name ();
            if (rld::symbols::is_cplusplus (label))
              rld::symbols::demangle_name (label, label);
            std::cout << " " << label;
          }
          else
          {
            std::cout << " no symbol";
          }
          std::cout << std::endl;
        }
      }

      std::cout << std::endl;
    }

    struct func_count
    {
      std::string name;
      int         count;
      size_t      size;

      func_count (std::string name, size_t size)
        : name (name),
          count (1),
          size (size) {
      }
    };
    typedef std::vector < func_count > func_counts;

    void image::output_inlined ()
    {
      size_t           total = 0;
      size_t           total_size = 0;
      size_t           inlined_size = 0;
      dwarf::functions funcs_inlined;
      dwarf::functions funcs_not_inlined;
      func_counts      counts;

      for (auto& cu : debug.get_cus ())
      {
        for (auto& f : cu.get_functions ())
        {
          if (f.size () > 0 && f.has_machine_code ())
          {
            bool counted;
            ++total;
            total_size += f.size ();
            switch (f.get_inlined ())
            {
              case dwarf::function::inl_inline:
              case dwarf::function::inl_declared_inlined:
                inlined_size += f.size ();
                counted = false;
                for (auto& c : counts)
                {
                  if (c.name == f.name ())
                  {
                    ++c.count;
                    c.size += f.size ();
                    counted = true;
                    break;
                  }
                }
                if (!counted)
                  counts.push_back (func_count (f.name (), f.size ()));
                funcs_inlined.push_back (f);
                break;
              case dwarf::function::inl_declared_not_inlined:
                funcs_not_inlined.push_back (f);
                break;
              default:
                break;
            }
          }
        }
      }

      std::cout << "inlined funcs   : " << funcs_inlined.size () << std::endl
                << "    total funcs : " << total << std::endl
                << " % inline funcs : " << (funcs_inlined.size () * 100) / total << '%'
                << std::endl
                << "     total size : " << total_size << std::endl
                << "    inline size : " << inlined_size << std::endl
                << "  % inline size : " << (inlined_size * 100) / total_size << '%'
                << std::endl;

      auto count_compare = [](func_count const & a, func_count const & b) {
        return a.size != b.size?  a.size < b.size : a.count > b.count;
      };
      std::sort (counts.begin (), counts.end (), count_compare);
      std::reverse (counts.begin (), counts.end ());

      std::cout  << std::endl << "inlined repeats : " << std::endl;
      for (auto& c : counts)
        if (c.count > 1)
          std::cout << std::setw (6) << c.size << ' '
                    << std::setw (4) << c.count << ' '
                    << c.name << std::endl;

      dwarf::function_compare compare (dwarf::function_compare::fc_by_size);

      std::sort (funcs_inlined.begin (), funcs_inlined.end (), compare);
      std::reverse (funcs_inlined.begin (), funcs_inlined.end ());

      std::cout << std::endl << "inline funcs : " << std::endl;
      for (auto& f : funcs_inlined)
      {
        std::string flags;

        std::cout << std::setw (6) << f.size () << ' '
                  << (char) (f.is_external () ? 'E' : ' ')
                  << (char) (f.get_inlined () == dwarf::function::inl_inline ? 'C' : ' ')
                  << std::hex << std::setfill ('0')
                  << " 0x" << std::setw (8) << f.pc_low ()
                  << std::dec << std::setfill (' ')
                  << ' ' << f.name ()
                  << std::endl;
      }

      if (funcs_not_inlined.size () > 0)
      {
        std::sort (funcs_not_inlined.begin (), funcs_not_inlined.end (), compare);
        std::reverse (funcs_not_inlined.begin (), funcs_not_inlined.end ());

        std::cout << std::endl << "inline funcs not inlined: " << std::endl;
        for (auto& f : funcs_not_inlined)
        {
          std::cout << std::setw (6) << f.size () << ' '
                    << (char) (f.is_external () ? 'E' : ' ')
                    << (char) (f.get_inlined () == dwarf::function::inl_inline ? 'C' : ' ')
                    << std::hex << std::setfill ('0')
                    << " 0x" << std::setw (8) << f.pc_low ()
                    << std::dec << std::setfill (' ')
                    << ' ' << f.name ()
                    << std::endl;
        }
      }
    }

    void image::output_dwarf ()
    {
      std::cout << "DWARF Data:" << std::endl;
      debug.dump (std::cout);
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
  { "inlined",     no_argument,            NULL,           'i' },
  { "dwarf",       no_argument,            NULL,           'D' },
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
            << " -a        : all output excluding the map and DAWRF (also --all)" << std::endl
            << " -S        : show all section (also --sections)" << std::endl
            << " -I        : show init section tables (also --init)" << std::endl
            << " -F        : show fini section tables (also --fini)" << std::endl
            << " -O        : show object files (also --objects)" << std::endl
            << " -i        : show inlined code (also --inlined)" << std::endl
            << " -D        : dump the DWARF data (also --dwarf)" << std::endl;
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

void
unhandled_exception (void)
{
  std::cerr << "error: exception handling error, please report" << std::endl;
  exit (1);
}

int
main (int argc, char* argv[])
{
  int ec = 0;

  setup_signals ();

  std::set_terminate(unhandled_exception);

  try
  {
    std::string exe_name;
    bool        map = false;
    bool        all = false;
    bool        sections = false;
    bool        init = false;
    bool        fini = false;
    bool        objects = false;
    bool        inlined = false;
    bool        dwarf_data = false;

    rld::set_cmdline (argc, argv);

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvVMaSIFiD", rld_opts, NULL);
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

        case 'O':
          objects = true;
          break;

        case 'i':
          inlined = true;
          break;

        case 'D':
          dwarf_data = true;
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
      objects = true;
      inlined = true;
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

    std::cout << "exe: " << exe.exe.name ().full () << std::endl
              << std::endl;

    /*
     * Generate the output.
     */
    exe.output_compilation_unit (objects);
    if (sections)
      exe.output_sections ();
    if (init)
      exe.output_init ();
    if (fini)
      exe.output_fini ();
    if (inlined)
      exe.output_inlined ();
    if (dwarf_data)
      exe.output_dwarf ();

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
  catch (std::exception& e)
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
