# SPDX-License-Identifier: BSD-2-Clause

#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2024 Kevin Kirspel
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
#   This tty uses a windows console application to obtain tty stdout data.
#

import errno
import datetime
import os
import sys
import time
import threading
import signal

from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import host
from rtemstoolkit import path

class tty:
    juart_active = False
    trace = False
    trace_reader = True
    lock_trace = False
    lock_locked = None
    lock = None
    dev = None
    process = None
    running = False
    output_length = 0
    output_buffer = ''
    ecode = None
    lc = 0
    juart_thread = None

    def __init__(self, dev, index, total):
        if not tty.juart_active and index == 1:
            tty.juart_active = True
            tty.dev = dev
            tty.lock = threading.RLock()
            tty.juart_thread = threading.Thread(target=self._juart_run, args=[tty.dev])
            tty.juart_thread.start()

    def __del__(self):
        pass

    def __str__(self):
        s = 'dev: %s' % ((tty.dev))
        return s

    def _lock(self, msg):
        if tty.lock_trace:
            print('|[   LOCK:%s ]|' % (msg))
            tty.lock_locked = datetime.datetime.now()
        tty.lock.acquire()

    def _unlock(self, msg):
        if tty.lock_trace:
            period = datetime.datetime.now() - tty.lock_locked
            print('|] UNLOCK:%s [| : %s' % (msg, period))
        tty.lock.release()

    def _output(self, line):
        sys.stdout.write(line)
        sys.stdout.flush()

    def _reader(self, line):
        if self.trace_reader:
            self._output(line)
        self._lock('_reader')
        try:
            tty.lc += 1
            tty.output_buffer += line
            tty.output_length += len(line)
        finally:
            self._unlock('_reader')

    def _juart_quit(self, backtrace = False):
        self._lock('_juart_quit')
        try:
            tty.process.send_signal(signal.SIGTERM)
        finally:
            self._unlock('_juart_quit')

    def _stop(self):
        self._lock('_stop')
        self._juart_quit()
        seconds = 5
        step = 0.1
        while tty.process and seconds > 0:
            if seconds > step:
                seconds -= step
            else:
                seconds = 0
            self._unlock('_stop')
            time.sleep(step)
            self._lock('_stop')
        seconds = 5
        while tty.running:
            if seconds > step:
                seconds -= step
            else:
                seconds = 0
            self._unlock('_stop')
            time.sleep(step)
            self._lock('_stop')
        if tty.process and seconds == 0:
            self._kill()

    def _kill(self):
        if tty.process:
            tty.process.kill()
        tty.process = None

    def _juart_run(self, args):
        '''Thread to run JUART application.'''
        tty.running = True
        self._open(args)
        tty.running = False

    def _execute_juart(self, args):
        '''Thread to execute JUART I/O processing and to wait for it to finish.'''
        cmds = args
        ecode, proc = tty.process.open(cmds)
        self._lock('_execute_juart')
        tty.ecode = ecode
        tty.process = None
        self._unlock('_execute_juart')
        if tty.trace:
            print('_execute_juart ecode = %s' % tty.ecode)

    def _monitor(self):
        output_length = tty.output_length
        step = 0.25
        while tty.process:
            self._unlock('_monitor')
            time.sleep(step)
            self._lock('_monitor')

    def _open(self, command):
        self._lock('_open')
        try:
            cmds = execute.arg_list(command)
            tty.process = execute.execute(output = self._reader)
            exec_thread = threading.Thread(target=self._execute_juart, args=[cmds])
            exec_thread.start()
            self._monitor()
            if tty.ecode is not None and tty.ecode > 0:
                raise error.general('juart exec: %s: %s' % (cmds[0], os.strerror(tty.ecode)))
        finally:
            self._unlock('_open')

    def off(self):
        self.is_on = False

    def on(self):
        self.is_on = True

    def set(self, flags):
        pass

    def read(self):
        try:
            self._lock('read')
            outs = tty.output_buffer;
            tty.output_buffer = '';
            tty.output_length = 0;
            self._unlock('read')
        except:
            outs = ''
        return outs

    def close(self, index, total):
        if index == total and tty.running and tty.juart_thread:
            try:
                if tty.trace:
                    print('stopping...')
                self._stop()
                if tty.trace:
                    print('stopping Done')
            except:
                if tty.trace:
                    print('stopping EXCEPTION')

if __name__ == "__main__":
    if len(sys.argv) == 2:
        import time
        t = tty(sys.argv[1])
        print(t)
        while True:
            time.sleep(0.05)
            c = t.read()
            sys.stdout.write(c)
