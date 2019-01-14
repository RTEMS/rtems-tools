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

#include <rtems/recorddata.h>
#include <rtems/recordclient.h>

#include <sys/queue.h>
#include <sys/socket.h>

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <netinet/in.h>
#include <arpa/inet.h>

static const struct option longopts[] = {
  { "help", 0, NULL, 'h' },
  { "host", 1, NULL, 'H' },
  { "port", 1, NULL, 'p' },
  { NULL, 0, NULL, 0 }
};

typedef struct client_item {
  uint64_t           ns;
  uint32_t           cpu;
  rtems_record_event event;
  uint64_t           data;
} client_item;

typedef struct client_context {
  int dummy;
} client_context;

static void usage( char **argv )
{
  printf(
    "%s [--host=HOST] [--port=PORT]\n"
    "\n"
    "Mandatory arguments to long options are mandatory for short options too.\n"
    "  -h, --help                 print this help text\n"
    "  -H, --host=HOST            the host IPv4 address of the record server\n"
    "  -p, --port=PORT            the TCP port of the record server\n"
    argv[ 0 ]
  );
}

static int connect_client( const char *host, uint16_t port )
{
  struct sockaddr_in in_addr;
  int fd;
  int rv;

  fd = socket( PF_INET, SOCK_STREAM, 0 );
  assert( fd >= 0 );

  memset( &in_addr, 0, sizeof( in_addr ) );
  in_addr.sin_family = AF_INET;
  in_addr.sin_port = htons( port );
  in_addr.sin_addr.s_addr = inet_addr( host );
  rv = connect( fd, (struct sockaddr *) &in_addr, sizeof( in_addr ) );
  assert( rv == 0 );

  return fd;
}

static void print_item( FILE *f, const client_item *item )
{
  if ( item->ns != 0 ) {
    uint32_t seconds;
    uint32_t nanoseconds;

    seconds = (uint32_t) ( item->ns / 1000000000 );
    nanoseconds = (uint32_t) ( item->ns % 1000000000 );
    fprintf( f, "%" PRIu32 ".%09" PRIu32 ":", seconds, nanoseconds );
  } else {
    fprintf( f, "*:" );
  }

  fprintf(
    f,
    "%" PRIu32 ":%s:%" PRIx64 "\n",
    item->cpu,
    rtems_record_event_text( item->event ),
    item->data
  );
}

static rtems_record_client_status handler(
  uint64_t            bt,
  uint32_t            cpu,
  rtems_record_event  event,
  uint64_t            data,
  void               *arg
)
{
  client_context *cctx;
  client_item     item;

  cctx = arg;
  (void) cctx;

  item.ns = rtems_record_client_bintime_to_nanoseconds( bt );
  item.cpu = cpu;
  item.event = event;
  item.data = data;

  print_item( stdout, &item );

  return RTEMS_RECORD_CLIENT_SUCCESS;
}

int main( int argc, char **argv )
{
  rtems_record_client_context ctx;
  client_context cctx;
  const char *host;
  uint16_t port;
  int fd;
  int rv;
  int opt;
  int longindex;

  host = "127.0.0.1";
  port = 1234;

  while (
    ( opt = getopt_long( argc, argv, "hH:p:", &longopts[0], &longindex ) )
      != -1
  ) {
    switch ( opt ) {
      case 'h':
        usage( argv );
        exit( EXIT_SUCCESS );
        break;
      case 'H':
        host = optarg;
        break;
      case 'p':
        port = (uint16_t) strtoul( optarg, NULL, 10 );
        break;
      default:
        exit( EXIT_FAILURE );
        break;
    }
  }

  memset( &cctx, 0, sizeof( cctx ) );

  fd = connect_client( host, port );
  rtems_record_client_init( &ctx, handler, &cctx );

  while ( true ) {
    int buf[ 8192 ];
    ssize_t n;

    n = recv( fd, buf, sizeof( buf ), 0 );
    if ( n >= 0 ) {
      rtems_record_client_run( &ctx, buf, (size_t) n );
    } else {
      break;
    }
  }

  rv = close( fd );
  assert( rv == 0 );

  return 0;
}
