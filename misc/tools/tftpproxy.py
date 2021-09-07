#
# Copyright 2019, 2020 Chris Johns (chris@contemporary.software)
# All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#
# The TFTP proxy redirects a TFTP session to another host. If you have a
# farm of boards you can configure them to point to this proxy and it will
# redirect the requests to another machine that is testing it.
#

from __future__ import print_function

import argparse
import os
import socket
import sys
import time
import threading

try:
    import socketserver
except:
    import SocketServer as socketserver

from rtemstoolkit import configuration
from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import version

import misc.tools.getmac

def host_port_split(ip_port):
    ips = ip_port.split(':')
    port = 0
    if len(ips) >= 1:
        ip = ips[0]
        if len(ips) == 2:
            port = int(ips[1])
        else:
            raise error.general('invalid host:port: %s' % (ip_port))
    return ip, port

class tftp_session(object):

    opcodes = ['nul', 'RRQ', 'WRQ', 'DATA', 'ACK', 'ERROR', 'OACK']

    def __init__(self):
        self.packets = []
        self.block = 0
        self.block_size = 512
        self.timeout = 0
        self.finished = True

    def __str__(self):
        return os.linesep.join([self.decode(p[0], p[1], p[2]) for p in self.packets])

    def data(self, host, port, data):
        finished = False
        self.packets += [(host, port, data)]
        opcode = (data[0] << 8) | data[1]
        if opcode == 1 or opcode == 2:
            self.block = 0
            self.finished = False
            value = self.get_option('timeout', data)
            if value is not None:
                self.timeout = int(value)
            value = self.get_option('blksize', data)
            if value is not None:
                self.block_size = int(value)
            else:
                self.block_size = 512
        elif opcode == 3:
            self.block = (data[2] << 8) | data[3]
            if len(data) - 4 < self.block_size:
                self.finished = True
        elif opcode == 4:
            self.block = (data[2] << 8) | data[3]
            if self.finished:
                finished = True
        return finished

    def decode(self, host, port, data):
        s = ''
        dlen = len(data)
        if dlen > 2:
            opcode = (data[0] << 8) | data[1]
            if opcode < len(self.opcodes):
                if opcode == 1 or opcode == 2:
                    s += '  ' + self.opcodes[opcode] + ', '
                    i = 2
                    while data[i] != 0:
                        s += chr(data[i])
                        i += 1
                    while i < dlen - 1:
                        s += ', '
                        i += 1
                        while data[i] != 0:
                            s += chr(data[i])
                            i += 1
                elif opcode == 3:
                    block = (data[2] << 8) | data[3]
                    s += '  ' + self.opcodes[opcode] + ', '
                    s += '#' + str(block) + ', '
                    if dlen > 8:
                        s += '%02x%02x..%02x%02x' % (data[4], data[5], data[-2], data[-1])
                    else:
                        for i in range(4, dlen):
                            s += '%02x' % (data[i])
                    s += ',' + str(dlen - 4)
                elif opcode == 4:
                    block = (data[2] << 8) | data[3]
                    s += '  ' + self.opcodes[opcode] + ' ' + str(block)
                elif opcode == 5:
                    s += 'E ' + self.opcodes[opcode] + ', '
                    s += str((data[2] << 8) | (data[3]))
                    i = 2
                    while data[i] != 0:
                        s += chr(data[i])
                        i += 1
                elif opcode == 6:
                    s += '  ' + self.opcodes[opcode]
                    i = 1
                    while i < dlen - 1:
                        s += ', '
                        i += 1
                        while data[i] != 0:
                            s += chr(data[i])
                            i += 1
            else:
                s += 'E INV(%d)' % (opcode)
        else:
            s += 'E INVALID LENGTH'
        return s[:2] + '[%s:%d] ' % (host, port) + s[2:]

    def get_option(self, option, data):
        dlen = len(data)
        opcode = (data[0] << 8) | data[1]
        next_option = False
        if opcode == 1 or opcode == 2:
            i = 1
            while i < dlen - 1:
                o = ''
                i += 1
                while data[i] != 0:
                    o += chr(data[i])
                    i += 1
                if o == option:
                    next_option = True
                elif next_option:
                    return o
        return None

    def get_timeout(self, default_timeout, timeout_guard):
        if self.timeout != 0:
            return self.timeout * timeout_guard
        return default_timeout

    def get_block_size(self):
        return self.block_size

class udp_handler(socketserver.BaseRequestHandler):

    def handle(self):
        client_ip = self.client_address[0]
        client_port = self.client_address[1]
        client = '%s:%i' % (client_ip, client_port)
        session = tftp_session()
        finished = session.data(client_ip, client_port, self.request[0])
        if not finished:
            timeout = session.get_timeout(self.server.proxy.session_timeout, 4)
            host = self.server.proxy.get_host(client_ip)
            if host is not None:
                session_count = self.server.proxy.get_session_count()
                log.notice(' ] %6d: session: %s -> %s: start' % (session_count,
                                                                 client,
                                                                 host))
                host_ip, host_server_port = host_port_split(host)
                host_port = host_server_port
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                sock.settimeout(timeout)
                log.trace('  > ' + session.decode(client_ip,
                                                  client_port,
                                                  self.request[0]))
                sock.sendto(self.request[0], (host_ip, host_port))
                while not finished:
                    try:
                        data, address = sock.recvfrom(16 * 1024)
                    except socket.error as se:
                        log.notice(' ] session: %s -> %s: error: %s' % (client,
                                                                        host,
                                                                        se))
                        return
                    except socket.gaierror as se:
                        log.notice(' ] session: %s -> %s: error: %s' % (client,
                                                                        host,
                                                                        se))
                        return
                    except:
                        return
                    finished = session.data(address[0], address[1], data)
                    if address[0] == host_ip:
                        if host_port == host_server_port:
                            host_port = address[1]
                        if  address[1] == host_port:
                            log.trace('  < ' + session.decode(address[0],
                                                              address[1],
                                                              data))
                            sock.sendto(data, (client_ip, client_port))
                    elif address[0] == client_ip and address[1] == client_port:
                        log.trace('  > ' + session.decode(address[0],
                                                          address[1],
                                                          data))
                        sock.sendto(data, (host_ip, host_port))
                log.notice(' ] %6d: session: %s -> %s: end' % (session_count,
                                                               client,
                                                               host))
            else:
                mac = misc.tools.getmac.get_mac_address(ip = client_ip)
                log.trace(' . request: host not found: %s (%s)' % (client, mac))

class udp_server(socketserver.ThreadingMixIn, socketserver.UDPServer):
    pass

class proxy_server(object):
    def __init__(self, config, host, port):
        self.lock = threading.Lock()
        self.session_timeout = 10
        self.host = host
        self.port = port
        self.server = None
        self.clients = { }
        self.config = configuration.configuration()
        self._load(config)
        self.session_counter = 0

    def __del__(self):
        self.stop()

    def _lock(self):
        self.lock.acquire()

    def _unlock(self):
        self.lock.release()

    def _load_client(self, client, depth = 0):
        if depth > 32:
            raise error.general('\'clients\'" nesting too deep; circular?')
        if not self.config.has_section(client):
            raise error.general('client not found: %s' % (client))
        for c in self.config.comma_list(client, 'clients', err = False):
            self._load_client(c, depth + 1)
        if client in self.clients:
            raise error.general('repeated client: %s' % (client))
        host = self.config.get_item(client, 'host', err = False)
        if host is not None:
            ips = self.config.comma_list(client, 'ip', err = False)
            macs = self.config.comma_list(client, 'mac', err = False)
            if len(ips) != 0 and len(macs) != 0:
                raise error.general('client has ip and mac: %s' % (client))
            if len(ips) != 0:
                keys = ips
            elif len(macs) != 0:
                keys = macs
            else:
                raise error.general('not client ip or mac: %s' % (client))
            for key in keys:
                self.clients[key] = host

    def _load(self, config):
        self.config.load(config)
        clients = self.config.comma_list('default', 'clients', err = False)
        if len(clients) == 0:
            raise error.general('\'clients\'" entry not found in config [defaults]')
        for client in clients:
            self._load_client(client)

    def start(self):
        log.notice('Proxy: %s:%i' % (self.host, self.port))
        if self.host == 'all':
            host = ''
        else:
            host = self.host
        try:
            self.server = udp_server((host, self.port), udp_handler)
        except Exception as e:
            raise error.general('proxy create: %s' % (e))
        self.server.proxy = self
        self._lock()
        try:
            self.server_thread = threading.Thread(target = self.server.serve_forever)
            self.server_thread.daemon = True
            self.server_thread.start()
        finally:
            self._unlock()

    def stop(self):
        self._lock()
        try:
            if self.server is not None:
                self.server.shutdown()
                self.server.server_close()
                self.server = None
        finally:
            self._unlock()

    def run(self):
        while True:
            time.sleep(1)

    def get_host(self, client):
        host = None
        self._lock()
        try:
            if client in self.clients:
                host = self.clients[client]
            else:
                mac = misc.tools.getmac.get_mac_address(ip = client)
                if mac in self.clients:
                    host = self.clients[mac]
        finally:
            self._unlock()
        return host

    def get_session_count(self):
        count = 0
        self._lock()
        try:
            self.session_counter += 1
            count = self.session_counter
        finally:
            self._unlock()
        return count


def load_log(logfile):
    if logfile is None:
        log.default = log.log(streams = ['stdout'])
    else:
        log.default = log.log(streams = [logfile])

def run(args = sys.argv, command_path = None):
    ec = 0
    notice = None
    proxy = None
    try:
        description  = 'Proxy TFTP sessions from the host running this proxy'
        description += 'to hosts and ports defined in the configuration file. '
        description += 'The tool lets you create a farm of hardware and to run '
        description += 'more than one TFTP test session on a host or multiple '
        description += 'hosts at once. This proxy service is not considered secure'
        description += 'and is for use in a secure environment.'

        argsp = argparse.ArgumentParser(prog = 'rtems-tftp-proxy',
                                        description = description)
        argsp.add_argument('-l', '--log',
                           help = 'log file.',
                           type = str, default = None)
        argsp.add_argument('-v', '--trace',
                           help = 'enable trace logging for debugging.',
                           action = 'store_true', default = False)
        argsp.add_argument('-c', '--config',
                           help = 'proxy configuation (default: %(default)s).',
                           type = str, default = None)
        argsp.add_argument('-B', '--bind',
                           help = 'address to bind the proxy too (default: %(default)s).',
                           type = str, default = 'all')
        argsp.add_argument('-P', '--port',
                           help = 'port to bind the proxy too(default: %(default)s).',
                           type = int, default = '69')

        argopts = argsp.parse_args(args[1:])

        load_log(argopts.log)
        log.notice('RTEMS Tools - TFTP Proxy, %s' % (version.string()))
        log.output(log.info(args))
        log.tracing = argopts.trace

        if argopts.config is None:
            raise error.general('no config file, see -h')

        proxy = proxy_server(argopts.config, argopts.bind, argopts.port)

        try:
            proxy.start()
            proxy.run()
        except:
            proxy.stop()
            raise

    except error.general as gerr:
        notice = str(gerr)
        ec = 1
    except error.internal as ierr:
        notice = str(ierr)
        ec = 1
    except error.exit as eerr:
        pass
    except KeyboardInterrupt:
        notice = 'abort: user terminated'
        ec = 1
    except:
        raise
        notice = 'abort: unknown error'
        ec = 1
    if proxy is not None:
        del proxy
    if notice is not None:
        log.stderr(notice)
    sys.exit(ec)

if __name__ == "__main__":
    run()
