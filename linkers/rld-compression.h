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
       * @param compress Set to false to disable compression.
       */
      compressor (files::image& image,
                  size_t        size,
                  bool          compress = true);

      /**
       * Destruct the compressor.
       */
      ~compressor ();

      /**
       * Write to the output buffer and the image once full
       * and compressed.
       *
       * @param data The data to write to the image compressed
       * @param length The mount of data in bytes to write.
       */
      void write (const void* data, size_t length);

      /**
       * Flush the output buffer is data is present.
       */
      void flush ();

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
      files::image& image;            //< The image to read or write to or from.
      size_t        size;             //< The size of the buffer.
      bool          compress;         //< If true compress the data.
      uint8_t*      buffer;           //< The decompressed buffer
      uint8_t*      io;               //< The I/O buffer.
      size_t        level;            //< The amount of data in the buffer.
      size_t        total;            //< The amount of uncompressed data
                                      //  transferred.
      size_t        total_compressed; //< The amount of compressed data
                                      //  transferred.
    };

  }


}

#endif
