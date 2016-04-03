/*
 * Copyright (c) 2016, Chris Johns <chrisj@rtems.org>
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

#include <string.h>

#include <rld-buffer.h>
#include <rtems-utils.h>

namespace rld
{
  namespace buffer
  {
    buffer::buffer (const size_t size, bool le)
      : data (0),
        size (size),
        le (le),
        in (0),
        out (0),
        level_ (0)
    {
      data = new uint8_t[size];
      clear ();
    }

    buffer::buffer ()
      : data (0),
        size (0),
        le (true),
        in (0),
        out (0),
        level_ (0)
    {
    }

    buffer::buffer (const buffer& orig)
      : data (0),
        size (orig.size),
        le (orig.le),
        in (orig.in),
        out (orig.out),
        level_ (orig.level_)
    {
      data = new uint8_t[size];
      memcpy (data, orig.data, size);
    }

    buffer::~buffer ()
    {
      if (data)
        delete [] data;
    }

    void
    buffer::clear ()
    {
      memset (data, 0, size);
      out = 0;
      in = 0;
      level_ = 0;
    }

    void
    buffer::write (const void* data_, const size_t length)
    {
      if ((out + length) > size)
        throw rld::error ("Buffer overflow", "buffer:write");
      memcpy (&data[out], data_, length);
      out += length;
      if (out > level_)
        level_ = out;
    }

    void
    buffer::read (void* data_, const size_t length)
    {
      if ((in + length) > level_)
        throw rld::error ("Buffer underflow", "buffer:read");
      memcpy (data_, &data[in], length);
      in += length;
    }

    void
    buffer::fill (const size_t length, const uint8_t value)
    {
      if ((out + length) > size)
        throw rld::error ("Buffer overflow", "buffer:fill");
      memset (&data[out], value, length);
      out += length;
      if (out > level_)
        level_ = out;
    }

    void
    buffer::set (const size_t out_, const uint8_t value)
    {
      if (out_ < out)
        throw rld::error ("Invalid set out", "buffer:set");
      fill (out_ - out, value);
    }

    size_t
    buffer::level () const
    {
      return level_;
    }

    void
    buffer::write (files::image& img)
    {
      if (out > 0)
      {
        img.write (data, level_);
        clear ();
      }
    }

    void
    buffer::read (files::image& img, size_t length)
    {
      if (length > size)
        throw rld::error ("Invalid length", "buffer:read");
      if (length == 0)
        length = size;
      img.read (data, length);
      in = 0;
      out = 0;
      level_ = length;
    }

    void
    buffer::dump ()
    {
      rtems::utils::dump (data, level_, 1);
    }

    buffer& operator<< (buffer& buf, const uint64_t value)
    {
      write < uint64_t > (buf, value);
      return buf;
    }

    buffer& operator>> (buffer& buf, uint64_t& value)
    {
      read < uint64_t > (buf, value);
      return buf;
    }

    buffer& operator<< (buffer& buf, const uint32_t value)
    {
      write < uint32_t > (buf, value);
      return buf;
    }

    buffer& operator>> (buffer& buf, uint32_t& value)
    {
      read < uint32_t > (buf, value);
      return buf;
    }

    buffer& operator<< (buffer& buf, const uint16_t value)
    {
      write < uint16_t > (buf, value);
      return buf;
    }

    buffer& operator>> (buffer& buf, uint16_t& value)
    {
      read < uint16_t > (buf, value);
      return buf;
    }

    buffer& operator<< (buffer& buf, const uint8_t value)
    {
      buf.write (&value, 1);
      return buf;
    }

    buffer& operator>> (buffer& buf, uint8_t& value)
    {
      buf.read (&value, 1);
      return buf;
    }

    buffer& operator<< (buffer& buf, const std::string& str)
    {
      buf.write (str.c_str (), str.size ());
      return buf;
    }

    b_fill fill (const size_t amount, const uint8_t value)
    {
      return b_fill (amount, value);
    }

    b_set set (const size_t level, const uint8_t value)
    {
      return b_set (level, value);
    }

    b_skip skip (const size_t amount)
    {
      return b_skip (amount);
    }
  }
}
