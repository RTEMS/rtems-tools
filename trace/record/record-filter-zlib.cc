/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (C) 2020 embedded brains GmbH & Co. KG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ZLIB_H

#include "client.h"

#include <cstring>

ZlibFilter::ZlibFilter() : buffer_(65536)
{
  std::memset(&stream_, 0, sizeof(stream_));
  inflateInit(&stream_);
}

ZlibFilter::~ZlibFilter()
{
  inflateEnd(&stream_);
}

bool ZlibFilter::Run(void** buf, size_t* n) {
  stream_.next_in = static_cast<unsigned char*>(*buf);
  stream_.avail_in = *n;
  stream_.next_out = &buffer_[0];
  size_t avail_out = 0;
  size_t buffer_size = buffer_.size();
  size_t chunk_size = buffer_size;
  stream_.avail_out = buffer_size;

  while (true) {
    int err = inflate(&stream_, Z_NO_FLUSH);
    if (err != Z_OK && err != Z_STREAM_END) {
      return false;
    }

    if (stream_.avail_in > 0) {
      chunk_size = buffer_size;
      buffer_size *= 2;
      buffer_.resize(buffer_size);
      stream_.next_out = reinterpret_cast<Bytef*>(&buffer_[chunk_size]);
      stream_.avail_out = chunk_size;
      avail_out = chunk_size;
      continue;
    }

    *buf = &buffer_[0];
    *n = avail_out + chunk_size - stream_.avail_out;
    return true;
  }
}

#endif  // HAVE_ZLIB_H
