/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (C) 2018, 2020 embedded brains GmbH (http://www.embedded-brains.de)
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

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <cassert>
#include <cstring>

#include <ini.h>

static ssize_t ReadFile(int fd, void* buf, size_t n) {
  return ::read(fd, buf, n);
}

static ssize_t ReadSocket(int fd, void* buf, size_t n) {
  // This cast is necessary for Windows
  return ::recv(fd, static_cast<char*>(buf), n, 0);
}

void FileDescriptor::Open(const char* file) {
  assert(fd_ == -1);

  int oflag = O_RDONLY;
#ifdef _WIN32
  oflag |= O_BINARY;
#endif
  fd_ = ::open(file, oflag);
  if (fd_ < 0) {
    throw ErrnoException(std::string("cannot open file '") + file + "'");
  }

  reader_ = ReadFile;
}

void FileDescriptor::Connect(const char* host, uint16_t port) {
  assert(fd_ == -1);

  fd_ = ::socket(PF_INET, SOCK_STREAM, 0);
  if (fd_ < 0) {
    throw ErrnoException("cannot open socket");
  }

  struct sockaddr_in in_addr;
  std::memset(&in_addr, 0, sizeof(in_addr));
  in_addr.sin_family = AF_INET;
  in_addr.sin_port = htons(port);
  in_addr.sin_addr.s_addr = inet_addr(host);

  int rv = ::connect(fd_, (struct sockaddr*)&in_addr, sizeof(in_addr));
  if (rv != 0) {
    throw ErrnoException(std::string("cannot connect to ") + host + " port " +
                         std::to_string(port));
  }

  reader_ = ReadSocket;
}

void FileDescriptor::Destroy() {
  if (fd_ != -1) {
    int rv = ::close(fd_);
    if (rv != 0) {
      std::cerr << "close failed: " << strerror(errno) << std::endl;
    }
  }
}

const std::string ConfigFile::kNoError;

void ConfigFile::AddParser(const char* section, Parser parser, void* arg) {
  parser_[section] = std::make_pair(parser, arg);
}

void ConfigFile::Parse(const char* file) {
  int status = ini_parse(file, INIHandler, this);
  if (status < 0) {
    throw ErrnoException(std::string("cannot parse configuration file '") +
                         file + "'");
  } else if (status > 0) {
    throw std::runtime_error(
        std::string("invalid line ") + std::to_string(status) +
        " in configuration file '" + file + "': " + error_);
  }
}

int ConfigFile::INIHandler(void* user,
                           const char* section,
                           const char* name,
                           const char* value) {
  ConfigFile* self = static_cast<ConfigFile*>(user);
  auto it = self->parser_.find(section);
  if (it != self->parser_.end()) {
    std::string error = (*it->second.first)(it->second.second, name, value);
    if (error == kNoError) {
      return 1;
    }

    self->error_ = error;
  } else {
    self->error_ = std::string("unknown section: ") + section;
  }

  return 0;
}

void Client::Flush() {
  while (true) {
    void* p = nullptr;
    size_t n = 0;
    for (auto filter : filters_) {
      if (!filter->Run(&p, &n)) {
        break;
      }
    }

    if (n > 0) {
      rtems_record_client_run(&base_, p, n);
    } else {
      break;
    }
  }
}

void Client::Run() {
  uint64_t todo = UINT64_MAX;

  if (limit_ != 0) {
    todo = limit_;
  }

  while (stop_ == 0 && todo > 0) {
    long buf[8192];
    size_t m = std::min(static_cast<uint64_t>(sizeof(buf)), todo);
    ssize_t n = input_.Read(buf, m);
    if (n <= 0) {
      break;
    }

    void* p = &buf[0];
    size_t k = static_cast<size_t>(n);
    for (auto filter : filters_) {
      if (!filter->Run(&p, &k)) {
        std::cerr << "error: input filter failure" << std::endl;
        return;
      }
    }

    rtems_record_client_run(&base_, p, k);
    todo -= static_cast<size_t>(n);
  }

  Flush();
}

void Client::Destroy() {
  input_.Destroy();
  rtems_record_client_destroy(&base_);
}
