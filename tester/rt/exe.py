# SPDX-License-Identifier: BSD-2-Clause
'''Executable test target.'''

# Copyright (C) 2013-2020 Chris Johns (chrisj@rtems.org)
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

import datetime
import os
import sys
import threading
import time

from rtemstoolkit import execute


class exe(object):
    '''RTEMS Testing EXE base.'''

    # pylint: disable=useless-object-inheritance
    # pylint: disable=too-many-instance-attributes

    def __init__(self, bsp_arch, bsp, trace=False):
        self.trace = trace
        self.lock_trace = False
        self.lock_locked = None
        self.lock = threading.RLock()
        self.bsp = bsp
        self.bsp_arch = bsp_arch
        self.output = None
        self.output_length = 0
        self.process = None
        self.ecode = None
        self.output = None
        self.output_buffer = ''
        self.console = None
        self.timeout = None
        self.test_too_long = None
        self.kill_good = True
        self.result = None

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

    def _capture(self, text):
        self._lock('_capture')
        self.output_length += len(text)
        self._unlock('_capture')
        if self.output is not None:
            self.output(text)

    def _timeout(self):
        self._kill()
        if self.timeout is not None:
            self.timeout()

    def _test_too_long(self):
        self._kill()
        if self.test_too_long is not None:
            self.test_too_long()

    def _kill(self):
        self._lock('_kill')
        self.kill_good = True
        self._unlock('_kill')
        if self.process:
            # pylint: disable=bare-except
            try:
                self.process.kill()
            except:
                pass
        self.process = None

    def _execute(self, args):
        '''Thread to execute the test and to wait for it to finish.'''
        # pylint: disable=unused-variable
        try:
            cmds = args
            if self.console is not None:
                self.console('exe: %s' % (' '.join(cmds)))
                ecode, proc = self.process.open(cmds)
            if self.trace:
                print('exe done', ecode)
            self._lock('_execute')
            self.ecode = ecode
            self.process = None
            self._unlock('_execute')
        except:
            self.result = sys.exc_info()

    def _monitor(self, timeout):
        output_length = self.output_length
        step = 0.25
        period = timeout[0] / step
        seconds = timeout[1] / step
        while self.process and period > 0 and seconds > 0:
            if self.result is not None:
                raise self.result[1]
            current_length = self.output_length
            if output_length != current_length:
                period = timeout[0] / step
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

    def open(self, command, ignore_exit_code, output, console, timeout):
        '''Open the execute test run'''
        # pylint: disable=too-many-arguments
        self._lock('_open')
        self.timeout = timeout[2]
        self.test_too_long = timeout[3]
        try:
            cmds = execute.arg_list(command)
            self.output = output
            self.console = console
            self.process = execute.execute(output=self._capture)
            exec_thread = threading.Thread(target=self._execute, args=[cmds])
            exec_thread.start()
            self._monitor(timeout)
            if self.ecode is not None and \
               not (self.kill_good or ignore_exit_code) and self.ecode > 0:
                if self.output:
                    self.output('*** TARGET ERROR %d %s ***' %
                                (self.ecode, os.strerror(self.ecode)))
        finally:
            self._unlock('_open')

    def kill(self):
        '''Kill the test run.'''
        self._lock('_kill')
        try:
            self._kill()
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
