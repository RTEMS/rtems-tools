/* SPDX-License-Identifier: ISC */

/*
 * Copyright (C) 2020 embedded brains GmbH (http://www.embedded-brains.de)
 * Copyright (C) 2004, 2005, 2007, 2009  Internet Systems Consortium, Inc.
 * ("ISC") Copyright (C) 1998-2001, 2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

#include <cstring>

static const char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

bool Base64Filter::DecodeChar(int c, char **target) {
  char* s;

  if (seen_end_)
    return false;
  if ((s = std::strchr(const_cast<char*>(base64), c)) == NULL)
    return false;
  val_[digits_++] = s - base64;
  if (digits_ == 4) {
    int n;
    unsigned char buf[3];
    if (val_[0] == 64 || val_[1] == 64)
      return false;
    if (val_[2] == 64 && val_[3] != 64)
      return false;
    /*
     * Check that bits that should be zero are.
     */
    if (val_[2] == 64 && (val_[1] & 0xf) != 0)
      return false;
    /*
     * We don't need to test for val_[2] != 64 as
     * the bottom two bits of 64 are zero.
     */
    if (val_[3] == 64 && (val_[2] & 0x3) != 0)
      return false;
    n = (val_[2] == 64) ? 1 : (val_[3] == 64) ? 2 : 3;
    if (n != 3) {
      seen_end_ = true;
      if (val_[2] == 64)
        val_[2] = 0;
      if (val_[3] == 64)
        val_[3] = 0;
    }
    buf[0] = (val_[0] << 2) | (val_[1] >> 4);
    buf[1] = (val_[1] << 4) | (val_[2] >> 2);
    buf[2] = (val_[2] << 6) | (val_[3]);
    char *out = *target;
    for (int i = 0; i < n; ++i) {
      out[i] = buf[i];
    }
    *target = out + n;
    digits_ = 0;
  }
  return true;
}

bool Base64Filter::Run(void** buf, size_t* n) {
  char* begin = static_cast<char*>(*buf);
  const char* in = begin;
  const char* end = in + *n;

  if (begin == end) {
    return digits_ == 0;
  }

  char* target = begin;
  while (in != end) {
    int c = *in;
    ++in;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      continue;
    }

    if (!DecodeChar(c, &target)) {
      return false;
    };
  }

  *n = static_cast<size_t>(target - begin);
  return true;
}
