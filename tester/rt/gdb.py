#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013, 2020 Chris Johns (chrisj@rtems.org)
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
# RTEMS Testing GDB Interface
#

from __future__ import print_function

import datetime
import os
try:
    import Queue
    queue = Queue
except ImportError:
    import queue
import sys
import threading
import signal
import time

from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import options
from rtemstoolkit import path

import tester.rt.pygdb

class gdb(object):
    '''RTEMS Testing GDB base.'''

    def __init__(self, bsp_arch, bsp, trace = False, mi_trace = False):
        self.session = tester.rt.pygdb.mi_parser.session()
        self.trace = trace
        self.mi_trace = mi_trace
        self.lock_trace = False
        self.lock_locked = None
        self.lock = threading.RLock()
        self.script = None
        self.script_line = 0
        self.bsp = bsp
        self.bsp_arch = bsp_arch
        self.output = None
        self.output_length = 0
        self.gdb_console = None
        self.input = queue.Queue()
        self.commands = queue.Queue()
        self.process = None
        self.ecode = None
        self.state = {}
        self.running = False
        self.breakpoints = {}
        self.output = None
        self.output_buffer = ''
        self.timeout = None
        self.test_too_long = None
        self.lc = 0

    def _lock(self, msg):
        if self.lock_trace:
            print('|[   LOCK:%s ]|' % (msg))
            self.lock_locked = datetime.datetime.now()
        self.lock.acquire()

    def _unlock(self, msg):
        if self.lock_trace:
            period = datetime.datetime.now() - self.lock_locked
            print('|] UNLOCK:%s [| : %s' % (msg, period))
        self.lock.release()

    def _put(self, text):
        if self.trace:
            print(')))', text)
        self.commands.put(text)

    def _input_commands(self):
        if self.commands.empty():
            return False
        try:
            if self.trace:
                print('... input empty ', self.input.empty())
            if self.input.empty():
                line = self.commands.get(block = False)
                if self.trace:
                    print('+++', line)
                self.input.put(line)
        except:
            pass
        return True

    def _reader(self, line):
        self._lock('_reader')
        if self.trace:
            print('<<<', line)
        try:
            self.lc += 1
            if line.startswith('(gdb)'):
                if self.trace:
                    print('^^^ (gdb)')
                if not self._input_commands():
                    self.gdb_expect()
                    self._input_commands()
            else:
                self.gdb_parse(line)
        finally:
            self._unlock('_reader')

    def _writer(self):
        try:
            try:
                self._lock('_writer')
                try:
                    if self.process is None:
                        return None
                finally:
                    self._unlock('_writer')
                line = self.input.get(timeout = 0.5)
                if self.trace:
                    print('>>> input: queue=%d' % (self.input.qsize()), line)
            except queue.Empty:
                return True
            if line is None:
                return None
            return line + os.linesep
        except:
            if self.trace:
                print('writer exception')
            pass
        if self.trace:
            print('writer closing')
        return False

    def _timeout(self):
        self._stop()
        if self.timeout is not None:
            self.timeout()

    def _test_too_long(self):
        self._stop()
        if self.test_too_long is not None:
            self.test_too_long()

    def _cleanup(self, proc):
        self._lock('_cleanup')
        try:
            self._put(None)
        finally:
            self._unlock('_cleanup')

    def _gdb_quit(self, backtrace = False):
        self._lock('_gdb_quit')
        try:
            self.process.send_signal(signal.SIGINT)
            if backtrace:
                self._put('bt')
            self._put('disconnect')
            self._put('quit')
            if self.script:
                self.script_line = len(self.script)
        finally:
            self._unlock('_gdb_quit')

    def _stop(self):
        self._gdb_quit(backtrace=True)
        seconds = 5
        step = 0.1
        while self.process and seconds > 0:
            if seconds > step:
                seconds -= step
            else:
                seconds = 0
            self._unlock('_stop')
            time.sleep(step)
            self._lock('_stop')
        if self.process and seconds == 0:
            self._kill()

    def _kill(self):
        if self.process:
            self.process.kill()
        self.process = None

    def _execute_gdb(self, args):
        '''Thread to execute GDB and to wait for it to finish.'''
        cmds = args
        self.gdb_console('gdb: %s' % (' '.join(cmds)))
        ecode, proc = self.process.open(cmds)
        if self.trace:
            print('gdb done', ecode)
        self._lock('_execute_gdb')
        self.ecode = ecode
        self.process = None
        self.running = False
        self._unlock('_execute_gdb')

    def _monitor(self, timeout):
        output_length = self.output_length
        step = 0.25
        period = timeout[0]
        seconds = timeout[1]
        while self.process and period > 0 and seconds > 0:
            current_length = self.output_length
            if output_length != current_length:
                period = timeout[0]
            output_length = current_length
            if seconds < step:
                seconds = 0
            else:
                seconds -= step
            if period < step:
                step = period
                period = 0
            else:
                period -= step
            self._unlock('_monitor')
            time.sleep(step)
            self._lock('_monitor')
        if self.process is not None:
            if period == 0:
                self._timeout()
            elif seconds == 0:
                self._test_too_long()

    def open(self, command, executable,
             output, gdb_console, timeout,
             script = None, tty = None):
        self._lock('_open')
        self.timeout = timeout[2]
        self.test_too_long = timeout[3]
        try:
            cmds = execute.arg_list(command) + ['-i=mi',
                                                '--nx',
                                                '--quiet']
            if tty:
                cmds += ['--tty=%s' % tty]
            if executable:
                cmds += [executable]
            self.output = output
            self.gdb_console = gdb_console
            self.script = script
            self.process = execute.execute(output = self._reader,
                                           input = self._writer,
                                           cleanup = self._cleanup)
            exec_thread = threading.Thread(target=self._execute_gdb,
                                           args=[cmds])
            exec_thread.start()
            self._monitor(timeout)
            if self.ecode is not None and self.ecode > 0:
                raise error.general('gdb exec: %s: %s' % (cmds[0],
                                                          os.strerror(self.ecode)))
        finally:
            self._unlock('_open')

    def kill(self):
        self._lock('_kill')
        try:
            self._stop()
        finally:
            self._unlock('_kill')

    def target_restart(self, started):
        pass

    def target_reset(self, started):
        pass

    def target_start(self):
        pass

    def target_end(self):
        pass

    def gdb_expect(self):
        if self.trace:
            print('}}} gdb-expect')
        if self.process and not self.running and self.script is not None:
            if self.script_line == len(self.script):
                self._put(None)
            else:
                if self.script_line == 0:
                    self._put('-gdb-set confirm no')
                line = self.script[self.script_line]
                self.script_line += 1
                self._put(line)

    def gdb_parse(self, lines):
        try:
            if self.mi_trace:
                print('mi-data:', lines)
            rec = self.session.process(lines)
            if self.mi_trace:
                print('mi-rec:', rec)
            if rec.record_type == 'result':
                if rec.type == 'result':
                    if rec.class_ == 'error':
                        self._gdb_quit()
                    elif 'register_names' in dir(rec.result):
                        self.register_names = rec.result.register_names
                    elif 'register_values' in dir(rec.result):
                        self.register_values = rec.result.register_values
                elif rec.type == 'exec':
                    if rec.class_ == 'running':
                        if self.trace:
                            print('*** running')
                        self._put('')
                        self.running = True
                    elif rec.class_ == 'stopped':
                        if self.trace:
                            print('*** stopped')
                        self.running = False
                        #self._put('-data-list-register-values')
                elif rec.type == 'breakpoint':
                    if rec.class_ == 'breakpoint-created':
                        self.breakpoints[rec.result.bkpt.number] = rec.result.bkpt
                    elif rec.class_ == 'breakpoint-modified':
                        self.breakpoints[rec.result.bkpt.number] = rec.result.bkpt
                    elif rec.class_ == 'breakpoint-deleted':
                        if rec.result.id in self.breakpoints:
                            del self.breakpoints[rec.result.id]
            elif rec.record_type == 'error':
                self._gdb_quit()
            elif rec.record_type == 'stream':
                if rec.type == 'console' or rec.type == 'log':
                    for line in rec.value.splitlines():
                        self.gdb_console(line)
                if rec.type == 'target':
                    self.output_buffer += rec.value
                    last_lf = self.output_buffer.rfind('\n')
                    if last_lf >= 0:
                        lines = self.output_buffer[:last_lf]
                        if self.trace:
                            print('/// console output: ', len(lines))
                        for line in lines.splitlines():
                            self.output_length += len(line)
                            self.output(line)
                        self.output_buffer = self.output_buffer[last_lf + 1:]
        except:
            if self.trace:
                print('/// exception: console output')
            for line in lines.splitlines():
                self.output(line)

if __name__ == "__main__":
    import tester.rt.console
    stdtty = tester.rt.console.save()
    try:
        def output(text):
            print(']', text)
        def gdb_console(text):
            print('>', text)
        script = ['target sim']
        if len(sys.argv) > 1:
            executable = sys.argv[1]
            script += ['load',
                       'run',
                       'info reg',
                       '-stack-list-frames',
                       '-stack-list-arguments --all-values']
        else:
            executable = None
        script += ['quit']
        g = gdb('sparc', 'sis', mi_trace = True)
        g.open('sparc-rtems4.11-gdb', executable, output, gdb_console, script)
    except:
        tester.rt.console.restore(stdtty)
        raise
    finally:
        tester.rt.console.restore(stdtty)
