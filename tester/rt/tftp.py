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
# RTEMS Testing TFTP Interface
#

from __future__ import print_function

import errno
import logging
import threading
import time
import sys

from rtemstoolkit import error
from rtemstoolkit import reraise

import tftpserver

class tftp(object):
    '''RTEMS Testing TFTP base.'''

    def __init__(self, bsp_arch, bsp, trace = False):
        self.trace = trace
        self.lock_trace = False
        self.lock = threading.RLock()
        self.bsp = bsp
        self.bsp_arch = bsp_arch
        self._init()

    def __del__(self):
        self.kill()

    def _init(self):
        self.output_length = None
        self.console = None
        self.server = None
        self.port = 0
        self.exe = None
        self.timeout = None
        self.timer = None
        self.running = False
        self.finished = False
        self.caught = None

    def _lock(self, msg):
        if self.lock_trace:
            print('|[   LOCK:%s ]|' % (msg))
        self.lock.acquire()

    def _unlock(self, msg):
        if self.lock_trace:
            print('|] UNLOCK:%s [|' % (msg))
        self.lock.release()

    def _finished(self):
        self.server = None
        self.exe = None

    def _stop(self):
        try:
            if self.server:
                self.server.stop()
            self.finished = True
        except:
            pass

    def _kill(self):
        self._stop()
        while self.running or not self.finished:
            self._unlock('_kill')
            time.sleep(0.1)
            self._lock('_kill')

    def _timeout(self):
        self._stop()
        while self.running or not self.finished:
            self._unlock('_timeout')
            time.sleep(0.1)
            self._lock('_timeout')
        if self.timeout is not None:
            self.timeout()

    def _exe_handle(self, req_file, raddress, rport):
        self._lock('_exe_handle')
        exe = self.exe
        self.exe = None
        self._unlock('_exe_handle')
        if exe is not None:
            if self.console:
                self.console('tftp: %s' % (exe))
            return open(exe, "rb")
        if self.console:
            self.console('tftp: re-requesting exe; target must have reset')
        self._stop()
        return None

    def _listener(self):
        self._lock('_listener')
        exe = self.exe
        self.exe = None
        self._unlock('_listener')
        self.server = tftpserver.tftp_server(host = 'all',
                                             port = self.port,
                                             timeout = 1,
                                             forced_file = exe,
                                             sessions = 1)
        try:
            self.server.start()
            self.server.run()
        except:
            self.server.stop()
            raise

    def _runner(self):
        self._lock('_runner')
        self.running = True
        self._unlock('_runner')
        caught = None
        try:
            self._listener()
        except:
            caught = sys.exc_info()
        self._lock('_runner')
        self.running = False
        self.caught = caught
        self._unlock('_runner')

    def open(self, executable, port, output_length, console, timeout):
        self._lock('_open')
        if self.exe is not None:
            self._unlock('_open')
            raise error.general('tftp: already running')
        self._init()
        self.output_length = output_length
        self.console = console
        self.port = port
        self.exe = executable
        if self.console:
            self.console('tftp: exe: %s' % (executable))
        self.timeout = timeout[1]
        self.listener = threading.Thread(target = self._runner,
                                         name = 'tftp-listener')
        self.listener.start()
        step = 0.5
        period = timeout[0]
        output_len = self.output_length()
        while not self.finished and period > 0:
            current_length = self.output_length()
            if output_length != current_length:
                period = timeout[0]
            output_length = current_length
            if period < step:
                period = 0
            else:
                period -= step
            self._unlock('_open')
            time.sleep(step)
            self._lock('_open')
        if not self.finished and period == 0:
            self._timeout()
        caught = self.caught
        self.caught = None
        self._init()
        self._unlock('_open')
        if caught is not None:
            reraise.reraise(*caught)

    def kill(self):
        self._lock('kill')
        self._kill()
        self._unlock('kill')

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        executable = sys.argv[1]
    else:
        executable = None
    t = tftp('arm', 'beagleboneblack')
    t.open(executable)
