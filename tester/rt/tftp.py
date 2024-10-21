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
# RTEMS Testing TFTP Interface
#

from __future__ import print_function

import errno
import logging
import threading
import time
import sys

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import reraise

import tester.rt.tftpserver


class tftp(object):
    '''RTEMS Testing TFTP base.'''

    def __init__(self, bsp_arch, bsp, session_timeout, trace=False):
        self.session_timeout = session_timeout
        self.trace = trace
        self.lock_trace = False
        self.lock = threading.RLock()
        self.bsp = bsp
        self.bsp_arch = bsp_arch
        self._init()

    def __del__(self):
        self.kill()

    def _init(self, state='reset'):
        self.output_length = None
        self.console = None
        self.server = None
        self.port = 0
        self.exe = None
        self.timeout = None
        self.test_too_long = None
        self.timer = None
        self.opened = False
        self.running = False
        self.finished = False
        self.caught = None
        self.target_state = state

    def _lock(self, msg):
        if self.lock_trace:
            log.trace('|[   LOCK:%s ]|' % (msg))
        self.lock.acquire()
        if self.lock_trace:
            log.trace('|[   LOCK:%s done ]|' % (msg))

    def _unlock(self, msg):
        if self.lock_trace:
            log.trace('|] UNLOCK:%s [|' % (msg))
        self.lock.release()
        if self.lock_trace:
            log.trace('|[ UNLOCK:%s done ]|' % (msg))

    def _trace(self, text):
        if log.tracing:
            log.trace('] tftp: ' + text)

    def _console(self, text):
        if self.console:
            self.console(text)

    def _set_target_state(self, state):
        self._trace('set state: ' + state)
        self.target_state = state

    def _finished(self):
        self.server = None
        self.exe = None

    def _stop(self, finished=True):
        try:
            if self.server is not None:
                self.server.stop()
            self.finished = finished
            self._set_target_state('finished')
        except:
            pass

    def _run_finished(self):
        while self.opened and (self.running or not self.finished):
            self._unlock('_run_finished')
            time.sleep(0.2)
            self._lock('_run_finished')

    def _kill(self):
        self._stop()
        self._run_finished()

    def _timeout(self):
        self._stop()
        self._run_finished()
        if self.timeout is not None:
            self.timeout()

    def _test_too_long(self):
        self._stop()
        self._run_finished()
        if self.test_too_long is not None:
            self.test_too_long()

    def _exe_handle(self, req_file, raddress, rport):
        self._lock('_exe_handle')
        exe = self.exe
        self.exe = None
        self._unlock('_exe_handle')
        if exe is not None:
            self._console('tftp: %s' % (exe))
            return open(exe, "rb")
        self._console('tftp: re-requesting exe; target must have reset')
        self._stop()
        return None

    def _listener(self, exe):
        self.server = tester.rt.tftpserver.tftp_server(
            host='all',
            port=self.port,
            session_timeout=self.session_timeout,
            timeout=10,
            forced_file=exe,
            sessions=1)
        try:
            if False and log.tracing:
                self.server.trace_packets()
            self.server.start()
            return self.server.run() == 1
        except:
            self.server.stop()
            raise

    def _runner(self):
        self._trace('session: start')
        self._lock('_runner')
        exe = self.exe
        self.exe = None
        self._unlock('_runner')
        caught = None
        retry = 0
        target_loaded = False
        try:
            self._lock('_runner')
            state = self.target_state
            self._unlock('_runner')
            self._trace('runner: ' + state)
            while state not in ['shutdown', 'finished', 'timeout']:
                if state in ['booting', 'running']:
                    time.sleep(0.25)
                else:
                    self._trace('listening: begin: ' + state)
                    target_loaded = self._listener(exe)
                    self._lock('_runner')
                    if target_loaded:
                        self._set_target_state('booting')
                    else:
                        retry += 1
                        if retry > 1:
                            self._set_target_state('timeout')
                            self._timeout()
                    state = self.target_state
                    self._unlock('_runner')
                    self._trace('listening: end: ' + state)
                self._lock('_runner')
                state = self.target_state
                self._unlock('_runner')
        except:
            caught = sys.exc_info()
            self._trace('session: exception')
        self._lock('_runner')
        self._trace('runner: ' + self.target_state)
        self.caught = caught
        self.finished = True
        self.running = False
        self._unlock('_runner')
        self._trace('session: end')

    def open(self, executable, port, output_length, console, timeout):
        self._trace('open: start')
        self._lock('_open')
        if self.exe is not None:
            self._unlock('_open')
            raise error.general('tftp: already running')
        self._init()
        self.output_length = output_length
        self.console = console
        self.port = port
        self.exe = executable
        self.timeout = timeout[2]
        self.test_too_long = timeout[3]
        self.opened = True
        self.running = True
        self._console('tftp: exe: %s' % (executable))
        self.listener = threading.Thread(target=self._runner,
                                         name='tftp-listener')
        self.listener.daemon = True
        self._unlock('_open: start listner')
        self.listener.start()
        self._lock('_open: start listner')
        step = 0.25
        period = timeout[0]
        seconds = timeout[1]
        output_length = self.output_length()
        while not self.finished and period > 0 and seconds > 0:
            if not self.running and self.caught:
                break
            self._unlock('_open: loop')
            current_length = self.output_length()
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
            time.sleep(step)
            self._lock('_open: loop')
        self._trace('open: finished')
        if not self.finished:
            if period == 0:
                self._timeout()
            elif seconds == 0:
                self._test_too_long()
        caught = self.caught
        self._init('finished')
        self._unlock('_open')
        if caught is not None:
            reraise.reraise(*caught)

    def kill(self):
        self._trace('kill')
        self._lock('kill')
        self._set_target_state('shutdown')
        self._kill()
        self._unlock('kill')

    def target_restart(self, started):
        self._trace('target: restart: %d' % (started))
        self._lock('target_restart')
        try:
            if not started:
                if self.server is not None:
                    self._trace('server: stop')
                    self._stop(finished=False)
                    self._trace('server: stop done')
                state = 'booting'
            else:
                state = 'shutdown'
            self._set_target_state(state)
        finally:
            self._unlock('target_restart')

    def target_reset(self, started):
        self._trace('target: reset: %d' % (started))
        self._lock('target_reset')
        try:
            if not started:
                if self.server is not None:
                    self._trace('server: stop')
                    self.server.stop()
                state = 'booting'
            else:
                state = 'shutdown'
            self._set_target_state(state)
        finally:
            self._unlock('target_reset')

    def target_start(self):
        self._trace('target: start')
        self._lock('target_start')
        self._set_target_state('running')
        self._unlock('target_start')

    def target_end(self):
        self._trace('target: end')
        self._lock('target_end')
        self._set_target_state('finished')
        self._unlock('target_end')


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        executable = sys.argv[1]
    else:
        executable = None
    t = tftp('arm', 'beagleboneblack')
    t.open(executable)
