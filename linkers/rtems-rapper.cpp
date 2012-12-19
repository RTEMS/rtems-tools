/*
 * Copyright (c) 2012, Chris Johns <chrisj@rtems.org>
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
 * @brief RTEMS RAP Manager lets you look at and play with RAP files.
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
#include <rld-compression.h>
#include <rld-files.h>
#include <rld-process.h>

#ifndef HAVE_KILL
#define kill(p,s) raise(s)
#endif

#define RAP_
/**
 * A class to manage a RAP file.
 */
class rap
{
public:
  /**
   * Open a RAP file and read the header.
   */
  rap (const std::string& name);

  /**
   * Close the RAP file.
   */
  ~rap ();

  /**
   * Parse header.
   */
  void parse_header ();

  /**
   * Expand the image.
   */
  void expand ();

  /**
   * The name.
   */
  const std::string name () const;

private:

  rld::files::image image;
  std::string       header;
  size_t            rhdr_len;
  uint32_t          rhdr_length;
  uint32_t          rhdr_version;
  std::string       rhdr_compression;
  uint32_t          rhdr_checksum;
};

rap::rap (const std::string& name)
  : image (name),
    rhdr_len (0),
    rhdr_length (0),
    rhdr_version (0),
    rhdr_checksum (0)
{
  image.open ();
  parse_header ();
}

rap::~rap ()
{
  image.close ();
}

void
rap::parse_header ()
{
  std::string name = image.name ().full ();

  char rhdr[64];

  image.seek_read (0, (uint8_t*) rhdr, 64);

  if ((rhdr[0] != 'R') || (rhdr[1] != 'A') || (rhdr[2] != 'P') || (rhdr[3] != ','))
    throw rld::error ("Invalid RAP file", "open: " + name);

  char* sptr = rhdr + 4;
  char* eptr;

  rhdr_length = ::strtoul (sptr, &eptr, 10);

  if (*eptr != ',')
    throw rld::error ("Cannot parse RAP header", "open: " + name);

  sptr = eptr + 1;

  rhdr_version = ::strtoul (sptr, &eptr, 10);

  if (*eptr != ',')
    throw rld::error ("Cannot parse RAP header", "open: " + name);

  sptr = eptr + 1;

  if ((sptr[0] == 'N') &&
      (sptr[1] == 'O') &&
      (sptr[2] == 'N') &&
      (sptr[3] == 'E'))
  {
    rhdr_compression = "NONE";
    eptr = sptr + 4;
  }
  else if ((sptr[0] == 'L') &&
           (sptr[1] == 'Z') &&
           (sptr[2] == '7') &&
           (sptr[3] == '7'))
  {
    rhdr_compression = "LZ77";
    eptr = sptr + 4;
  }
  else
    throw rld::error ("Cannot parse RAP header", "open: " + name);

  if (*eptr != ',')
    throw rld::error ("Cannot parse RAP header", "open: " + name);

  sptr = eptr + 1;

  rhdr_checksum = strtoul (sptr, &eptr, 16);

  if (*eptr != '\n')
    throw rld::error ("Cannot parse RAP header", "open: " + name);

  rhdr_len = eptr - rhdr + 1;

  header.insert (0, rhdr, rhdr_len);

  image.seek (rhdr_len);
}

void
rap::expand ()
{
  std::string name = image.name ().full ();
  std::string extension = rld::files::extension (image.name ().full ());

  name = name.substr (0, name.size () - extension.size ()) + ".xrap";

  image.seek (rhdr_len);

  rld::compress::compressor comp (image, 2 * 1024, false);
  rld::files::image         out (name);

  out.open (true);
  comp.read (out, 0, image.size () - rhdr_len);
  out.close ();
}

const std::string
rap::name () const
{
  return image.name ().full ();
}

void
rap_expander (rld::files::paths& raps)
{
  std::cout << "Expanding .... " << std::endl;
  for (rld::files::paths::iterator pi = raps.begin();
       pi != raps.end();
       ++pi)
  {
    rap r (*pi);
    std::cout << ' ' << r.name () << std::endl;
    r.expand ();
  }
}

/**
 * RTEMS RAP options.
 */
static struct option rld_opts[] = {
  { "help",        no_argument,            NULL,           'h' },
  { "version",     no_argument,            NULL,           'V' },
  { "verbose",     no_argument,            NULL,           'v' },
  { "warn",        no_argument,            NULL,           'w' },
  { "expand",      no_argument,            NULL,           'x' },
  { NULL,          0,                      NULL,            0 }
};

void
usage (int exit_code)
{
  std::cout << "rtems-rap [options] objects" << std::endl
            << "Options and arguments:" << std::endl
            << " -h        : help (also --help)" << std::endl
            << " -V        : print linker version number and exit (also --version)" << std::endl
            << " -v        : verbose (trace import parts), can be supply multiple times" << std::endl
            << "             to increase verbosity (also --verbose)" << std::endl
            << " -w        : generate warnings (also --warn)" << std::endl
            << " -x        : expand (also --expand)" << std::endl;
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
    rld::files::paths raps;
    bool              expand = false;
#if HAVE_WARNINGS
    bool              warnings = false;
#endif

    while (true)
    {
      int opt = ::getopt_long (argc, argv, "hvwVx", rld_opts, NULL);
      if (opt < 0)
        break;

      switch (opt)
      {
        case 'V':
          std::cout << "rtems-rap (RTEMS RAP Manager) " << rld::version ()
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

        case '?':
          usage (3);
          break;

        case 'h':
          usage (0);
          break;

        case 'x':
          expand = true;
          break;
      }
    }

    argc -= optind;
    argv += optind;

    std::cout << "RTEMS RAP " << rld::version () << std::endl;

    /*
     * If there are no RAP files so there is nothing to do.
     */
    if (argc == 0)
      throw rld::error ("no RAP files", "options");

    /*
     * Load the remaining command line arguments into a container.
     */
    while (argc--)
      raps.push_back (*argv++);

    if (expand)
      rap_expander (raps);
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
