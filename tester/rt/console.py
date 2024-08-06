#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013-2020 Chris Johns (chrisj@rtems.org)
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

from __future__ import print_function

import errno
import os
import threading
import time

from rtemstoolkit import path

import tester.rt.telnet
import tester.rt.juart

#
# Not available on Windows. Not sure what this means.
#
if os.name != 'nt':
    import tester.rt.stty as stty
else:
    stty = None

def save():
    if stty is not None:
        return stty.save()
    return None

def restore(attributes):
    if attributes is not None and stty is not None:
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
    '''TTY console connects to the target's console.'''

    def __init__(self, dev, output, setup = None, trace = False):
        self.tty = None
        self.read_thread = None
        self.dev = dev
        self.output = output
        self.setup = setup
        super(tty, self).__init__(dev, trace)

    def __del__(self):
        super(tty, self).__del__()
        self.close(0, 0)

    def open(self, index, total):
        def _readthread(me, x):
            line = ''
            while me.running:
                time.sleep(0.05)
                try:
                    data = me.tty.read()
                    if isinstance(data, bytes):
                        data = data.decode('utf-8', 'ignore')
                except IOError as ioe:
                    if ioe.errno == errno.EAGAIN:
                        continue
                    raise
                except:
                    raise
                for c in data:
                    if ord(c) > 0 and ord(c) < 128:
                        line += c
                        if c == '\n':
                            me.output(line)
                            line = ''
        if "juart-terminal.exe" in self.dev:
            self.tty = tester.rt.juart.tty(self.dev, index, total)
        elif stty and path.exists(self.dev):
            self.tty = stty.tty(self.dev, index, total)
        else:
            self.tty = tester.rt.telnet.tty(self.dev, index, total)
        self.tty.set(self.setup)
        self.tty.on()
        self.read_thread = threading.Thread(target = _readthread,
                                            name = 'tty[%s]' % (self.dev),
                                            args = (self, 0))
        self.read_thread.daemon = True
        self.running = True
        self.read_thread.start()

    def close(self, index, total):
        if self.tty:
            time.sleep(1)
            if self.read_thread:
                self.running = False
                self.read_thread.join(1)
                self.tty.close(index, total)
            self.tty = None
