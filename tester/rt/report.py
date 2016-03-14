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
# RTEMS Testing Reports
#

import datetime
import os
import threading

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import path

class report(object):
    '''RTEMS Testing report.'''

    def __init__(self, total):
        self.lock = threading.Lock()
        self.total = total
        self.total_len = len(str(total))
        self.passed = 0
        self.failed = 0
        self.timeouts = 0
        self.invalids = 0
        self.invalid_tests = 0
        self.results = {}
        self.name_max_len = 0

    def __str__(self):
        msg  = 'Passed:   %*d%s' % (self.total_len, self.passed, os.linesep)
        msg += 'Failed:   %*d%s' % (self.total_len, self.failed, os.linesep)
        msg += 'Timeouts: %*d%s' % (self.total_len, self.timeouts, os.linesep)
        msg += 'Invalid:  %*d%s' % (self.total_len, self.invalids, os.linesep)
        return msg

    def set_invalid_tests(self, invalid_tests):
        self.invalid_tests = invalid_tests

    def start(self, index, total, name, executable, bsp_arch, bsp):
        header = '[%*d/%*d] p:%-*d f:%-*d t:%-*d i:%-*d | %s/%s: %s' % \
                 (len(str(total)), index,
                  len(str(total)), total,
                  len(str(total)), self.passed,
                  len(str(total)), self.failed,
                  len(str(total)), self.timeouts,
                  len(str(total)), self.invalids,
                  bsp_arch,
                  bsp,
                  path.basename(executable))
        self.lock.acquire()
        if name in self.results:
            self.lock.release()
            raise error.general('duplicate test: %s' % (name))
        self.results[name] = { 'index': index,
                               'bsp': bsp,
                               'bsp_arch': bsp_arch,
                               'exe': executable,
                               'start': datetime.datetime.now(),
                               'end': None,
                               'result': None,
                               'output': None,
                               'header': header }

        self.lock.release()
        log.notice(header, stdout_only = True)

    def end(self, name, output):
        start = False
        end = False
        timeout = False
        prefixed_output = []
        for line in output:
            if line[0] == ']':
                if line[1].startswith('*** '):
                    if line[1][4:].startswith('END OF '):
                        end = True
                    if line[1][4:].startswith('TIMEOUT TIMEOUT'):
                        timeout = True
                    else:
                        start = True
            prefixed_output += [line[0] + ' ' + line[1]]
        self.lock.acquire()
        if name not in self.results:
            self.lock.release()
            raise error.general('test report missing: %s' % (name))
        if self.results[name]['end'] is not None:
            self.lock.release()
            raise error.general('test already finished: %s' % (name))
        self.results[name]['end'] = datetime.datetime.now()
        if start and end:
            status = 'passed'
            self.passed += 1
        elif timeout:
            status = 'timeout'
            self.timeouts += 1
        elif start:
            if not end:
                status = 'failed'
                self.failed += 1
        else:
            if self.invalid_tests and path.basename(name) in self.invalid_tests:
                status = 'passed'
                self.passed += 1
            else:
                status = 'invalid'
                self.invalids += 1
        self.results[name]['result'] = status
        self.results[name]['output'] = prefixed_output
        if self.name_max_len < len(path.basename(name)):
            self.name_max_len = len(path.basename(name))
        self.lock.release()

    def log(self, name, mode):
        if mode != 'none':
            self.lock.acquire()
            if name not in self.results:
                self.lock.release()
                raise error.general('test report missing: %s' % (name))
            exe = path.basename(self.results[name]['exe'])
            result = self.results[name]['result']
            time = self.results[name]['end'] - self.results[name]['start']
            if mode != 'none':
                header = self.results[name]['header']
            if mode == 'all' or result != 'passed':
                output = self.results[name]['output']
            else:
                output = None
            self.lock.release()
            if header:
                log.output(header)
            if output:
                log.output(output)
                log.output('Result: %-10s Time: %s %s' % (result, str(time), exe))

    def summary(self):
        def show_state(results, state, max_len):
            for name in results:
                if results[name]['result'] == state:
                    log.output(' %s' % (path.basename(name)))
        log.output()
        log.notice('Passed:   %*d' % (self.total_len, self.passed))
        log.notice('Failed:   %*d' % (self.total_len, self.failed))
        log.notice('Timeouts: %*d' % (self.total_len, self.timeouts))
        log.notice('Invalid:  %*d' % (self.total_len, self.invalids))
        log.output('----------%s' % ('-' * self.total_len))
        log.notice('Total:    %*d' % (self.total_len, self.total))
        log.output()
        if self.failed:
            log.output('Failures:')
            show_state(self.results, 'failed', self.name_max_len)
        if self.timeouts:
            log.output('Timeouts:')
            show_state(self.results, 'timeout', self.name_max_len)
        if self.invalids:
            log.output('Invalid:')
            show_state(self.results, 'invalid', self.name_max_len)
