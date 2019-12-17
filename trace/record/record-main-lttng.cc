/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Ravindra Kumar Meena <rmeena840@gmail.com>
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

#include "config.h"

#include "client.h"

#include <getopt.h>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifdef HAVE_LLVM_DEBUGINFO_SYMBOLIZE_SYMBOLIZE_H
#include <llvm/DebugInfo/Symbolize/Symbolize.h>
#include <llvm/Support/Path.h>
#endif

#define CTF_MAGIC 0xC1FC1FC1
#define TASK_RUNNING 0x0000
#define TASK_IDLE 0x0402
#define UUID_SIZE 16
#define THREAD_NAME_SIZE 16
#define THREAD_API_COUNT 3
#define THREAD_ID_COUNT 0x10000
#define BITS_PER_CHAR 8
#define COMPACT_HEADER_ID 31

static const uint8_t kEmptyThreadName[THREAD_API_COUNT] = "";

static const uint8_t kUUID[] = {0x6a, 0x77, 0x15, 0xd0, 0xb5, 0x02, 0x4c, 0x65,
                                0x86, 0x78, 0x67, 0x77, 0xac, 0x7f, 0x75, 0x5a};

struct ClientItem {
  uint64_t ns;
  uint32_t cpu;
  rtems_record_event event;
  uint64_t data;
};

struct PacketHeader {
  uint32_t ctf_magic;
  uint8_t uuid[UUID_SIZE];
  uint32_t stream_id;
  uint64_t stream_instance_id;
} __attribute__((__packed__));

struct PacketContext {
  PacketHeader header;
  uint64_t timestamp_begin;
  uint64_t timestamp_end;
  uint64_t content_size;
  uint64_t packet_size;
  uint64_t packet_seq_num;
  uint64_t events_discarded;
  uint32_t cpu_id;
} __attribute__((__packed__));

static const size_t kPacketContextBits = sizeof(PacketContext) * BITS_PER_CHAR;

struct EventHeaderCompact {
  uint8_t id;
  uint32_t event_id;
  uint64_t ns;
} __attribute__((__packed__));

struct EventRecordItem {
  EventHeaderCompact header;
  uint64_t data;
} __attribute__((__packed__));

static const size_t kEventRecordItemBits =
    sizeof(EventRecordItem) * BITS_PER_CHAR;

struct EventSchedSwitch {
  EventHeaderCompact header;
  uint8_t prev_comm[THREAD_NAME_SIZE];
  int32_t prev_tid;
  int32_t prev_prio;
  int64_t prev_state;
  uint8_t next_comm[THREAD_NAME_SIZE];
  int32_t next_tid;
  int32_t next_prio;
} __attribute__((__packed__));

static const size_t kEventSchedSwitchBits =
    sizeof(EventSchedSwitch) * BITS_PER_CHAR;

struct EventIRQHandlerEntry {
  EventHeaderCompact header;
  int32_t irq;
  uint8_t name[1];
} __attribute__((__packed__));

static const size_t kEventIRQHandlerEntryBits =
    sizeof(EventIRQHandlerEntry) * BITS_PER_CHAR;

struct EventIRQHandlerExit {
  EventHeaderCompact header;
  int32_t irq;
  int32_t ret;
} __attribute__((__packed__));

static const size_t kEventIRQHandlerExitBits =
    sizeof(EventIRQHandlerExit) * BITS_PER_CHAR;

struct PerCPUContext {
  FILE* event_stream;
  uint64_t timestamp_begin;
  uint64_t timestamp_end;
  uint64_t size_in_bits;
  uint32_t thread_id;
  uint64_t thread_ns;
  size_t thread_name_index;
  EventRecordItem record_item;
  EventSchedSwitch sched_switch;
  EventIRQHandlerEntry irq_handler_entry;
  EventIRQHandlerExit irq_handler_exit;
};

class LTTNGClient : public Client {
 public:
  LTTNGClient() {
    Initialize(LTTNGClient::HandlerCaller);

    std::memset(&pkt_ctx_, 0, sizeof(pkt_ctx_));
    std::memcpy(pkt_ctx_.header.uuid, kUUID, sizeof(pkt_ctx_.header.uuid));
    pkt_ctx_.header.ctf_magic = CTF_MAGIC;

    for (size_t i = 0; i < RTEMS_RECORD_CLIENT_MAXIMUM_CPU_COUNT; ++i) {
      PerCPUContext& pcpu = per_cpu_[i];
      pcpu.sched_switch.header.id = COMPACT_HEADER_ID;
      pcpu.sched_switch.header.event_id = 1024;
      pcpu.irq_handler_entry.header.id = COMPACT_HEADER_ID;
      pcpu.irq_handler_entry.header.event_id = 1025;
      pcpu.irq_handler_entry.name[0] = '\0';
      pcpu.irq_handler_exit.header.id = COMPACT_HEADER_ID;
      pcpu.irq_handler_exit.header.event_id = 1026;
      pcpu.irq_handler_exit.ret = 1;
      pcpu.record_item.header.id = COMPACT_HEADER_ID;
    }
  }

  void OpenExecutable(const char* elf_file);

  void Destroy() {
    Client::Destroy();
    CloseStreamFiles();
  }

 private:
  PerCPUContext per_cpu_[RTEMS_RECORD_CLIENT_MAXIMUM_CPU_COUNT];

  /*
   * @brief Thread names indexed by API and object index.
   *
   * The API indices are 0 for Internal API, 1 for Classic API and 2 for
   * POSIX API.
   */
  uint8_t thread_names_[THREAD_API_COUNT][THREAD_ID_COUNT][THREAD_NAME_SIZE];

  PacketContext pkt_ctx_;

  size_t cpu_count_ = 0;

#ifdef HAVE_LLVM_DEBUGINFO_SYMBOLIZE_SYMBOLIZE_H
  llvm::symbolize::LLVMSymbolizer symbolizer_;
#endif

  std::string elf_file_;

  bool resolve_address_ = false;

  typedef std::map<uint64_t, std::vector<char>> AddressToLineMap;

  AddressToLineMap address_to_line_;

  static rtems_record_client_status HandlerCaller(uint64_t bt,
                                                  uint32_t cpu,
                                                  rtems_record_event event,
                                                  uint64_t data,
                                                  void* arg) {
    LTTNGClient& self = *static_cast<LTTNGClient*>(arg);
    return self.Handler(bt, cpu, event, data);
  }

  rtems_record_client_status Handler(uint64_t bt,
                                     uint32_t cpu,
                                     rtems_record_event event,
                                     uint64_t data);

  void CopyThreadName(const ClientItem& item,
                      size_t api_index,
                      uint8_t* dst) const;

  void WriteRecordItem(PerCPUContext* pcpu, const ClientItem& item);

  void WriteSchedSwitch(PerCPUContext* pcpu, const ClientItem& item);

  void WriteIRQHandlerEntry(PerCPUContext* pcpu, const ClientItem& item);

  void WriteIRQHandlerExit(PerCPUContext* pcpu, const ClientItem& item);

  void AddThreadName(PerCPUContext* pcpu, const ClientItem& item);

  void PrintItem(const ClientItem& item);

  void OpenStreamFiles(uint64_t data);

  void CloseStreamFiles();

  AddressToLineMap::iterator AddAddressAsHexNumber(const ClientItem& item);

  AddressToLineMap::iterator ResolveAddress(const ClientItem& item);
};

static uint32_t GetAPIIndexOfID(uint32_t id) {
  return ((id >> 24) & 0x7) - 1;
}

static uint32_t GetObjIndexOfID(uint32_t id) {
  return id & (THREAD_ID_COUNT - 1);
}

static bool IsIdleTaskByAPIIndex(uint32_t api_index) {
  return api_index == 0;
}

static const uint8_t kCPUPostfix[RTEMS_RECORD_CLIENT_MAXIMUM_CPU_COUNT][2] = {
    {'0', '\0'}, {'1', '\0'}, {'2', '\0'}, {'3', '\0'}, {'4', '\0'},
    {'5', '\0'}, {'6', '\0'}, {'7', '\0'}, {'8', '\0'}, {'9', '\0'},
    {'1', '0'},  {'1', '1'},  {'1', '2'},  {'1', '3'},  {'1', '4'},
    {'1', '5'},  {'1', '6'},  {'1', '7'},  {'1', '8'},  {'1', '9'},
    {'2', '0'},  {'2', '1'},  {'2', '2'},  {'2', '3'},  {'2', '4'},
    {'2', '5'},  {'2', '6'},  {'2', '7'},  {'2', '8'},  {'2', '9'},
    {'3', '0'},  {'3', '1'}};

void LTTNGClient::OpenExecutable(const char* elf_file) {
  elf_file_ = elf_file;
  resolve_address_ = true;
}

void LTTNGClient::CopyThreadName(const ClientItem& item,
                                 size_t api_index,
                                 uint8_t* dst) const {
  const uint8_t* name;

  if (api_index < THREAD_API_COUNT) {
    name = thread_names_[api_index][GetObjIndexOfID(item.data)];
  } else {
    name = kEmptyThreadName;
  }

  std::memcpy(dst, name, THREAD_NAME_SIZE);

  if (IsIdleTaskByAPIIndex(api_index)) {
    /*
     * In Linux, the idle threads are bound to a specific CPU (swapper/n).  In
     * RTEMS, the idle threads can move around, so mimic this Linux behaviour.
     */
    dst[4] = '/';
    dst[5] = kCPUPostfix[item.cpu][0];
    dst[6] = kCPUPostfix[item.cpu][1];
  }
}

static bool IsCodeEvent(rtems_record_event event) {
  switch (event) {
    default:
      return false;
    case RTEMS_RECORD_CALLER:
    case RTEMS_RECORD_FUNCTION_ENTRY:
    case RTEMS_RECORD_FUNCTION_EXIT:
    case RTEMS_RECORD_ISR_DISABLE:
    case RTEMS_RECORD_ISR_ENABLE:
    case RTEMS_RECORD_LINE:
    case RTEMS_RECORD_THREAD_DISPATCH_DISABLE:
    case RTEMS_RECORD_THREAD_DISPATCH_ENABLE:
      return true;
  }
}

LTTNGClient::AddressToLineMap::iterator LTTNGClient::AddAddressAsHexNumber(
    const ClientItem& item) {
  char hex[19];
  int n = std::snprintf(hex, sizeof(hex), "0x%" PRIx64, item.data);
  assert(static_cast<size_t>(n) < sizeof(hex));
  std::vector<char> code(hex, hex + n + 1);
  return address_to_line_.emplace(item.data, std::move(code)).first;
}

LTTNGClient::AddressToLineMap::iterator LTTNGClient::ResolveAddress(
    const ClientItem& item) {
#ifdef HAVE_LLVM_DEBUGINFO_SYMBOLIZE_SYMBOLIZE_H
  if (resolve_address_) {
    auto res_or_err = symbolizer_.symbolizeCode(elf_file_,
#if LLVM_VERSION_MAJOR >= 9
      {item.data, llvm::object::SectionedAddress::UndefSection});
#else
      item.data);
#endif

    if (res_or_err) {
      auto info = res_or_err.get();
      std::string fn = info.FunctionName;
      std::string str;

      if (fn != "<invalid>") {
        str += fn;
        str += " at ";
      }

      str += llvm::sys::path::filename(info.FileName);
      str += ":";
      str += std::to_string(info.Line);
      std::vector<char> code(str.begin(), str.end());
      code.push_back('\0');
      return address_to_line_.emplace(item.data, std::move(code)).first;
    }
  }
#endif

  return AddAddressAsHexNumber(item);
}

void LTTNGClient::WriteRecordItem(PerCPUContext* pcpu, const ClientItem& item) {
  if (IsCodeEvent(item.event)) {
    EventHeaderCompact header;
    header.id = COMPACT_HEADER_ID;
    header.event_id = item.event;
    header.ns = item.ns;

    auto it = address_to_line_.find(item.data);
    if (it == address_to_line_.end()) {
      it = ResolveAddress(item);
    }

    pcpu->size_in_bits += (sizeof(header) + it->second.size()) * BITS_PER_CHAR;

    std::fwrite(&header, sizeof(header), 1, pcpu->event_stream);
    std::fwrite(&(*it->second.begin()), it->second.size(), 1,
                pcpu->event_stream);
  } else {
    pcpu->size_in_bits += kEventRecordItemBits;

    EventRecordItem& ri = pcpu->record_item;
    ri.header.ns = item.ns;
    ri.header.event_id = item.event;
    ri.data = item.data;

    std::fwrite(&ri, sizeof(ri), 1, pcpu->event_stream);
  }
}

void LTTNGClient::WriteSchedSwitch(PerCPUContext* pcpu,
                                   const ClientItem& item) {
  pcpu->size_in_bits += kEventSchedSwitchBits;

  EventSchedSwitch& ss = pcpu->sched_switch;
  ss.header.ns = item.ns;

  uint32_t api_index = GetAPIIndexOfID(item.data);
  ss.next_tid = IsIdleTaskByAPIIndex(api_index) ? 0 : item.data;

  CopyThreadName(item, api_index, ss.next_comm);
  std::fwrite(&ss, sizeof(ss), 1, pcpu->event_stream);
}

void LTTNGClient::WriteIRQHandlerEntry(PerCPUContext* pcpu,
                                       const ClientItem& item) {
  pcpu->size_in_bits += kEventIRQHandlerEntryBits;

  EventIRQHandlerEntry& ih = pcpu->irq_handler_entry;
  ih.header.ns = item.ns;
  ih.irq = static_cast<int32_t>(item.data);
  std::fwrite(&ih, sizeof(ih), 1, pcpu->event_stream);
}

void LTTNGClient::WriteIRQHandlerExit(PerCPUContext* pcpu,
                                      const ClientItem& item) {
  pcpu->size_in_bits += kEventIRQHandlerExitBits;

  EventIRQHandlerExit& ih = pcpu->irq_handler_exit;
  ih.header.ns = item.ns;
  ih.irq = static_cast<int32_t>(item.data);
  std::fwrite(&ih, sizeof(ih), 1, pcpu->event_stream);
}

void LTTNGClient::AddThreadName(PerCPUContext* pcpu, const ClientItem& item) {
  if (pcpu->thread_name_index >= THREAD_NAME_SIZE) {
    return;
  }

  uint32_t api_index = GetAPIIndexOfID(pcpu->thread_id);
  if (api_index >= THREAD_API_COUNT) {
    return;
  }

  uint32_t obj_index = GetObjIndexOfID(pcpu->thread_id);
  uint64_t name = item.data;
  size_t i;
  for (i = pcpu->thread_name_index; i < pcpu->thread_name_index + data_size();
       ++i) {
    thread_names_[api_index][obj_index][i] = static_cast<uint8_t>(name);
    name >>= BITS_PER_CHAR;
  }

  pcpu->thread_name_index = i;
}

void LTTNGClient::PrintItem(const ClientItem& item) {
  PerCPUContext& pcpu = per_cpu_[item.cpu];
  if (pcpu.timestamp_begin == 0) {
    pcpu.timestamp_begin = item.ns;
  }

  pcpu.timestamp_end = item.ns;

  EventSchedSwitch& ss = pcpu.sched_switch;
  switch (item.event) {
    case RTEMS_RECORD_THREAD_SWITCH_OUT: {
      uint32_t api_index = GetAPIIndexOfID(item.data);
      ss.header.ns = item.ns;

      if (IsIdleTaskByAPIIndex(api_index)) {
        ss.prev_tid = 0;
        ss.prev_state = TASK_IDLE;
      } else {
        ss.prev_tid = item.data;
        ss.prev_state = TASK_RUNNING;
      }

      CopyThreadName(item, api_index, ss.prev_comm);
      break;
    }
    case RTEMS_RECORD_THREAD_SWITCH_IN:
      if (item.ns == ss.header.ns) {
        WriteSchedSwitch(&pcpu, item);
      }
      break;
    case RTEMS_RECORD_THREAD_ID:
      pcpu.thread_id = item.data;
      pcpu.thread_ns = item.ns;
      pcpu.thread_name_index = 0;
      break;
    case RTEMS_RECORD_INTERRUPT_ENTRY:
      WriteIRQHandlerEntry(&pcpu, item);
      break;
    case RTEMS_RECORD_INTERRUPT_EXIT:
      WriteIRQHandlerExit(&pcpu, item);
      break;
    case RTEMS_RECORD_THREAD_NAME:
      AddThreadName(&pcpu, item);
      break;
    case RTEMS_RECORD_PROCESSOR_MAXIMUM:
      OpenStreamFiles(item.data);
      break;
    default:
      if (item.ns != 0) {
        WriteRecordItem(&pcpu, item);
      }
      break;
  }
}

rtems_record_client_status LTTNGClient::Handler(uint64_t bt,
                                                uint32_t cpu,
                                                rtems_record_event event,
                                                uint64_t data) {
  ClientItem item;

  item.ns = rtems_record_client_bintime_to_nanoseconds(bt);
  item.cpu = cpu;
  item.event = event;
  item.data = data;

  PrintItem(item);

  return RTEMS_RECORD_CLIENT_SUCCESS;
}

void LTTNGClient::OpenStreamFiles(uint64_t data) {
  // Assertions are ensured by C record client
  assert(cpu_count_ == 0 && data < RTEMS_RECORD_CLIENT_MAXIMUM_CPU_COUNT);
  cpu_count_ = static_cast<size_t>(data) + 1;

  for (size_t i = 0; i < cpu_count_; ++i) {
    std::string filename("stream_");
    filename += std::to_string(i);
    FILE* f = std::fopen(filename.c_str(), "wb");
    if (f == NULL) {
      throw ErrnoException("cannot create file '" + filename + "'");
    }
    per_cpu_[i].event_stream = f;
    std::fwrite(&pkt_ctx_, sizeof(pkt_ctx_), 1, f);
  }
}

void LTTNGClient::CloseStreamFiles() {
  for (size_t i = 0; i < cpu_count_; ++i) {
    PerCPUContext* pcpu = &per_cpu_[i];
    std::fseek(pcpu->event_stream, 0, SEEK_SET);

    pkt_ctx_.header.stream_instance_id = i;
    pkt_ctx_.timestamp_begin = pcpu->timestamp_begin;
    pkt_ctx_.timestamp_end = pcpu->timestamp_end;
    pkt_ctx_.content_size = pcpu->size_in_bits + kPacketContextBits;
    pkt_ctx_.packet_size = pkt_ctx_.content_size;
    pkt_ctx_.cpu_id = i;

    std::fwrite(&pkt_ctx_, sizeof(pkt_ctx_), 1, pcpu->event_stream);
    std::fclose(pcpu->event_stream);
  }
}

static const char kMetadata[] =
    "/* CTF 1.8 */\n"
    "\n"
    "typealias integer { size = 5; align = 1; signed = false; } := uint5_t;\n"
    "typealias integer { size = 8; align = 8; signed = false; } := uint8_t;\n"
    "typealias integer { size = 32; align = 8; signed = true; } := int32_t;\n"
    "typealias integer { size = 32; align = 8; signed = false; } := uint32_t;\n"
    "typealias integer { size = 64; align = 8; signed = true; } := int64_t;\n"
    "typealias integer { size = 64; align = 8; signed = false; } := uint64_t;\n"
    "\n"
    "typealias integer {\n"
    "\tsize = 64; align = 8; signed = false; base = 16;\n"
    "} := xint64_t;\n"
    "\n"
    "typealias integer {\n"
    "\tsize = 8; align = 8; signed = false; encoding = UTF8; base = 10;\n"
    "} := utf8_t;\n"
    "\n"
    "typealias integer {\n"
    "\tsize = 27; align = 1; signed = false;\n"
    "\tmap = clock.monotonic.value;\n"
    "} := uint27_clock_monotonic_t;\n"
    "\n"
    "typealias integer {\n"
    "\tsize = 64; align = 8; signed = false;\n"
    "\tmap = clock.monotonic.value;\n"
    "} := uint64_clock_monotonic_t;\n"
    "\n"
    "trace {\n"
    "\tmajor = 1;\n"
    "\tminor = 8;\n"
    "\tuuid = \"6a7715d0-b502-4c65-8678-6777ac7f755a\";\n"
    "\tbyte_order = le;\n"
    "\tpacket.header := struct {\n"
    "\t\tuint32_t magic;\n"
    "\t\tuint8_t  uuid[16];\n"
    "\t\tuint32_t stream_id;\n"
    "\t\tuint64_t stream_instance_id;\n"
    "\t};\n"
    "};\n"
    "\n"
    "env {\n"
    "\thostname = \"RTEMS\";\n"
    "\tdomain = \"kernel\";\n"
    "\tsysname = \"Linux\";\n"
    "\tkernel_release = \"5\";\n"
    "\tkernel_version = \"0\";\n"
    "\ttracer_name = \"lttng-modules\";\n"
    "\ttracer_major = 2;\n"
    "\ttracer_minor = 11;\n"
    "\ttracer_patchlevel = 0;\n"
    "};\n"
    "\n"
    "clock {\n"
    "\tname = \"monotonic\";\n"
    "\tuuid = \"234d669d-7651-4bc1-a7fd-af581ecc6232\";\n"
    "\tdescription = \"Monotonic Clock\";\n"
    "\tfreq = 1000000000;\n"
    "\toffset = 0;\n"
    "};\n"
    "\n"
    "struct packet_context {\n"
    "\tuint64_clock_monotonic_t timestamp_begin;\n"
    "\tuint64_clock_monotonic_t timestamp_end;\n"
    "\tuint64_t content_size;\n"
    "\tuint64_t packet_size;\n"
    "\tuint64_t packet_seq_num;\n"
    "\tuint64_t events_discarded;\n"
    "\tuint32_t cpu_id;\n"
    "};\n"
    "\n"
    "struct event_header_compact {\n"
    "\tenum : uint5_t { compact = 0 ... 30, extended = 31 } id;\n"
    "\tvariant <id> {\n"
    "\t\tstruct {\n"
    "\t\t\tuint27_clock_monotonic_t timestamp;\n"
    "\t\t} compact;\n"
    "\t\tstruct {\n"
    "\t\t\tuint32_t id;\n"
    "\t\t\tuint64_clock_monotonic_t timestamp;\n"
    "\t\t} extended;\n"
    "\t} v;\n"
    "} align(8);\n"
    "\n"
    "stream {\n"
    "\tid = 0;\n"
    "\tevent.header := struct event_header_compact;\n"
    "\tpacket.context := struct packet_context;\n"
    "};\n"
    "\n"
    "event {\n"
    "\tname = sched_switch;\n"
    "\tid = 1024;\n"
    "\tstream_id = 0;\n"
    "\tfields := struct {\n"
    "\t\tutf8_t _prev_comm[16];\n"
    "\t\tint32_t _prev_tid;\n"
    "\t\tint32_t _prev_prio;\n"
    "\t\tint64_t _prev_state;\n"
    "\t\tutf8_t _next_comm[16];\n"
    "\t\tint32_t _next_tid;\n"
    "\t\tint32_t _next_prio;\n"
    "\t};\n"
    "};\n"
    "\n"
    "event {\n"
    "\tname = irq_handler_entry;\n"
    "\tid = 1025;\n"
    "\tstream_id = 0;\n"
    "\tfields := struct {\n"
    "\t\tint32_t _irq;\n"
    "\t\tstring _name;\n"
    "\t};\n"
    "};\n"
    "\n"
    "event {\n"
    "\tname = irq_handler_exit;\n"
    "\tid = 1026;\n"
    "\tstream_id = 0;\n"
    "\tfields := struct {\n"
    "\t\tint32_t _irq;\n"
    "\t\tint32_t _ret;\n"
    "\t};\n"
    "};\n";

static void GenerateMetadata() {
  FILE* f = std::fopen("metadata", "w");
  if (f == NULL) {
    throw ErrnoException("cannot create file 'metadata'");
  }

  std::fwrite(kMetadata, sizeof(kMetadata) - 1, 1, f);

  for (int i = 0; i <= RTEMS_RECORD_LAST; ++i) {
    if (IsCodeEvent(static_cast<rtems_record_event>(i))) {
      std::fprintf(f,
                   "\n"
                   "event {\n"
                   "\tname = %s;\n"
                   "\tid = %i;\n"
                   "\tstream_id = 0;\n"
                   "\tfields := struct {\n"
                   "\t\tstring _code;\n"
                   "\t};\n"
                   "};\n",
                   rtems_record_event_text(static_cast<rtems_record_event>(i)),
                   i);
    } else {
      std::fprintf(f,
                   "\n"
                   "event {\n"
                   "\tname = %s;\n"
                   "\tid = %i;\n"
                   "\tstream_id = 0;\n"
                   "\tfields := struct {\n"
                   "\t\txint64_t _data;\n"
                   "\t};\n"
                   "};\n",
                   rtems_record_event_text(static_cast<rtems_record_event>(i)),
                   i);
    }
  }

  std::fclose(f);
}

static LTTNGClient client;

static void SignalHandler(int s) {
  client.RequestStop();
  std::signal(s, SIG_DFL);
}

static const struct option kLongOpts[] = {{"help", 0, NULL, 'h'},
                                          {"host", 1, NULL, 'H'},
                                          {"port", 1, NULL, 'p'},
                                          {NULL, 0, NULL, 0}};

static void Usage(char** argv) {
  std::cout
      << argv[0]
      << " [--host=HOST] [--port=PORT] [--limit=LIMIT] [--elf=ELF] [INPUT-FILE]"
      << std::endl
      << std::endl
      << "Mandatory arguments to long options are mandatory for short "
         "options too."
      << std::endl
      << "  -h, --help                 print this help text" << std::endl
      << "  -H, --host=HOST            the host IPv4 address of the "
         "record server"
      << std::endl
      << "  -p, --port=PORT            the TCP port of the record server"
      << std::endl
      << "  -l, --limit=LIMIT          limit in bytes to process" << std::endl
      << "  -e, --elf=ELF              the ELF executable file" << std::endl
      << "  INPUT-FILE                 the input file" << std::endl;
}

int main(int argc, char** argv) {
  const char* host = "127.0.0.1";
  uint16_t port = 1234;
  const char* elf_file = nullptr;
  const char* input_file = nullptr;
  int opt;
  int longindex;

  while ((opt = getopt_long(argc, argv, "e:hH:l:p:", &kLongOpts[0],
                            &longindex)) != -1) {
    switch (opt) {
      case 'e':
        elf_file = optarg;
        break;
      case 'h':
        Usage(argv);
        return 0;
      case 'H':
        host = optarg;
        break;
      case 'l':
        client.set_limit(strtoull(optarg, NULL, 0));
        break;
      case 'p':
        port = (uint16_t)strtoul(optarg, NULL, 0);
        break;
      default:
        return 1;
    }
  }

  if (optind == argc - 1) {
    input_file = argv[optind];
    ++optind;
  }

  if (optind != argc) {
    std::cerr << argv[0] << ": unrecognized options:";
    for (int i = optind; i < argc; ++i) {
      std::cerr << " '" << argv[i] << "'";
    }
    std::cerr << std::endl;
    return 1;
  }

  try {
    GenerateMetadata();

    if (elf_file != nullptr) {
      client.OpenExecutable(elf_file);
    }

    if (input_file != nullptr) {
      client.Open(input_file);
    } else {
      client.Connect(host, port);
    }

    std::signal(SIGINT, SignalHandler);
    client.Run();
    client.Destroy();
  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
