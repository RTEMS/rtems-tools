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
 * @brief RTEMS RA Linker Main manages opions, sequence of operations and exceptions.
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
#include <rld-rap.h>
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
  { "output-path", required_argument,      NULL,           'p' },
  { "output",      required_argument,      NULL,           'o' },
  { "lib-path",    required_argument,      NULL,           'L' },
  { "lib",         required_argument,      NULL,           'l' },
  { "no-stdlibs",  no_argument,            NULL,           'n' },
  { "cc",          required_argument,      NULL,           'C' },
  { "exec-prefix", required_argument,      NULL,           'E' },
  { "march",       required_argument,      NULL,           'a' },
  { "mcpu",        required_argument,      NULL,           'c' },
  { "rap-strip",   no_argument,            NULL,           'S' },
  { "rpath",       required_argument,      NULL,           'R' },
  { "add-rap",     required_argument,      NULL,           'A' },
  { "replace-rap", required_argument,      NULL,           'r' },
  { "delete-rap",  required_argument,      NULL,           'd' },
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
  std::cout << "rtems-ra [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -o name   : linker output, this options is just" << std::endl
            << "             for waf, it will not output to file (also --output)" << std::endl
            << " -p path   : output path (also --output-path)" << std::endl
            << " -L path   : path to a library, add multiple for more than" << std::endl
            << "             one path (also --lib-path)" << std::endl
            << " -l lib    : add lib to the libraries searched, add multiple" << std::endl
            << "             for more than one library (also --lib)" << std::endl
            << " -n        : do not search standard libraries (also --no-stdlibs)" << std::endl
            << " -C file   : execute file as the target C compiler (also --cc)" << std::endl
            << " -E prefix : the RTEMS tool prefix (also --exec-prefix)" << std::endl
            << " -a march  : machine architecture (also --march)" << std::endl
            << " -c cpu    : machine architecture's CPU (also --mcpu)" << std::endl
            << " -S        : do not include file details (also --rap-strip)" << std::endl
            << " -R        : include file paths (also --rpath)" << std::endl
            << " -A        : Add rap files (also --Add-rap)" << std::endl
            << " -r        : replace rap files (also --replace-rap)" << std::endl
            << " -d        : delete rap files (also --delete-rap)" << std::endl
            << " -Wl,opts  : link compatible flags, ignored" << std::endl
            << "Output Formats:" << std::endl
            << " ra      - RTEMS archive container of rap files" << std::endl;
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
    rld::files::paths       libpaths;
    rld::files::paths       libs;
    rld::files::paths       libraries;
    rld::files::paths       outra;
    rld::files::paths       raps_add;
    rld::files::paths       raps_replace;
    rld::files::paths       raps_delete;
    std::string             cc_name;
    std::string             entry;
    std::string             exit;
    std::string             output_path = "./";
    std::string             output = "a.ra";
    bool                    standard_libs = true;
    bool                    exec_prefix_set = false;
    bool                    convert = true;
    rld::files::object_list dependents;

    libpaths.push_back (".");
    dependents.clear ();

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hVvnS:a:p:L:l:o:C:E:c:R:W:A:r:d", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-ra (RTEMS Linker) " << rld::version ()
                    << std::endl;
          ::exit (0);
          break;

        case 'v':
          rld::verbose_inc ();
          break;

        case 'l':
          libs.push_back (optarg);
          break;

        case 'A':
          /*
           * Add rap files to ra file.
           */
          raps_add.push_back (optarg);
          convert = false;
          break;

        case 'r':
          /*
           * replace rap files to ra file.
           */
          raps_replace.push_back (optarg);
          convert = false;
          break;

        case 'd':
          /*
           * delete rap files to ra file.
           */
          raps_delete.push_back (optarg);
          convert = false;
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

        case 'p':
          std::cout << "Output path: " << optarg << std::endl;
          output_path = optarg;
          break;

        case 'o':
          output = optarg;
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

        case 'S':
          rld::rap::add_obj_details = false;
          break;

        case 'R':
          rld::rap::rpath += optarg;
          rld::rap::rpath += '\0';
          break;

        case 'W':
          /* ignore linker compatiable flags */
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
      std::cout << "RTEMS RAP RA Linker " << rld::version () << std::endl;

    /*
     * libs can be elf archive and rap archive
     */
    while (argc--)
      libs.push_back (*argv++);

    /*
     * If the full path to CC is not provided and the exec-prefix is not set by
     * the command line see if it can be detected from the object file
     * types. This must be after we have added the object files because they
     * are used when detecting.
     */
    if (rld::cc::cc.empty () && !exec_prefix_set)
      rld::cc::exec_prefix = rld::elf::machine_type ();

    if (convert)
    {
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
      * Convert ar file to ra file
      */
      for (rld::files::paths::iterator p = libraries.begin (); p != libraries.end (); ++p)
      {
        rld::files::paths    library;
        rld::symbols::table  symbols;
        rld::files::cache*   cache = new rld::files::cache ();

        library.clear ();
        library.push_back (*p);

        /*
        * Open the cache.
        */
        cache->open ();

        /*
         * Load the library to the cache.
         */
        cache->add_libraries (library);

        cache->load_symbols (symbols);

        try
        {

          rld::files::objects& objs = cache->get_objects ();
          rld::files::paths    raobjects;

          int pos = -1;
          std::string rap_name;
          for (rld::files::objects::iterator obi = objs.begin ();
              obi != objs.end ();
              ++obi)
          {
            rld::files::object* obj = (*obi).second;

            dependents.clear ();

            rap_name = obj->name ().oname ();

            pos = obj->name ().oname ().rfind ('.', rap_name.length ());
            if (pos != -1)
            {
              rap_name.erase (pos, rap_name.length ());
            }

            rap_name += ".rap";

            dependents.push_back (obj);

            raobjects.push_back (rap_name);

            /* Todo: include absolute name for rap_name */

            rld::outputter::application (rap_name, entry, exit,
                                         dependents, *cache, symbols,
                                         true);
          }

          dependents.clear ();
          for (rld::files::paths::iterator ni = raobjects.begin (); ni != raobjects.end (); ++ni)
          {
            rld::files::object* obj = new rld::files::object (*ni);
            dependents.push_back (obj);
          }

          bool ra_rap = true;
          bool ra_exist = false;
          rld::files::cache cachera;
          std::string raname = *p;

          pos = -1;
          pos = raname.rfind ('/', raname.length ());
          if (pos != -1)
          {
            raname.erase (0, pos);
          }

          pos = -1;
          pos = raname.rfind ('.', raname.length ());
          if (pos != -1)
          {
            raname.erase (pos, raname.length ());
          }
          raname += ".ra";

          raname = output_path + raname;

          rld::outputter::archivera (raname, dependents, cachera,
                                     ra_exist, ra_rap);
          std::cout << "Generated: " << raname << std::endl;


          for (rld::files::object_list::iterator oi = dependents.begin ();
               oi != dependents.end ();
               ++oi)
          {
            rld::files::object& obj = *(*oi);
            ::unlink (obj.name ().oname ().c_str ());
          }
        }
        catch (...)
        {
          cache->archives_end ();
          throw;
        }

        cache->archives_end ();
        delete cache;
      }
    }
    else
    {
     /*
      * Add, replace, delete files from the ra file.
      */
      for (rld::files::paths::iterator pl = libs.begin (); pl != libs.end (); ++pl)
      {
        rld::files::paths    library;
        rld::files::cache*   cache = new rld::files::cache ();

        library.clear ();
        library.push_back (*pl);

        /*
        * Open the cache.
        */
        cache->open ();

        /*
         * Load the library to the cache.
         */
        cache->add_libraries (library);

        rld::files::objects& objs = cache->get_objects ();
        rld::files::paths    raobjects;

        std::string rap_name;
        bool rap_delete = false;

        dependents.clear ();
        /*
         * Delete rap files in ra file.
         */
        for (rld::files::objects::iterator obi = objs.begin ();
            obi != objs.end ();
            ++obi)
        {
          rld::files::object* obj = (*obi).second;

          rap_name = obj->name ().oname ();
          rap_delete = false;

          for (rld::files::paths::iterator pa = raps_delete.begin ();
               pa != raps_delete.end ();
               ++pa)
          {
            if (*pa == rap_name)
            {
              rap_delete = true;
              break;
            }
          }

          if (!rap_delete)
            dependents.push_back (obj);
        }

        /*
         * Add rap files into ra file, add supports replace.
         */
        bool rap_exist = false;
        rld::files::paths rap_objects;
        for (rld::files::paths::iterator pa = raps_add.begin ();
             pa != raps_add.end ();
             ++pa)
        {
          rap_exist = false;

          for (rld::files::object_list::iterator oi = dependents.begin ();
               oi != dependents.end ();
               ++oi)
          {
            rld::files::object& obj = *(*oi);
            if (*pa == obj.name ().oname ())
            {
              rap_exist = true;
              raps_replace.push_back (*pa);
              break;
            }
          }

          if (!rap_exist)
            rap_objects.push_back (*pa);
        }

        for (rld::files::paths::iterator pa = rap_objects.begin ();
             pa != rap_objects.end ();
             ++pa)
        {
          rld::files::object* obj = new rld::files::object (*pa);
          if (!obj->name ().exists ())
          {
            delete obj;
            throw rld::error ("file not exist", "rap-add");
          }
          else dependents.push_back (obj);
        }

        /*
         * Replace rap files in ra file
         */
        bool rap_replace = false;
        rld::files::cache cachera;

        rap_objects.clear ();
        cachera.open ();

        for (rld::files::paths::iterator pa = raps_replace.begin ();
             pa != raps_replace.end ();
             ++pa)
        {
          rap_replace = false;

          for (rld::files::object_list::iterator oi = dependents.begin ();
               oi != dependents.end ();
               )
          {
            rld::files::object& obj = *(*oi);
            if (*pa == obj.name ().oname ())
            {
              rap_replace = true;
              dependents.erase (oi++);
              break;
            }
            ++oi;
          }

          if (rap_replace)
            rap_objects.push_back (*pa);
        }

        for (rld::files::paths::iterator pa = rap_objects.begin ();
             pa != rap_objects.end ();
             ++pa)
        {
          rld::files::object* obj = new rld::files::object (*pa);
          if (!obj->name ().exists ())
          {
            delete obj;
            throw rld::error ("file not exist", "rap-add");
          }
          else dependents.push_back (obj);
        }

        rld::outputter::archivera (*pl, dependents, cachera,
                                   true, true);
        std::cout << "End" << std::endl;

        cache->archives_end ();
        delete cache;
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
