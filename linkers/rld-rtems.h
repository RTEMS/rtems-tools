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
     * Set the RTEMS version.
     */
    void set_version (const std::string& version);

    /**
     * Set the arch/bsp string.
     */
    void set_arch_bsp (const std::string& arch_bsp);

    /**
     * Set the path to RTEMS.
     */
    void set_path (const std::string& path);

    /**
     * Get the RTEMS version.
     */
    const std::string version ();

    /**
     * Return the arch/bsp string.
     */
    const std::string arch_bsp ();

    /**
     * Return the architecture given an arch/bsp string.
     */
    const std::string arch ();

    /**
     * Return the bsp given an arch/bsp string.
     */
    const std::string bsp ();

    /**
     * Get the RTEMS path.
     */
    const std::string path ();

    /**
     * Return the RTEMS BSP prefix.
     */
    const std::string rtems_arch_prefix ();

    /**
     * Return the arch/bsp as an RTEMS prefix and BSP string.
     */
    const std::string rtems_arch_bsp ();
  }
}

#endif
