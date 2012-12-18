/*
 * Copyright (c) 2011, Chris Johns <chrisj@rtems.org>
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
 * @ingroup rtems-ld
 *
 * @brief RTEMS Linker readies the RTEMS object files for dynamic linking.
 *
 */

#if !defined (_RLD_H_)
#define _RLD_H_

#include <iostream>
#include <sstream>
#include <string>

/**
 * Path handling for Windows.
 */
#if __WIN32__
#define RLD_PATH_SEPARATOR        '\\'
#define RLD_PATHSTR_SEPARATOR     ';'
#define RLD_PATHSTR_SEPARATOR_STR ";"
#define RLD_DRIVE_SEPARATOR       (1)
#else
#define RLD_PATH_SEPARATOR        '/'
#define RLD_PATHSTR_SEPARATOR     ':'
#define RLD_PATHSTR_SEPARATOR_STR ":"
#define RLD_DRIVE_SEPARATOR       (0)
#endif

namespace rld
{
  /**
   * Forward declarations.
   */
  namespace files
  {
    class file;
    class image;
    class archive;
    class object;
  }
}

#include <rld-elf-types.h>
#include <rld-symbols.h>
#include <rld-elf.h>
#include <rld-files.h>

/**
 * The debug levels.
 */
#define RLD_VERBOSE_OFF        (0)
#define RLD_VERBOSE_INFO       (1)
#define RLD_VERBOSE_DETAILS    (2)
#define RLD_VERBOSE_TRACE      (3)
#define RLD_VERBOSE_TRACE_SYMS (4)
#define RLD_VERBOSE_TRACE_FILE (5)
#define RLD_VERBOSE_FULL_DEBUG (6)

namespace rld
{
  /**
   * Convert a supported type to a string.
   */
  template <class T>
  std::string to_string (T t, std::ios_base & (*f)(std::ios_base&) = std::dec)
  {
    std::ostringstream oss;
    oss << f << t;
    return oss.str();
  }

  /**
   * General error.
   */
  struct error
  {
    const std::string what;
    const std::string where;

    error (const std::ostringstream& what, const std::string& where) :
      what (what.str ()), where (where) {
    }

    error (const std::string& what, const std::string& where) :
      what (what), where (where) {
    }
  };

  /**
   * A convenience macro to make where a file and line number.
   */
  #define rld_error_at(_what) \
    rld::error (_what, std::string (__FILE__) + ":" + to_string (__LINE__))

  /**
   * Increment the verbose level.
   */
  void verbose_inc ();

  /**
   * Return the verbose level. Setting the flag more than once raises the
   * level.
   */
  int verbose ();

  /**
   * The version string.
   */
  const std::string version ();

  /**
   * The RTEMS version string.
   */
  const std::string rtems_version ();

  /**
   * Container of strings to hold the results of a split.
   */
  typedef std::vector < std::string > strings;

  /**
   * Split a string into strings by the separator.
   */
  void split (const std::string& str, strings& strs, char separator);

  /**
   * Map of the symbol table.
   */
  void map (rld::files::cache& cache, rld::symbols::table& symbols);

  /**
   * Warn is externals in referenced object files are not used.
   */
  void warn_unused_externals (rld::files::object_list& objects);
}

#endif
