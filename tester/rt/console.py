#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013-2014 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# RTEMS Testing Consoles
#

import errno
import fcntl
import os
import threading
import time

import stty

def save():
    return stty.save()

def restore(attributes):
    stty.restore(attributes)

class console(object):
    '''RTEMS Testing console base.'''

    def __init__(self, name, trace):
        self.name = name
        self.trace = trace

    def __del__(self):
        pass

    def _tracing(self):
        return self.trace

    def open(self):
        pass

    def close(self):
        pass

class stdio(console):
    '''STDIO console.'''

    def __init__(self, trace = False):
        super(stdio, self).__init__('stdio', trace)

class tty(console):
    '''TTY console connects to serial ports.'''

    raw = 'B115200,~BRKINT,IGNBRK,IGNCR,~ICANON,~ISIG,~IEXTEN,~ECHO,CLOCAL,~CRTSCTS'

    def __init__(self, dev, output, setup = None, trace = False):
        self.tty = None
        self.read_thread = None
        self.dev = dev
        self.output = output
        if setup is None:
            self.setup = raw
        else:
            self.setup = setup
        super(tty, self).__init__(dev, trace)

    def __del__(self):
        super(tty, self).__del__()
        if self._tracing():
            print ':: tty close', self.dev
        fcntl.fcntl(me.tty.fd, fcntl.F_SETFL,
                    fcntl.fcntl(me.tty.fd, fcntl.F_GETFL) & ~os.O_NONBLOCK)
        self.close()

    def open(self):
        def _readthread(me, x):
            if self._tracing():
                print ':: tty runner started', self.dev
            fcntl.fcntl(me.tty.fd, fcntl.F_SETFL,
                        fcntl.fcntl(me.tty.fd, fcntl.F_GETFL) | os.O_NONBLOCK)
            line = ''
            while me.running:
                time.sleep(0.05)
                try:
                    data = me.tty.fd.read()
                except IOError, ioe:
                    if ioe.errno == errno.EAGAIN:
                        continue
                    raise
                except:
                    raise
                for c in data:
                    if len(c) == 0:
                        continue
                    if c != chr(0):
                        line += c
                    if c == '\n':
                        me.output(line)
                        line = ''
            if self._tracing():
                print ':: tty runner finished', self.dev
        if self._tracing():
            print ':: tty open', self.dev
        self.tty = stty.tty(self.dev)
        self.tty.set(self.setup)
        self.tty.on()
        self.read_thread = threading.Thread(target = _readthread,
                                            name = 'tty[%s]' % (self.dev),
                                            args = (self, 0))
        self.read_thread.daemon = True
        self.running = True
        self.read_thread.start()

    def close(self):
        if self.tty:
            time.sleep(1)
            if self.read_thread:
                self.running = False
                self.read_thread.join(1)
            self.tty = None
