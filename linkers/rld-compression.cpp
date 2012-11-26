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
 * @ingroup rtems_ld
 *
 * @brief RTEMS Linker.
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>

#include <errno.h>
#include <string.h>

#include <rld.h>
#include <rld-compression.h>

#include "fastlz.h"

namespace rld
{
  namespace compress
  {
    compressor::compressor (files::image& image,
                            size_t        size,
                            bool          compress)
      : image (image),
        size (size),
        compress (compress),
        buffer (0),
        io (0),
        level (0),
        total (0),
        total_compressed (0)
    {
      buffer = new uint8_t[size];
      io = new uint8_t[size + (size / 10)];
    }

    compressor::~compressor ()
    {
      flush ();
      delete [] buffer;
      delete [] io;
    }

    void
    compressor::write (const void* data_, size_t length)
    {
      const uint8_t* data = static_cast <const uint8_t*> (data_);

      if (compress)
      {
        while (length)
        {
          size_t appending;

          if (length > (size - level))
            appending = size - level;
          else
            appending = length;

          ::memcpy ((void*) (buffer + level), data, appending);

          level += appending;
          length -= appending;
          total += appending;

          if (level >= size)
          {
            int writing =
              ::fastlz_compress (buffer, level, io);

            image.write (io, writing);

            total_compressed += writing;
            level = 0;
          }
        }
      }
      else
      {
        image.write (data, length);
        total += length;
      }
    }

    void
    compressor::write (files::image& input, off_t offset, size_t length)
    {
      input.seek (offset);

      while (length)
      {
        size_t appending;

        if (length > (size - level))
          appending = size - level;
        else
          appending = length;

        input.read ((void*) (buffer + level), appending);

        level += appending;
        length -= appending;
        total += appending;

        if (level >= size)
        {
          if (compress)
          {
            int writing =
              ::fastlz_compress (buffer, level, io);

            image.write (io, writing);

            total_compressed += writing;
          }
          else
          {
            image.write (buffer, length);
          }

          level = 0;
        }
      }
    }

    void
    compressor::flush ()
    {
      if (level)
      {
        int writing =
          ::fastlz_compress (buffer, level, io);

        image.write (io, writing);

        total_compressed += writing;
        level = 0;
      }
    }

    size_t
    compressor::transferred () const
    {
      return total;
    }

    size_t
    compressor::compressed () const
    {
      return total_compressed;
    }

  }
}
