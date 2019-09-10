/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (C) 2018, 2019 embedded brains GmbH (http://www.embedded-brains.de)
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

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

class ErrnoException : public std::runtime_error {
 public:
  ErrnoException(std::string msg)
      : std::runtime_error(msg + ": " + std::strerror(errno)) {
    // Nothing to do
  }
};

class FileDescriptor {
 public:
  FileDescriptor() = default;

  FileDescriptor(const FileDescriptor&) = delete;

  FileDescriptor& operator=(const FileDescriptor&) = delete;

  void Open(const char* file);

  void Connect(const char* host, uint16_t port);

  ssize_t Read(void* buf, size_t n) { return (*reader_)(fd_, buf, n); }

  void Destroy();

 private:
  int fd_ = -1;
  ssize_t (*reader_)(int fd, void* buf, size_t n) = nullptr;
};

class ConfigFile {
 public:
  static const std::string kNoError;

  ConfigFile() = default;

  ConfigFile(const ConfigFile&) = delete;

  ConfigFile& operator=(const ConfigFile&) = delete;

  typedef std::string (*Parser)(void* arg, const char* name, const char* value);

  void AddParser(const char* section, Parser parser, void* arg);

  void Parse(const char* file);

 private:
  std::map<std::string, std::pair<Parser, void*> > parser_;

  std::string error_;

  static int INIHandler(void* user,
                        const char* section,
                        const char* name,
                        const char* value);
};

class Client {
 public:
  Client() = default;

  Client(const Client&) = delete;

  Client& operator=(const Client&) = delete;

  void Open(const char* file) { input_.Open(file); }

  void Connect(const char* host, uint16_t port) { input_.Connect(host, port); }

  void Run();

  void RequestStop() { stop_ = 1; }

  void Destroy();

  void set_limit(uint64_t limit) { limit_ = limit; }

 protected:
  void Initialize(rtems_record_client_handler handler) {
    rtems_record_client_init(&base_, handler, this);
  }

  size_t data_size() const { return base_.data_size; };

 private:
  rtems_record_client_context base_;
  FileDescriptor input_;
  sig_atomic_t stop_ = 0;
  uint64_t limit_ = 0;
};

#endif  // RTEMS_TOOLS_TRACE_RECORD_CLIENT_H_
