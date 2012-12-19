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
 * @ingroup rtems-ld
 *
 * @brief RTEMS Linker compression handles compressed images.
 *
 */

#if !defined (_RLD_COMPRESSION_H_)
#define _RLD_COMPRESSION_H_

#include <rld-files.h>

namespace rld
{
  namespace compress
  {
    /**
     * A compressor.
     */
    class compressor
    {
    public:
      /**
       * Construct the compressor for the given image.
       *
       * @param image The image to read or write to.
       * @param size The size of the input and output buffers.
       * @param out The compressor is compressing.
       * @param compress Set to false to disable compression.
       */
      compressor (files::image& image,
                  size_t        size,
                  bool          out = true,
                  bool          compress = true);

      /**
       * Destruct the compressor.
       */
      ~compressor ();

      /**
       * Write the data to the output buffer and once the image buffer is full
       * compress and write the compressed data to the image.
       *
       * @param data The data to write to the image compressed
       * @param length The mount of data in bytes to write.
       */
      void write (const void* data, size_t length);

      /**
       * Write the section of the input image file to the output buffer and
       * once the image buffer is full compress and write the compressed data
       * to the image.
       *
       * @param input The input image.
       * @param offset The input image offset to read from.
       * @param length The mount of data in bytes to write.
       */
      void write (files::image& input, off_t offset, size_t length);

      /**
       * Flush the output buffer is data is present.
       */
      void flush ();

      /**
       * Read the compressed data into the input buffer and return the section
       * requested.
       *
       * @param data Write the decompressed data here.
       * @param length The mount of data in bytes to read.
       */
      void read (void* data, size_t length);

      /**
       * Read the decompressed data writing it to the image.
       *
       * @param output The output image.
       * @param offset The output image offset to write from.
       * @param length The mount of data in bytes to read.
       */
      void read (files::image& output_, off_t offset, size_t length);

      /**
       * The amount of uncompressed data transferred.
       *
       * @param return size_t The amount of data tranferred.
       */
      size_t transferred () const;

      /**
       * The amount of compressed data transferred.
       *
       * @param return size_t The amount of compressed data tranferred.
       */
      size_t compressed () const;

    private:

      /**
       * Output the block of data to the output file with the block header.
       *
       * @param forced If true output the buffer.
       */
      void output (bool forced = false);

      /**
       * Input a block of compressed data and decompress it.
       */
      void input ();

      files::image& image;            //< The image to read or write to or from.
      size_t        size;             //< The size of the buffer.
      bool          out;              //< If true the it is compression.
      bool          compress;         //< If true compress the data.
      uint8_t*      buffer;           //< The decompressed buffer
      uint8_t*      io;               //< The I/O buffer.
      size_t        level;            //< The amount of data in the buffer.
      size_t        total;            //< The amount of uncompressed data
                                      //  transferred.
      size_t        total_compressed; //< The amount of compressed data
                                      //  transferred.
    };

    /**
     * Compressor template function for writing data to the compressor..
     */
    template < typename T >
    void write (compressor& comp, const T value)
    {
      uint8_t bytes[sizeof (T)];
      T       v = value;
      int     b = sizeof (T) - 1;
      while (b >= 0)
      {
        bytes[b--] = (uint8_t) v;
        v >>= 8;
      }
      comp.write (bytes, sizeof (T));
    }

  }
}

static inline rld::compress::compressor& operator<< (rld::compress::compressor& comp,
                                                     const uint64_t             value) {
  rld::compress::write < uint64_t > (comp, value);
  return comp;
}

static inline rld::compress::compressor& operator<< (rld::compress::compressor& comp,
                                                     const uint32_t             value) {
  rld::compress::write < uint32_t > (comp, value);
  return comp;
}

static inline rld::compress::compressor& operator<< (rld::compress::compressor& comp,
                                                     const std::string&         str) {
  comp.write (str.c_str (), str.size ());
  return comp;
}

#endif
