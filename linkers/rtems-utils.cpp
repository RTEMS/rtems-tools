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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>
#include <iostream>
#include <vector>

#include <rtems-utils.h>

namespace rtems
{
  namespace utils
  {
    void
    dump (const void* addr,
          size_t      length,
          size_t      size,
          bool        real,
          size_t      line_length,
          uint32_t    offset)
    {
      char*    data;
      size_t   b = 0;
      uint8_t  d8 = 0;
      uint16_t d16 = 0;
      uint32_t d32 = 0;
      uint64_t d64 = 0;

      const uint8_t* addr8 = static_cast <const uint8_t*> (addr);

      data = new char[line_length];

      try
      {
        std::cout << std::hex << std::setfill ('0');

        while (true)
        {
          if (((b % line_length) == 0) || (b >= length))
          {
            if (b)
            {
              size_t line = b % line_length;

              if (line)
              {
                size_t remaining = line_length - line;
                remaining = (remaining * 2) + (remaining / size);
                std::cout << std::setfill (' ')
                          << std::setw (remaining) << ' '
                          << std::setfill ('0');
              }
              else
                line = line_length;

              std::cout << ' ';

              for (size_t c = 0; c < line; c++)
              {
                if ((data[c] < 0x20) || (data[c] > 0x7e))
                  std::cout << '.';
                else
                  std::cout << data[c];
              }

              if (b >= length)
              {
                std::cout << std::dec << std::setfill (' ')
                          << std::endl << std::flush;
                break;
              }

              std::cout << std::endl;
            }

            if (real)
              std::cout << std::setw (sizeof(void*)) << (uint64_t) (addr8 + b);
            else
              std::cout << std::setw (8) << (uint32_t) (offset + b);
          }

          if ((b & (line_length - 1)) == (line_length >> 1))
            std::cout << "-";
          else
            std::cout << " ";

          switch (size)
          {
            case sizeof (uint8_t):
            default:
              d8 = *(addr8 + b);
              std::cout << std::setw (2) << (uint32_t) d8;
              data[(b % line_length) + 0] = d8;
              break;

            case sizeof (uint16_t):
              d16 = *((const uint16_t*) (addr8 + b));
              std::cout << std::setw (4) << d16;
              data[(b % line_length) + 0] = (uint8_t) (d16 >> 8);
              data[(b % line_length) + 1] = (uint8_t) d16;
              break;

            case sizeof (uint32_t):
              d32 = *((const uint32_t*) (addr8 + b));
              std::cout << std::setw (8) << d32;
              for (int i = sizeof (uint32_t); i > 0; --i)
              {
                data[(b % line_length) + i] = (uint8_t) d32;
                d32 >>= 8;
              }
              break;

            case sizeof (uint64_t):
              d64 = *((const uint64_t*) (addr8 + b));
              std::cout << std::setw (16) << d64;
              for (int i = sizeof (uint64_t); i > 0; --i)
              {
                data[(b % line_length) + i] = (uint8_t) d64;
                d64 >>= 8;
              }
              break;
          }
          b += size;
        }
      }
      catch (...)
      {
        delete [] data;
        throw;
      }

      delete [] data;

      std::cout << std::dec << std::setfill (' ');
    }
  }
}
