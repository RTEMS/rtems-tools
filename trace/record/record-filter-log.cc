/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (C) 2024 embedded brains GmbH & Co. KG
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

#include "client.h"

const char LogFilter::kBeginOfRecords[] = "*** BEGIN OF RECORDS BASE64 ";

static const char kBase64Begin[] = "**";

static const char kBase64ZlibBegin[] = "LIB ***";

static const char kEndOfRecordsBase64[] = "** END OF RECORDS BASE64 ***";

static const char kEndOfRecordsBase64Zlib[] =
    "** END OF RECORDS BASE64 ZLIB ***";

bool LogFilter::Error(const char* message, void** buf, const char* in) {
  state_ = kDecodingDone;
  std::cerr << "log filter error: " << message << " at byte "
            << (consumed_ + static_cast<size_t>(in - static_cast<char*>(*buf)))
            << std::endl;
  return false;
}

bool LogFilter::Run(void** buf, size_t* n) {
  char* b = static_cast<char*>(*buf);
  size_t m = *n;
  char* in = b;
  const char* end = in + m;
  State s = state_;
  const char* ss = sub_state_;

  while (in != end) {
    switch (s) {
      case kSearchBeginOfRecords:
        if (*ss == '\0') {
          if (*in == '*') {
            s = kExpectBase64Begin;
            ss = kBase64Begin;
          } else if (*in == 'Z') {
#ifdef HAVE_ZLIB_H
            s = kExpectBase64ZlibBegin;
            ss = kBase64ZlibBegin;
#else
            return Error("zlib decompression is not supported", buf, in);
#endif
          } else {
            return Error("unexpected begin of records marker", buf, in);
          }
        } else if (*in == *ss) {
          ++ss;
        } else {
          ss = kBeginOfRecords;
        }

        --m;
        b = in;
        break;
      case kExpectBase64Begin:
        if (*ss == '\0') {
          s = kBase64Decoding;
          ss = kEndOfRecordsBase64;
          base64_filter_.reset(new Base64Filter());
          client_.AddFilter(base64_filter_.get());
        } else if (*in == *ss) {
          ++ss;
          --m;
        } else {
          return Error("unexpected begin of records base64 marker", buf, in);
        }

        b = in;
        break;
#ifdef HAVE_ZLIB_H
      case kExpectBase64ZlibBegin:
        if (*ss == '\0') {
          s = kBase64Decoding;
          ss = kEndOfRecordsBase64Zlib;
          base64_filter_.reset(new Base64Filter());
          client_.AddFilter(base64_filter_.get());
          zlib_filter_.reset(new ZlibFilter());
          client_.AddFilter(zlib_filter_.get());
        } else if (*in == *ss) {
          ++ss;
          --m;
        } else {
          return Error("unexpected begin of records base64 zlib marker", buf,
                       in);
        }

        b = in;
        break;
#endif
      case kBase64Decoding:
        if (*in == *ss) {
          s = kExpectEndOfRecords;
          --m;
        }

        break;
      case kExpectEndOfRecords:
        if (*ss == '\0') {
          state_ = kDecodingDone;
          *buf = b;
          *n = m - static_cast<size_t>(end - in) - 1;
          return true;
        } else if (*in == *ss) {
          ++ss;
          --m;
        } else {
          return Error("unexpected end of records marker", buf, in);
        }

        break;
      default:
        state_ = kDecodingDone;
        *buf = nullptr;
        *n = 0;
        return true;
    }

    ++in;
  }

  state_ = s;
  sub_state_ = ss;
  consumed_ += *n;
  *buf = b;
  *n = m;
  return true;
}
