# SPDX-License-Identifier: BSD-2-Clause
'''Wait test target.'''

# Copyright (C) 2013-2021 Chris Johns (chrisj@rtems.org)
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
import threading
import time


class wait(object):
    '''RTEMS Testing WAIT base.'''

    # pylint: disable=useless-object-inheritance
    # pylint: disable=too-many-instance-attributes

    def __init__(self, bsp_arch, bsp, trace=False):
        self.trace = trace
        self.lock_trace = False
        self.lock_locked = None
        self.lock = threading.RLock()
        self.bsp = bsp
        self.bsp_arch = bsp_arch
        self._init()

    def _init(self):
        self.output_length = None
        self.console = None
        self.server = None
        self.timeout = None
        self.test_too_long = None
        self.timer = None
        self.waiting = False
        self.seconds = 0

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

    def _trace(self, text):
        if log.tracing:
            log.trace('] wait: ' + text)

    def _console(self, text):
        if self.console:
            self.console(text)

    def _kill(self):
        self._lock('_kill')
        self._console('wait: kill')
        self.waiting = False
        self._unlock('_kill')

    def _timeout(self):
        self._kill()
        if self.timeout is not None:
            self.timeout()

    def _test_too_long(self):
        self._kill()
        if self.test_too_long is not None:
            self.test_too_long()

    def _monitor(self, timeout):
        output_length = self.output_length
        period = timeout[0]
        seconds = timeout[1]
        self._console('wait: start: period=%d seconds=%d' % (period, seconds))
        step = 0.5
        period *= step
        seconds *= step
        self.waiting = True
        while self.waiting and period > 0 and seconds > 0:
            self._unlock('_monitor')
            current_length = self.output_length()
            if output_length != current_length:
                period = timeout[0] * step
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
            self._lock('_monitor')
        if self.waiting:
            if period == 0:
                self._timeout()
            elif seconds == 0:
                self._test_too_long()
        self._console('wait: finished: period=%d seconds=%d' %
                      (period, seconds))

    def open(self, output_length, console, timeout):
        '''Start to wait'''
        # pylint: disable=too-many-arguments
        self._lock('_open')
        self._init()
        self.output_length = output_length
        self.console = console
        self.timeout = timeout[2]
        self.test_too_long = timeout[3]
        self._monitor(timeout)

    def kill(self):
        '''Kill the test run.'''
        self._kill()

    def target_restart(self, started):
        pass

    def target_reset(self, started):
        pass

    def target_start(self):
        pass

    def target_end(self):
        pass
