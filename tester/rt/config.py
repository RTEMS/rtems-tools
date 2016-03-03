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
# RTEMS Testing Config
#

from __future__ import print_function

import datetime
import os
import threading

from rtemstoolkit import config
from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import log
from rtemstoolkit import path

from . import console
from . import gdb

timeout = 15

class file(config.file):
    """RTEMS Testing configuration."""

    _directives = ['%execute',
                   '%gdb',
                   '%console']

    def __init__(self, report, name, opts, _directives = _directives):
        super(file, self).__init__(name, opts, directives = _directives)
        self.lock = threading.Lock()
        self.realtime_trace = self.debug_trace('output')
        self.process = None
        self.console = None
        self.output = None
        self.report = report
        self.name = name
        self.timedout = False

    def __del__(self):
        if self.console:
            del self.console
        super(file, self).__del__()

    def _lock(self):
        self.lock.acquire()

    def _unlock(self):
        self.lock.release()

    def _timeout(self):
        self._lock()
        self.timedout = True
        self._unlock()
        self.capture('*** TIMEOUT TIMEOUT')

    def _dir_console(self, data):
        if self.console is not None:
            raise error.general(self._name_line_msg('console already configured'))
        if len(data) == 0:
            raise error.general(self._name_line_msg('no console configuration provided'))
        console_trace = trace = self.debug_trace('console')
        if data[0] == 'stdio':
            self.console = console.stdio(trace = console_trace)
        elif data[0] == 'tty':
            if len(data) < 2 or len(data) >3:
                raise error.general(self._name_line_msg('no tty configuration provided'))
            if len(data) == 3:
                settings = data[2]
            else:
                settings = None
            self.console = console.tty(data[1],
                                       output = self.capture,
                                       setup = settings,
                                       trace = console_trace)
        else:
            raise error.general(self._name_line_msg('invalid console type'))

    def _dir_execute(self, data, total, index, exe, bsp_arch, bsp):
        self.process = execute.execute(output = self.capture)
        if not self.in_error:
            if self.console:
                self.console.open()
            self.capture_console('run: %s' % (' '.join(data)))
            ec, proc = self.process.open(data,
                                         timeout = (int(self.expand('%{timeout}')),
                                                    self._timeout))
            self._lock()
            if ec > 0:
                self._error('execute failed: %s: exit-code:%d' % (' '.join(data), ec))
            elif self.timedout:
                self.process.kill()
            self._unlock()
            if self.console:
                self.console.close()

    def _dir_gdb(self, data, total, index, exe, bsp_arch, bsp):
        if len(data) < 3 or len(data) > 4:
            raise error.general('invalid %gdb arguments')
        self.process = gdb.gdb(bsp_arch, bsp,
                               trace = self.debug_trace('gdb'),
                               mi_trace = self.debug_trace('gdb-mi'))
        script = self.expand('%%{%s}' % data[2])
        if script:
            script = [l.strip() for l in script.splitlines()]
        if not self.in_error:
            if self.console:
                self.console.open()
            self.process.open(data[0], data[1],
                              script = script,
                              output = self.capture,
                              gdb_console = self.capture_console,
                              timeout = int(self.expand('%{timeout}')))
            if self.console:
                self.console.close()

    def _directive_filter(self, results, directive, info, data):
        if results[0] == 'directive':
            _directive = results[1]
            _data = results[2]
            ds = []
            if len(_data):
                ds = [_data[0]]
                if len(_data) > 1:
                    ds += _data[1].split()
            ds = self.expand(ds)

            if _directive == '%console':
                self._dir_console(ds)
            else:
                self._lock()
                try:
                    total = int(self.expand('%{test_total}'))
                    index = int(self.expand('%{test_index}'))
                    exe = self.expand('%{test_executable}')
                    bsp_arch = self.expand('%{bsp_arch}')
                    bsp = self.expand('%{bsp}')
                    self.report.start(index, total, exe, exe, bsp_arch, bsp)
                    self.output = []
                finally:
                    self._unlock()
                if _directive == '%execute':
                    self._dir_execute(ds, total, index, exe, bsp_arch, bsp)
                elif _directive == '%gdb':
                    self._dir_gdb(ds, total, index, exe, bsp_arch, bsp)
                else:
                    raise error.general(self._name_line_msg('invalid directive'))
                self._lock()
                try:
                    self.report.end(exe, self.output)
                    self.process = None
                    self.output = None
                finally:
                    self._unlock()
        return None, None, None

    def _realtime_trace(self, text):
        if self.realtime_trace:
            for l in text:
                print(' '.join(l))

    def run(self):
        self.load(self.name)

    def capture(self, text):
        text = [(']', l) for l in text.replace(chr(13), '').splitlines()]
        self._lock()
        if self.output is not None:
            self._realtime_trace(text)
            self.output += text
        self._unlock()

    def capture_console(self, text):
        text = [('>', l) for l in text.replace(chr(13), '').splitlines()]
        self._lock()
        if self.output is not None:
            self._realtime_trace(text)
            self.output += text
        self._unlock()

    def debug_trace(self, flag):
        dt = self.macros['debug_trace']
        if dt:
            if flag in dt.split(','):
                return True
        return False

    def kill(self):
        if self.process:
            self.process.kill()
