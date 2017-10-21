#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013-2017 Chris Johns (chrisj@rtems.org)
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
import re
import threading

from rtemstoolkit import config
from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import log
from rtemstoolkit import path

from . import console
from . import gdb
from . import tftp

timeout = 15

class file(config.file):
    """RTEMS Testing configuration."""

    _directives = ['%execute',
                   '%gdb',
                   '%tftp',
                   '%console']

    def __init__(self, index, total, report, name, opts, _directives = _directives):
        super(file, self).__init__(name, opts, directives = _directives)
        self.lock = threading.Lock()
        self.realtime_trace = self.debug_trace('output')
        self.process = None
        self.console = None
        self.output = None
        self.index = index
        self.total = total
        self.report = report
        self.name = name
        self.timedout = False
        self.test_started = False
        self.kill_good = False
        self.kill_on_end = False
        self.test_label = None
        self.target_reset_regx = None
        self.target_start_regx = None

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

    def _ok_kill(self):
        self._lock()
        self.kill_good = True
        self._unlock()
        try:
            self.process.kill()
        except:
            pass

    def _target_regex(self, label):
        regex = None
        if self.defined(label):
            r = self.expand('%%{%s}' % (label))
            try:
                regex = re.compile(r, re.MULTILINE)
            except:
                msg = 'invalid target regex: %s: %s' % (label, r)
                raise error.general(msg)
        return regex

    def _target_command(self, command):
        if self.defined('target_%s_command' % (command)):
            cmd = self.expand('%%{target_%s_command}' % (command)).strip()
            if len(cmd) > 0:
                rs_proc = execute.capture_execution()
                ec, proc, output = rs_proc.open(cmd, shell = True)
                self._capture_console('target %s: %s: %s' % (command, cmd, output))

    def _output_length(self):
        self._lock()
        if self.test_started:
            l = len(self.output)
            self._unlock()
            return l
        self._unlock()
        return 0

    def _capture_console(self, text):
        text = [('=>', l) for l in text.replace(chr(13), '').splitlines()]
        if self.output is not None:
            self._realtime_trace(text)
            self.output += text

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
            if not self.kill_good and ec > 0:
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

    def _dir_tftp(self, data, total, index, exe, bsp_arch, bsp):
        if len(data) != 2:
            raise error.general('invalid %tftp arguments')
        try:
            port = int(data[1])
        except:
            raise error.general('invalid %tftp port')
        self.kill_on_end = True
        self.process = tftp.tftp(bsp_arch, bsp,
                                 trace = self.debug_trace('gdb'))
        if not self.in_error:
            if self.console:
                self.console.open()
            self.process.open(executable = data[0],
                              port = port,
                              output_length = self._output_length,
                              console = self.capture_console,
                              timeout = (int(self.expand('%{timeout}')),
                                         self._timeout))
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
                    self.output = []
                    total = int(self.expand('%{test_total}'))
                    index = int(self.expand('%{test_index}'))
                    exe = self.expand('%{test_executable}')
                    bsp_arch = self.expand('%{bsp_arch}')
                    bsp = self.expand('%{bsp}')
                    self.report.start(index, total, exe, exe, bsp_arch, bsp)
                    if self.index == 1:
                        self._target_command('on')
                finally:
                    self._unlock()
                if _directive == '%execute':
                    self._dir_execute(ds, total, index, exe, bsp_arch, bsp)
                elif _directive == '%gdb':
                    self._dir_gdb(ds, total, index, exe, bsp_arch, bsp)
                elif _directive == '%tftp':
                    self._dir_tftp(ds, total, index, exe, bsp_arch, bsp)
                else:
                    raise error.general(self._name_line_msg('invalid directive'))
                self._lock()
                if self.index == self.total:
                    self._target_command('off')
                try:
                    status = self.report.end(exe, self.output)
                    self._capture_console('test result: %s' % (status))
                    if status == 'timeout':
                        if self.index == self.total:
                            self._target_command('off')
                        else:
                            self._target_command('reset')
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
        self.target_start_regx = self._target_regex('target_start_regex')
        self.target_reset_regx = self._target_regex('target_reset_regex')
        self.load(self.name)

    def capture(self, text):
        if not self.test_started:
            s = text.find('*** BEGIN OF TEST ')
            if s >= 0:
                self.test_started = True
                e = text[s + 3:].find('***')
                if e >= 0:
                    self.test_label = text[s + len('*** BEGIN OF TEST '):s + e + 3 - 1]
                self.capture_console('test start: %s' % (self.test_label))
        ok_to_kill = '*** TEST STATE: USER_INPUT' in text or \
                     '*** TEST STATE: BENCHMARK' in text
        if ok_to_kill:
            reset_target = True
        else:
            reset_target = False
        if self.test_started and self.target_start_regx is not None:
            if self.target_start_regx.match(text):
                self.capture_console('target start detected')
                ok_to_kill = True
        if not reset_target and self.target_reset_regx is not None:
            if self.target_reset_regx.match(text):
                self.capture_console('target reset condition detected')
                reset_target = True
        if self.kill_on_end:
            if not ok_to_kill and '*** END OF TEST ' in text:
                self.capture_console('test end: %s' % (self.test_label))
                if self.test_label is not None:
                    ok_to_kill = '*** END OF TEST %s ***' % (self.test_label) in text
        text = [(']', l) for l in text.replace(chr(13), '').splitlines()]
        self._lock()
        if self.output is not None:
            self._realtime_trace(text)
            self.output += text
        if reset_target:
            if self.index == self.total:
                self._target_command('off')
            else:
                self._target_command('reset')
        self._unlock()
        if ok_to_kill:
            self._ok_kill()

    def capture_console(self, text):
        self._lock()
        self._capture_console(text)
        self._unlock()

    def debug_trace(self, flag):
        dt = self.macros['debug_trace']
        if dt:
            if flag in dt.split(','):
                return True
        return False

    def kill(self):
        if self.process:
            try:
                self.process.kill()
            except:
                pass
