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

#include "client.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

static ssize_t ReadFile(int fd, void* buf, size_t n) {
  return read(fd, buf, n);
}

static ssize_t ReadSocket(int fd, void* buf, size_t n) {
  return recv(fd, buf, n, 0);
}

void Client::Open(const char* file) {
  assert(fd_ == -1);
  fd_ = open(file, O_RDONLY);
  assert(fd_ >= 0);
  reader_ = ReadFile;
}

void Client::Connect(const char* host, uint16_t port) {
  assert(fd_ == -1);
  fd_ = socket(PF_INET, SOCK_STREAM, 0);
  assert(fd_ >= 0);

  struct sockaddr_in in_addr;
  memset(&in_addr, 0, sizeof(in_addr));
  in_addr.sin_family = AF_INET;
  in_addr.sin_port = htons(port);
  in_addr.sin_addr.s_addr = inet_addr(host);

  int rv = connect(fd_, (struct sockaddr*)&in_addr, sizeof(in_addr));
  assert(rv == 0);

  reader_ = ReadSocket;
}

void Client::Run() {
  while (stop_ == 0) {
    int buf[8192];
    ssize_t n = (*reader_)(fd_, buf, sizeof(buf));

    if (n > 0) {
      rtems_record_client_run(&base_, buf, static_cast<size_t>(n));
    } else {
      break;
    }
  }
}

void Client::Destroy() {
  int rv = close(fd_);
  assert(rv == 0);

  rtems_record_client_destroy(&base_);
}
