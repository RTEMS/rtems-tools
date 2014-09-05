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
#define RLD_PATHSTR_SEPARATOR     ';'
#define RLD_PATHSTR_SEPARATOR_STR ";"
#define RLD_DRIVE_SEPARATOR       (1)
#define RLD_LINE_SEPARATOR        "\r\n"
#else
#define RLD_PATH_SEPARATOR        '/'
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
  inline bool starts_with(const std::string& s1, const std::string& s2)
  {
    return s2.size () <= s1.size () && s1.compare (0, s2.size (), s2) == 0;
  }

  /**
   * Trim from start.
   */
  inline std::string& ltrim (std::string &s)
  {
    s.erase (s.begin (),
            std::find_if (s.begin (), s.end (),
                         std::not1 (std::ptr_fun < int, int > (std::isspace))));
    return s;
  }

  /**
   * Trim from end.
   */
  inline std::string& rtrim (std::string &s)
  {
    s.erase (std::find_if (s.rbegin (), s.rend (),
                           std::not1 (std::ptr_fun < int, int > (std::isspace))).base(),
             s.end());
    return s;
  }

  /**
   * Trim from both ends.
   */
  inline std::string& trim (std::string &s)
  {
    return ltrim (rtrim (s));
  }

  /**
   * Dequote a string.
   */
  inline std::string dequote (const std::string& s)
  {
    if (!s.empty ())
    {
      char front = s[0];
      char back = s[s.length () - 1];
      if ((front == '"') || (front == '\''))
      {
        if (front != back)
          throw rld::error ("invalid quoting", "string: " + s);
        return s.substr (1, s.length () - (1 + 1));
      }
    }
    return s;
  }

  /**
   * Find and replace.
   */
  inline std::string find_replace(const std::string& sin,
                                  const std::string& out,
                                  const std::string& in)
  {
    std::string s = sin;
    size_t      pos = 0;
    while ((pos = s.find (out, pos)) != std::string::npos)
    {
      s.replace (pos, out.length (), in);
      pos += in.length ();
    }
    return s;
  }

  /**
   * Split the string in a contain of strings based on the the
   * delimiter. Optionally trim any white space or include empty string.
   *
   * @todo The split should optionally honour string quoting.
   */
  inline strings& split (strings&           se,
                         const std::string& s,
                         char               delimiter = ' ',
                         bool               strip_quotes = true,
                         bool               strip_whitespace = true,
                         bool               empty = false)
  {
    std::stringstream ss(s);
    std::string       e;
    se.clear ();
    while (std::getline (ss, e, delimiter))
    {
      if (strip_whitespace)
        trim (e);
      if (strip_quotes)
        e = dequote (e);
      if (empty || !e.empty ())
      {
        se.push_back (e);
      }
    }
    return se;
  }

  /**
   * Join the strings together with the separator.
   */
  inline std::string join (const strings&     ss,
                           const std::string& separator)
  {
    std::string s;
    for (strings::const_iterator ssi = ss.begin ();
         ssi != ss.end ();
         ++ssi)
    {
      s += *ssi;
      if ((ssi != ss.begin ()) && (ssi != ss.end ()))
        s += separator;
    }
    return s;
  }

  /**
   * Convert a string to lower case.
   */
  inline std::string tolower (const std::string& sin)
  {
    std::string s = sin;
    std::transform (s.begin (), s.end (), s.begin (), ::tolower);
    return s;
  }

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
