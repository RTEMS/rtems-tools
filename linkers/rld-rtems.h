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
 * @ingroup rtems-ld
 *
 * @brief Support to manage RTEMS.
 *
 */

#if !defined (_RLD_RTEMS_H_)
#define _RLD_RTEMS_H_

#include <string>

namespace rld
{
  namespace rtems
  {
    /**
     * The RTEMS default version.
     */
    extern std::string version;

    /**
     * The path to RTEMS.
     */
    extern std::string path;

    /**
     * Is the RTEMS installed.
     */
    extern bool installed;

    /**
     * The BSP name.
     */
    extern std::string arch_bsp;

    /**
     * Return the architecture given an arch/bsp string.
     */
    const std::string arch (const std::string& ab);

    /**
     * Return the bsp given an arch/bsp string.
     */
    const std::string bsp (const std::string& ab);

    /**
     * Return the RTEMS bsp string given an arch/bsp string.
     */
    const std::string rtems_bsp (const std::string& ab);

    /**
     * Load the configuration. Set the various values via the command or a
     * configuration file then check the configuration.
     */
    void load_cc ();

    /**
     * Process the BSP name updating the various CC flags.
     */
    void set_cc (void);
  }
}

#endif
