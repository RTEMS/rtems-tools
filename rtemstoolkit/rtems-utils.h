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
 * @ingroup RLD
 *
 * @brief A memory dump routine.
 *
 */

#if !defined (_RTEMS_UTILS_H_)
#define _RTEMS_UTILS_H_

#include <stdint.h>

namespace rtems
{
  namespace utils
  {
    /**
     * Hex display memory.
     *
     * @param addr The address of the memory to display.
     * @param length The number of elements to display.
     * @param size The size of the data element.
     * @param real Use the real address based on addr.
     * @param line_length Number of elements per line.
     * @param offset The printed offset.
     */
    void dump (const void* addr,
               size_t      length,
               size_t      size,
               bool        real = false,
               size_t      line_length = 16,
               uint32_t    offset = 0);
  }
}

#endif
