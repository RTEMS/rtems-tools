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

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

/**
 * Path handling for Windows.
 */
#if __WIN32__
#define RLD_PATH_SEPARATOR        '\\'
#define RLD_PATH_SEPARATOR_STR    "\\"
#define RLD_PATHSTR_SEPARATOR     ';'
#define RLD_PATHSTR_SEPARATOR_STR ";"
#define RLD_DRIVE_SEPARATOR       (1)
#define RLD_LINE_SEPARATOR        "\r\n"
#else
#define RLD_PATH_SEPARATOR        '/'
#define RLD_PATH_SEPARATOR_STR    "/"
#define RLD_PATHSTR_SEPARATOR     ':'
#define RLD_PATHSTR_SEPARATOR_STR ":"
#define RLD_DRIVE_SEPARATOR       (0)
#define RLD_LINE_SEPARATOR        "\n"
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
   * A container of strings.
   */
  typedef std::vector < std::string > strings;

  /**
   * Does a string start with another string ?
   */
  bool starts_with(const std::string& s1, const std::string& s2);

  /**
   * Trim from start.
   */
  const std::string ltrim (const std::string& s);

  /**
   * Trim from end.
   */
  const std::string rtrim (const std::string& s);

  /**
   * Trim from both ends.
   */
  const std::string trim (const std::string& s);

  /**
   * Dequote a string.
   */
  const std::string dequote (const std::string& s);

  /**
   * Find and replace.
   */
  const std::string find_replace(const std::string& sin,
                                 const std::string& out,
                                 const std::string& in);

  /**
   * Split the string in a contain of strings based on the the
   * delimiter. Optionally trim any white space or include empty string.
   *
   * @todo The split should optionally honour string quoting.
   */
  const strings split (strings&           se,
                       const std::string& s,
                       char               delimiter = ' ',
                       bool               strip_quotes = true,
                       bool               strip_whitespace = true,
                       bool               empty = false);

  /**
   * Join the strings together with the separator.
   */
  const std::string join (const strings& ss, const std::string& separator);

  /**
   * Convert a string to lower case.
   */
  const std::string tolower (const std::string& sin);

  /**
   * Increment the verbose level.
   */
  void verbose_inc ();

  /**
   * Return the verbose level. Setting the flag more than once raises the
   * level.
   */
  int verbose (int level = 0);

  /**
   * The version string.
   */
  const std::string version ();

  /**
   * Container of strings to hold the results of a split.
   */
  typedef std::vector < std::string > strings;

  /**
   * Set the command line.
   */
  void set_cmdline (int argc, char* argv[]);

  /**
   * Get the command line.
   */
  const std::string get_cmdline ();

  /**
   * Set the progname.
   */
  void set_progname (const std::string& progname);

  /**
   * Get the progname. This is an absolute path.
   */
  const std::string get_progname ();

  /**
   * Get the program name.
   */
  const std::string get_program_name ();

  /**
   * Get the program path.
   */
  const std::string get_program_path ();

  /**
   * Get the current install prefix. If the path to the executable has 'bin' as
   * the executable's parent directory it is assumed the executable has been
   * installed under a standard PREFIX. If "bin" is not found return the
   * executable's absolute path.
   */
  const std::string get_prefix ();

  /**
   * Map of the cache and the symbol table.
   */
  void map (rld::files::cache& cache, rld::symbols::table& symbols);

  /**
   * Warn if externals in referenced object files are not used.
   */
  void warn_unused_externals (rld::files::object_list& objects);
}

#endif
