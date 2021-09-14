# SPDX-License-Identifier: BSD-2-Clause
'''The TFTP Server handles a read only TFTP session.'''

# Copyright (C) 2020 Chris Johns (chrisj@rtems.org)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function

import argparse
import os
import socket
import sys
import time
import threading

try:
    import socketserver
except ImportError:
    import SocketServer as socketserver

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import version


class tftp_session(object):
    '''Handle the TFTP session packets initiated on the TFTP port (69).
    '''
    # pylint: disable=useless-object-inheritance
    # pylint: disable=too-many-instance-attributes

    opcodes = ['nul', 'RRQ', 'WRQ', 'DATA', 'ACK', 'ERROR', 'OACK']

    OP_RRQ = 1
    OP_WRQ = 2
    OP_DATA = 3
    OP_ACK = 4
    OP_ERROR = 5
    OP_OACK = 6

    E_NOT_DEFINED = 0
    E_FILE_NOT_FOUND = 1
    E_ACCESS_VIOLATION = 2
    E_DISK_FULL = 3
    E_ILLEGAL_TFTP_OP = 4
    E_UKNOWN_TID = 5
    E_FILE_ALREADY_EXISTS = 6
    E_NO_SUCH_USER = 7
    E_NO_ERROR = 10

    def __init__(self, host, port, base, forced_file, reader=None):
        # pylint: disable=too-many-arguments
        self.host = host
        self.port = port
        self.base = base
        self.forced_file = forced_file
        if reader is None:
            self.data_reader = self._file_reader
        else:
            self.data_reader = reader
        self.filein = None
        self.resends_limit = 5
        # These are here to shut pylint up
        self.block = 0
        self.last_data = None
        self.block_size = 512
        self.timeout = 0
        self.resends = 0
        self.finished = False
        self.filename = None
        self._reinit()

    def _reinit(self):
        '''Reinitialise all the class variables used by the protocol.'''
        if self.filein is not None:
            self.filein.close()
            self.filein = None
        self.block = 0
        self.last_data = None
        self.block_size = 512
        self.timeout = 0
        self.resends = 0
        self.finished = False
        self.filename = None

    def _file_reader(self, command, **kwargs):
        '''The default file reader if the user does not provide one.

        The call returns a two element tuple where the first element
        is an error code, and the second element is data if the error
        code is 0 else it is an error message.
        '''
        # pylint: disable=too-many-return-statements
        if command == 'open':
            if 'filename' not in kwargs:
                raise error.general('tftp-reader: invalid open: no filename')
            filename = kwargs['filename']
            try:
                self.filein = open(filename, 'rb')
                filesize = os.stat(filename).st_size
            except FileNotFoundError:
                return self.E_FILE_NOT_FOUND, 'file not found (%s)' % (
                    filename)
            except PermissionError:
                return self.E_ACCESS_VIOLATION, 'access violation'
            except IOError as ioe:
                return self.E_NOT_DEFINED, str(ioe)
            return self.E_NO_ERROR, str(filesize)
        if command == 'read':
            if self.filein is None:
                raise error.general('tftp-reader: read when not open')
            if 'blksize' not in kwargs:
                raise error.general('tftp-reader: invalid read: no blksize')
            # pylint: disable=bare-except
            try:
                return self.E_NO_ERROR, self.filein.read(kwargs['blksize'])
            except IOError as ioe:
                return self.E_NOT_DEFINED, str(ioe)
            except:
                return self.E_NOT_DEFINED, 'unknown error'
        if command == 'close':
            if self.filein is not None:
                self.filein.close()
                self.filein = None
            return self.E_NO_ERROR, "closed"
        return self.E_NOT_DEFINED, 'invalid reader state'

    @staticmethod
    def _pack_bytes(data=None):
        bdata = bytearray()
        if data is not None:
            if not isinstance(data, list):
                data = [data]
            for item in data:
                if isinstance(item, int):
                    bdata.append(item >> 8)
                    bdata.append(item & 0xff)
                elif isinstance(item, str):
                    bdata.extend(item.encode())
                    bdata.append(0)
                else:
                    bdata.extend(item)
        return bdata

    def _response(self, opcode, data):
        code = self.opcodes.index(opcode)
        if code == 0 or code >= len(self.opcodes):
            raise error.general('invalid opcode: ' + opcode)
        bdata = self._pack_bytes([code, data])
        #print(''.join(format(x, '02x') for x in bdata))
        return bdata

    def _error_response(self, code, message):
        if log.tracing:
            log.trace('tftp: error: %s:%d: %d: %s' %
                      (self.host, self.port, code, message))
        self.finished = True
        return self._response('ERROR', self._pack_bytes([code, message, 0]))

    def _data_response(self, block, data):
        if len(data) < self.block_size:
            self.finished = True
        return self._response('DATA', self._pack_bytes([block, data]))

    def _oack_response(self, data):
        self.resends += 1
        if self.resends >= self.resends_limit:
            return self._error_response(self.E_NOT_DEFINED,
                                        'resend limit reached')
        return self._response('OACK', self._pack_bytes(data))

    def _next_block(self, block):
        # has the current block been acknowledged?
        if block == self.block:
            self.resends = 0
            self.block += 1
            err, data = self.data_reader('read', blksize=self.block_size)
            data = bytearray(data)
            if err != self.E_NO_ERROR:
                return self._error_response(err, data)
            # close if the length of data is less than the block size
            if len(data) < self.block_size:
                self.data_reader('close')
            self.last_data = data
        else:
            self.resends += 1
            if self.resends >= self.resends_limit:
                return self._error_response(self.E_NOT_DEFINED,
                                            'resend limit reached')
            data = self.last_data
        return self._data_response(self.block, data)

    def _read_req(self, data):
        # if the last block is not 0 something has gone wrong and
        # TID match. Restart the session. It could be the client
        # is a simple implementation that does not move the send
        # port on each retry.
        if self.block != 0:
            self.data_reader('close')
            self._reinit()
        # Get the filename, mode and options
        self.filename = self.get_option('filename', data)
        if self.filename is None:
            return self._error_response(self.E_NOT_DEFINED,
                                        'filename not found in request')
        if self.forced_file is not None:
            self.filename = self.forced_file
        # open the reader
        err, message = self.data_reader('open', filename=self.filename)
        if err != self.E_NO_ERROR:
            return self._error_response(err, message)
        # the no error on open message is the file size
        try:
            tsize = int(message)
        except ValueError:
            tsize = 0
        mode = self.get_option('mode', data)
        if mode is None:
            return self._error_response(self.E_NOT_DEFINED,
                                        'mode not found in request')
        oack_data = self._pack_bytes()
        value = self.get_option('timeout', data)
        if value is not None:
            oack_data += self._pack_bytes(['timeout', value])
            self.timeout = int(value)
        value = self.get_option('blksize', data)
        if value is not None:
            oack_data += self._pack_bytes(['blksize', value])
            self.block_size = int(value)
        else:
            self.block_size = 512
        value = self.get_option('tsize', data)
        if value is not None and tsize > 0:
            oack_data += self._pack_bytes(['tsize', str(tsize)])
        # Send the options ack
        return self._oack_response(oack_data)

    def _write_req(self):
        # WRQ is not supported
        return self._error_response(self.E_ILLEGAL_TFTP_OP,
                                    "writes not supported")

    def _op_ack(self, data):
        # send the next block of data
        block = (data[2] << 8) | data[3]
        return self._next_block(block)

    def process(self, host, port, data):
        '''Process the incoming client data sending a response. If the session
        has finished return None.
        '''
        if host != self.host and port != self.port:
            return self._error_response(self.E_UKNOWN_TID,
                                        'unkown transfer ID')
        if self.finished:
            return None
        opcode = (data[0] << 8) | data[1]
        if opcode == self.OP_RRQ:
            return self._read_req(data)
        if opcode in [self.OP_WRQ, self.OP_DATA]:
            return self._write_req()
        if opcode == self.OP_ACK:
            return self._op_ack(data)
        return self._error_response(self.E_ILLEGAL_TFTP_OP,
                                    "unknown or unsupported opcode")

    def decode(self, host, port, data):
        '''Decode the packet for diagnostic purposes.
        '''
        # pylint: disable=too-many-branches
        out = ''
        dlen = len(data)
        if dlen > 2:
            opcode = (data[0] << 8) | data[1]
            if 0 < opcode < len(self.opcodes):
                if opcode in [self.OP_RRQ, self.OP_WRQ]:
                    out += '  ' + self.opcodes[opcode] + ', '
                    i = 2
                    while data[i] != 0:
                        out += chr(data[i])
                        i += 1
                    while i < dlen - 1:
                        out += ', '
                        i += 1
                        while data[i] != 0:
                            out += chr(data[i])
                            i += 1
                elif opcode == self.OP_DATA:
                    block = (data[2] << 8) | data[3]
                    out += '  ' + self.opcodes[opcode] + ', '
                    out += '#' + str(block) + ', '
                    if dlen > 8:
                        out += '%02x%02x..%02x%02x' % (data[4], data[5],
                                                       data[-2], data[-1])
                    else:
                        for i in range(4, dlen):
                            out += '%02x' % (data[i])
                    out += ',' + str(dlen - 4)
                elif opcode == self.OP_ACK:
                    block = (data[2] << 8) | data[3]
                    out += '  ' + self.opcodes[opcode] + ' ' + str(block)
                elif opcode == self.OP_ERROR:
                    out += 'E ' + self.opcodes[opcode] + ', '
                    out += str((data[2] << 8) | (data[3]))
                    out += ': ' + str(data[4:].decode())
                    i = 2
                    while data[i] != 0:
                        out += chr(data[i])
                        i += 1
                elif opcode == self.OP_OACK:
                    out += '  ' + self.opcodes[opcode]
                    i = 1
                    while i < dlen - 1:
                        out += ', '
                        i += 1
                        while data[i] != 0:
                            out += chr(data[i])
                            i += 1
            else:
                out += 'E INV(%d)' % (opcode)
        else:
            out += 'E INVALID LENGTH'
        return out[:2] + '[%s:%d] (%d) ' % (host, port, len(data)) + out[2:]

    @staticmethod
    def get_option(option, data):
        '''Get the option from the TFTP packet.'''
        dlen = len(data) - 1
        opcode = (data[0] << 8) | data[1]
        next_option = False
        if opcode in [1, 2]:
            count = 0
            i = 2
            while i < dlen:
                value = ''
                while data[i] != 0:
                    value += chr(data[i])
                    i += 1
                i += 1
                if option == 'filename' and count == 0:
                    return value
                if option == 'mode' and count == 1:
                    return value
                if value == option and (count % 1) == 0:
                    next_option = True
                elif next_option:
                    return value
                count += 1
        return None

    def get_timeout(self, default_timeout, timeout_guard):
        '''Get the timeout. The timeout can be an option.'''
        if self.timeout != 0:
            return self.timeout + timeout_guard
        return default_timeout

    def get_block_size(self):
        '''Get the block size. The block size can be an option.'''
        return self.block_size


class udp_handler(socketserver.BaseRequestHandler):
    '''TFTP UDP handler for a TFTP session.'''
    def _notice(self, text):
        if self.server.tftp.notices:
            log.notice(text)
        if log.tracing:
            log.trace(text)

    def handle_session(self, index):
        '''Handle the TFTP session data.'''
        # pylint: disable=too-many-locals
        # pylint: disable=broad-except
        # pylint: disable=too-many-branches
        # pylint: disable=too-many-statements
        client_ip = self.client_address[0]
        client_port = self.client_address[1]
        client = '%s:%i' % (client_ip, client_port)
        self._notice('] tftp: %d: start: %s' % (index, client))
        try:
            session = tftp_session(client_ip, client_port,
                                   self.server.tftp.base,
                                   self.server.tftp.forced_file,
                                   self.server.tftp.reader)
            data = bytearray(self.request[0])
            response = session.process(client_ip, client_port, data)
            if response is not None:
                if log.tracing and self.server.tftp.packet_trace:
                    log.trace(' > ' +
                              session.decode(client_ip, client_port, data))
                timeout = session.get_timeout(self.server.tftp.timeout, 5)
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                sock.bind(('', 0))
                sock.settimeout(timeout)
                while response is not None:
                    if log.tracing and self.server.tftp.packet_trace:
                        log.trace(
                            ' < ' +
                            session.decode(client_ip, client_port, response))
                    sock.sendto(response, (client_ip, client_port))
                    try:
                        data, address = sock.recvfrom(2 + 2 +
                                                      session.get_block_size())
                        data = bytearray(data)
                        if log.tracing and self.server.tftp.packet_trace:
                            log.trace(
                                ' > ' +
                                session.decode(address[0], address[1], data))
                    except socket.timeout as sto:
                        self._notice('] tftp: %d: timeout: %s' % (index, client))
                        continue
                    except socket.error as serr:
                        log._notice('] tftp: %d: sock receive: %s: error: %s' \
                                    % (index, client, serr))
                        return
                    except socket.gaierror as serr:
                        log._notice('] tftp: %d: sock receive: %s: error: %s' \
                                    % (index, client, serr))
                        return
                    if session.finished:
                        break
                    response = session.process(address[0], address[1], data)
        except error.general as gerr:
            self._notice('] tftp: %d: error: %s' % (index, gerr))
        except error.internal as ierr:
            self._notice('] tftp: %d: error: %s' % (index, ierr))
        except error.exit:
            pass
        except KeyboardInterrupt:
            pass
        except Exception as exp:
            if self.server.tftp.exception_is_raise:
                raise
            self._notice('] tftp: %d: error: %s: %s' % (index, type(exp), exp))
        self._notice('] tftp: %d: end: %s' % (index, client))

    def handle(self):
        '''The UDP server handle method.'''
        if self.server.tftp.sessions is None \
           or self.server.tftp.session < self.server.tftp.sessions:
            self.handle_session(self.server.tftp.next_session())


class udp_server(socketserver.ThreadingMixIn, socketserver.UDPServer):
    '''UDP server. Default behaviour.'''


class tftp_server(object):
    '''TFTP server runs a UDP server to handle TFTP sessions.'''

    # pylint: disable=useless-object-inheritance
    # pylint: disable=too-many-instance-attributes

    def __init__(self,
                 host,
                 port,
                 timeout=10,
                 base=None,
                 forced_file=None,
                 sessions=None,
                 reader=None):
        # pylint: disable=too-many-arguments
        self.lock = threading.Lock()
        self.notices = False
        self.packet_trace = False
        self.exception_is_raise = False
        self.timeout = timeout
        self.host = host
        self.port = port
        self.server = None
        self.server_thread = None
        if base is None:
            base = os.getcwd()
        self.base = base
        self.forced_file = forced_file
        if sessions is not None and not isinstance(sessions, int):
            raise error.general('tftp session count is not a number')
        self.sessions = sessions
        self.session = 0
        self.reader = reader

    def __del__(self):
        self.stop()

    def _lock(self):
        self.lock.acquire()

    def _unlock(self):
        self.lock.release()

    def start(self):
        '''Start the TFTP server. Returns once started.'''
        # pylint: disable=attribute-defined-outside-init
        if log.tracing:
            log.trace('] tftp: server: %s:%i' % (self.host, self.port))
        if self.host == 'all':
            host = ''
        else:
            host = self.host
        try:
            self.server = udp_server((host, self.port), udp_handler)
        except Exception as exp:
            raise error.general('tftp server create: %s' % (exp))
        # We cannot set tftp in __init__ because the object is created
        # in a separate package.
        self.server.tftp = self
        self.server_thread = threading.Thread(target=self.server.serve_forever)
        self.server_thread.daemon = True
        self.server_thread.start()

    def stop(self):
        '''Stop the TFTP server and close the server port.'''
        if self.server is not None:
            self.server.server_close()
            self.server.shutdown()
            self._lock()
            try:
                self.server = None
            finally:
                self._unlock()

    def run(self):
        '''Run the TFTP server for the specified number of sessions.'''
        running = True
        while running:
            period = 1
            self._lock()
            if self.server is None:
                running = False
                period = 0
            elif self.sessions is not None:
                if self.sessions == 0:
                    running = False
                    period = 0
                else:
                    period = 0.25
            self._unlock()
            if period > 0:
                time.sleep(period)
        self.stop()

    def get_session(self):
        '''Return the session count.'''
        count = 0
        self._lock()
        try:
            count = self.session
        finally:
            self._unlock()
        return count

    def next_session(self):
        '''Return the next session number.'''
        count = 0
        self._lock()
        try:
            self.session += 1
            count = self.session
        finally:
            self._unlock()
        return count

    def enable_notices(self):
        '''Call to enable notices. The server is quiet without this call.'''
        self._lock()
        self.notices = True
        self._unlock()

    def trace_packets(self):
        '''Call to enable packet tracing as a diagnostic.'''
        self._lock()
        self.packet_trace = True
        self._unlock()

    def except_is_raise(self):
        '''If True a standard exception will generate a backtrace.'''
        self.exception_is_raise = True


def load_log(logfile):
    '''Set the log file.'''
    if logfile is None:
        log.default = log.log(streams=['stdout'])
    else:
        log.default = log.log(streams=[logfile])


def run(args=sys.argv, command_path=None):
    '''Run a TFTP server session.'''
    # pylint: disable=dangerous-default-value
    # pylint: disable=unused-argument
    # pylint: disable=too-many-branches
    # pylint: disable=too-many-statements
    ecode = 0
    notice = None
    server = None
    # pylint: disable=bare-except
    try:
        description = 'A TFTP Server that supports a read only TFTP session.'

        nice_cwd = os.path.relpath(os.getcwd())
        if len(nice_cwd) > len(os.path.abspath(nice_cwd)):
            nice_cwd = os.path.abspath(nice_cwd)

        argsp = argparse.ArgumentParser(prog='rtems-tftp-server',
                                        description=description)
        argsp.add_argument('-l',
                           '--log',
                           help='log file.',
                           type=str,
                           default=None)
        argsp.add_argument('-v',
                           '--trace',
                           help='enable trace logging for debugging.',
                           action='store_true',
                           default=False)
        argsp.add_argument('--trace-packets',
                           help='enable trace logging of packets.',
                           action='store_true',
                           default=False)
        argsp.add_argument('--show-backtrace',
                           help='show the exception backtrace.',
                           action='store_true',
                           default=False)
        argsp.add_argument(
            '-B',
            '--bind',
            help='address to bind the server too (default: %(default)s).',
            type=str,
            default='all')
        argsp.add_argument(
            '-P',
            '--port',
            help='port to bind the server too (default: %(default)s).',
            type=int,
            default='69')
        argsp.add_argument('-t', '--timeout',
                           help = 'timeout in seconds, client can override ' \
                           '(default: %(default)s).',
                           type = int, default = '10')
        argsp.add_argument(
            '-b',
            '--base',
            help='base path, not checked (default: %(default)s).',
            type=str,
            default=nice_cwd)
        argsp.add_argument(
            '-F',
            '--force-file',
            help='force the file to be downloaded overriding the client.',
            type=str,
            default=None)
        argsp.add_argument('-s', '--sessions',
                           help = 'number of TFTP sessions to run before exiting ' \
                           '(default: forever.',
                           type = int, default = None)

        argopts = argsp.parse_args(args[1:])

        load_log(argopts.log)
        log.notice('RTEMS Tools - TFTP Server, %s' % (version.string()))
        log.output(log.info(args))
        log.tracing = argopts.trace

        server = tftp_server(argopts.bind, argopts.port, argopts.timeout,
                             argopts.base, argopts.force_file,
                             argopts.sessions)
        server.enable_notices()
        if argopts.trace_packets:
            server.trace_packets()
        if argopts.show_backtrace:
            server.except_is_raise()

        try:
            server.start()
            server.run()
        finally:
            server.stop()

    except error.general as gerr:
        notice = str(gerr)
        ecode = 1
    except error.internal as ierr:
        notice = str(ierr)
        ecode = 1
    except error.exit:
        pass
    except KeyboardInterrupt:
        notice = 'abort: user terminated'
        ecode = 1
    except SystemExit:
        pass
    except:
        notice = 'abort: unknown error'
        ecode = 1
    if server is not None:
        del server
    if notice is not None:
        log.stderr(notice)
    sys.exit(ecode)


if __name__ == "__main__":
    run()
