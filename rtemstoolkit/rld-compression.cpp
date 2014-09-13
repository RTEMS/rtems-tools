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
                            bool          out,
                            bool          compress)
      : image (image),
        size (size),
        out (out),
        compress (compress),
        buffer (0),
        io (0),
        level (0),
        total (0),
        total_compressed (0)
    {
      if (size > 0xffff)
        throw rld::error ("Size too big, 16 bits only", "compression");

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
      if (!out)
        throw rld::error ("Write on read-only", "compression");

      const uint8_t* data = static_cast <const uint8_t*> (data_);

      while (length)
      {
        size_t appending;

        if (length > (size - level))
          appending = size - level;
        else
          appending = length;

        ::memcpy ((void*) (buffer + level), data, appending);

        data += appending;
        level += appending;
        length -= appending;
        total += appending;

        output ();
      }
    }

    void
    compressor::write (files::image& input, off_t offset, size_t length)
    {
      if (!out)
        throw rld::error ("Write on read-only", "compression");

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

        output ();
      }
    }

    size_t
    compressor::read (void* data_, size_t length)
    {
      if (out)
        throw rld::error ("Read on write-only", "compression");

      uint8_t* data = static_cast <uint8_t*> (data_);

      size_t amount = 0;

      while (length)
      {
        input ();

        if (level == 0)
          break;

        size_t appending;

        if (length > level)
          appending = level;
        else
          appending = length;

        ::memcpy (data, buffer, appending);
        ::memmove (buffer, buffer + appending, level - appending);

        data += appending;
        level -= appending;
        length -= appending;
        total += appending;
        amount += appending;
      }

      return amount;
    }

    size_t
    compressor::read (files::image& output_, off_t offset, size_t length)
    {
      if (out)
        throw rld::error ("Read on write-only", "compression");

      output_.seek (offset);

      return read (output_, length);
    }

    size_t
    compressor::read (files::image& output_, size_t length)
    {
      if (out)
        throw rld::error ("Read on write-only", "compression");

      size_t amount = 0;

      while (length)
      {
        input ();

        if (level == 0)
          break;

        size_t appending;

        if (length > level)
          appending = level;
        else
          appending = length;

        output_.write (buffer, appending);

        ::memmove (buffer, buffer + appending, level - appending);

        level -= appending;
        length -= appending;
        total += appending;
        amount += appending;
      }

      return amount;
    }

    void
    compressor::flush ()
    {
      output (true);
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

    off_t
    compressor::offset () const
    {
      return total;
    }

    void
    compressor::output (bool forced)
    {
      if (out && ((forced && level) || (level >= size)))
      {
        if (compress)
        {
          int     writing = ::fastlz_compress (buffer, level, io);
          uint8_t header[2];

          if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
            std::cout << "rtl: comp: offset=" << total_compressed
                      << " block-size=" << writing << std::endl;

          header[0] = writing >> 8;
          header[1] = writing;

          image.write (header, 2);
          image.write (io, writing);

          total_compressed += 2 + writing;
        }
        else
        {
          image.write (buffer, level);
        }

        level = 0;
      }
    }

    void
    compressor::input ()
    {
      if (!out && (level == 0))
      {
        if (compress)
        {
          uint8_t header[2];

          if (image.read (header, 2) == 2)
          {
            uint32_t block_size =
              (((uint32_t) header[0]) << 8) | (uint32_t) header[1];

            if (block_size == 0)
              throw rld::error ("Block size is invalid (0)", "compression");

            total_compressed += 2 + block_size;

            if (rld::verbose () >= RLD_VERBOSE_FULL_DEBUG)
              std::cout << "rtl: decomp: block-size=" << block_size
                        << std::endl;

            if (image.read (io, block_size) != block_size)
              throw rld::error ("Read past end", "compression");

            level = ::fastlz_decompress (io, block_size, buffer, size);
          }
        }
        else
        {
          image.read (buffer, size);
          level = size;
        }
      }
    }

  }
}
