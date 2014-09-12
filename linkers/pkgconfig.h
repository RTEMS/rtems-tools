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

#if !defined (_PKGCONFIG_H_)
#define _PKGCONFIG_H_

#include <map>
#include <string>

namespace pkgconfig
{
  /**
   * A simple class to parse a pkgconfig file as used in RTEMS. The RTEMS use
   * is simple and basically provides a simplified method to manage the various
   * flags used to build and link modules for a specific BSP.
   */
  class package
  {
  public:
    /**
     * The type of defines and fields parsed from a package config file.
     */
    typedef std::map < std::string, std::string > table;

    /**
     * Constructor and load the file.
     */
    package (const std::string& name);

    /**
     * Default constructor.
     */
    package ();

    /**
     * Load a package configuration file.
     *
     * @param name The file name of the package.
     */
    void load (const std::string& name);

    /**
     * Get a field from the package.
     *
     * @param label The label to search for.
     * @param result The result of the search.
     * @retval true The field was found.
     * @retval false The field was not found.
     */
    bool get (const std::string& label, std::string& result);

  private:
    table defines;  ///< The defines.
    table fields;   ///< The fields.
  };

}

#endif
