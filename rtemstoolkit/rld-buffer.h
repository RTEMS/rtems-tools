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
/**
 * @file
 *
 * @ingroup rtems-ld
 *
 * @brief A buffer of data.
 *
 */

#if !defined (_RLD_BUFFER_H_)
#define _RLD_BUFFER_H_

#include <string>

#include <rld-files.h>

namespace rld
{
  namespace buffer
  {
    /**
     * A buffer to help manage formats.
     */
    class buffer
    {
    public:
      /**
       * Create a buffer.
       */
      buffer (const size_t size, bool le = true);

      /**
       * An empty buffer
       */
      buffer ();

      /**
       * Copy a buffer.
       */
      buffer (const buffer& orig);

      /*
       * Destory the buffer.
       */
      ~buffer ();

      /**
       * Clear the buffer reseting the level to zero.
       */
      void clear ();

      /**
       * Write the data to the buffer.
       *
       * @param data The data to write to the buffer.
       * @param length The amount of data in bytes to write.
       */
      void write (const void* data_, const size_t length);

      /**
       * Read the data from the buffer.
       *
       * @param data Read from the buffer in the data.
       * @param length The amount of data in bytes to read.
       */
      void read (void* data_, const size_t length);

      /**
       * Fill the data to the buffer.
       *
       * @param length The amount of data in bytes to fill with.
       * @param value The value to fill the buffer with.
       */
      void fill (const size_t length, const uint8_t value = 0);

      /**
       * Set the write pointer in the buffer to the level provided filing with
       * the value also provided.
       *
       * @param out_ The new out pointer.
       * @param value The value to fill the buffer with.
       */
      void set (const size_t out_, const uint8_t value = 0);

      /**
       * Skip the data in the buffer moving the read pointer.
       *
       * @param length The amount of data in bytes to skip.
       */
      void skip (const size_t length);

      /**
       * Rewind the in pointer the buffer to the pointer.
       *
       * @param in_ The new in pointer.
       */
      void rewind (const size_t in_);

      /*
       * The level in the buffer.
       */
      size_t level () const;

      /*
       * Write the data buffered to the image. Clear the buffer after.
       */
      void write (files::image& img);

      /*
       * Read the data from the image into the start of buffer.
       */
      void read (files::image& img, size_t length = 0);

      /*
       * Dump.
       */
      void dump ();

    private:
      uint8_t* data;    //< The data held in the buffer.
      size_t   size;    //< The zie of the date the buffer hold.
      bool     le;      //< True is little endian else it is big.
      size_t   in;      //< The data in pointer, used when writing.
      size_t   out;     //< The data out ponter, used when reading.
      size_t   level_;  //< The level of data in the buffer.
    };

    /**
     * Buffer template function for writing data to the buffer.
     */
    template < typename T >
    void write (buffer& buf, const T value)
    {
      uint8_t bytes[sizeof (T)];
      T       v = value;
      int     b = sizeof (T) - 1;
      while (b >= 0)
      {
        bytes[b--] = (uint8_t) v;
        v >>= 8;
      }
      buf.write (bytes, sizeof (T));
    }

    /**
     * Buffer template function for reading data to the buffer.
     */
    template < typename T >
    void read (buffer& buf, T& value)
    {
      uint8_t bytes[sizeof (T)];
      int     b = sizeof (T) - 1;
      buf.read (bytes, sizeof(T));
      value = 0;
      while (b >= 0)
      {
        value <<= 8;
        value |= (T) bytes[b--];
      }
    }

    /*
     * Insertion operators.
     */
    buffer& operator<< (buffer& buf, const uint64_t value);
    buffer& operator<< (buffer& buf, const uint32_t value);
    buffer& operator<< (buffer& buf, const uint16_t value);
    buffer& operator<< (buffer& buf, const uint8_t value);
    buffer& operator<< (buffer& buf, const std::string& str);

    /*
     * Extraction operators.
     */
    buffer& operator>> (buffer& buf, uint64_t& value);
    buffer& operator>> (buffer& buf, uint32_t& value);
    buffer& operator>> (buffer& buf, uint16_t& value);
    buffer& operator>> (buffer& buf, uint8_t& value);

    /*
     * Buffer fill manipulator.
     */
    struct b_fill
    {
      const uint8_t value;
      const size_t  amount;

      b_fill (const size_t amount, const uint8_t value)
        : value (value),
          amount (amount) {
      }

      friend buffer& operator<< (buffer& buf, const b_fill& bf) {
        buf.fill (bf.amount, bf.value);
        return buf;
      }
    };

    b_fill fill (const size_t amount, const uint8_t value = 0);

    /*
     * Buffer set manipulator.
     */
    struct b_set
    {
      const uint8_t value;
      const size_t  level;

      b_set (const size_t level, const uint8_t value)
        : value (value),
          level (level) {
      }

      friend buffer& operator<< (buffer& buf, const b_set& bs) {
        buf.set (bs.level, bs.value);
        return buf;
      }
    };

    b_set set (const size_t level, const uint8_t value = 0);

    /*
     * Buffer skip manipulator.
     */
    struct b_skip
    {
      const size_t amount;

      b_skip (const size_t amount)
        : amount (amount) {
      }

      friend buffer& operator>> (buffer& buf, const b_skip& bs) {
        buf.skip (bs.amount);
        return buf;
      }
    };

    b_skip skip (const size_t amount);
  }
}

#endif
