/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2018, 2019 embedded brains GmbH
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

#ifndef RTEMS_TOOLS_TRACE_RECORD_CLIENT_H_
#define RTEMS_TOOLS_TRACE_RECORD_CLIENT_H_

#include <rtems/recordclient.h>
#include <rtems/recorddata.h>

#include <sys/types.h>

#include <csignal>

class Client {
 public:
  Client() = default;

  Client(const Client&) = delete;

  Client& operator=(const Client&) = delete;

  void Open(const char* file);

  void Connect(const char* host, uint16_t port);

  void Run();

  void RequestStop() { stop_ = 1; }

  void Destroy();

 protected:
  void Initialize(rtems_record_client_handler handler) {
    rtems_record_client_init(&base_, handler, this);
  }

  size_t data_size() const { return base_.data_size; };

 private:
  rtems_record_client_context base_;
  int fd_ = -1;
  ssize_t (*reader_)(int fd, void* buf, size_t n) = nullptr;
  sig_atomic_t stop_ = 0;
};

#endif  // RTEMS_TOOLS_TRACE_RECORD_CLIENT_H_
