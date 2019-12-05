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

from __future__ import print_function

import datetime
import os
import threading

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import path

#
# Maybe this should be a configuration.
#
test_fail_excludes = [
    'minimum'
]

class report(object):
    '''RTEMS Testing report.'''

    def __init__(self, total):
        self.lock = threading.Lock()
        self.total = total
        self.total_len = len(str(total))
        self.passed = 0
        self.failed = 0
        self.user_input = 0
        self.expected_fail = 0
        self.indeterminate = 0
        self.benchmark = 0
        self.timeouts = 0
        self.invalids = 0
        self.wrong_version = 0
        self.wrong_build = 0
        self.wrong_tools = 0
        self.results = {}
        self.config = {}
        self.name_max_len = 0

    def __str__(self):
        msg  = 'Passed:        %*d%s' % (self.total_len, self.passed, os.linesep)
        msg += 'Failed:        %*d%s' % (self.total_len, self.failed, os.linesep)
        msg += 'User Input:    %*d%s' % (self.total_len, self.user_input, os.linesep)
        msg += 'Expected Fail: %*d%s' % (self.total_len, self.expected_fail, os.linesep)
        msg += 'Indeterminate: %*d%s' % (self.total_len, self.self.indeterminate, os.linesep)
        msg += 'Benchmark:     %*d%s' % (self.total_len, self.self.benchmark, os.linesep)
        msg += 'Timeout:       %*d%s' % (self.total_len, self.timeouts, os.linesep)
        msg += 'Invalid:       %*d%s' % (self.total_len, self.invalids, os.linesep)
        msg += 'Wrong Version  %*d%s' % (self.total_len, self.wrong_version, os.linesep)
        msg += 'Wrong Build    %*d%s' % (self.total_len, self.wrong_build, os.linesep)
        msg += 'Wrong Tools    %*d%s' % (self.total_len, self.wrong_tools, os.linesep)
        return msg

    def start(self, index, total, name, executable, bsp_arch, bsp, show_header):
        header = '[%*d/%*d] p:%-*d f:%-*d u:%-*d e:%-*d I:%-*d B:%-*d ' \
                 't:%-*d i:%-*d W:%-*d | %s/%s: %s' % \
                 (len(str(total)), index,
                  len(str(total)), total,
                  len(str(total)), self.passed,
                  len(str(total)), self.failed,
                  len(str(total)), self.user_input,
                  len(str(total)), self.expected_fail,
                  len(str(total)), self.indeterminate,
                  len(str(total)), self.benchmark,
                  len(str(total)), self.timeouts,
                  len(str(total)), self.invalids,
                  len(str(total)), self.wrong_version + self.wrong_build + self.wrong_tools,
                  bsp_arch,
                  bsp,
                  path.basename(name))
        self.lock.acquire()
        if name in self.results:
            self.lock.release()
            raise error.general('duplicate test: %s' % (name))
        self.results[name] = { 'index': index,
                               'bsp': bsp,
                               'bsp_arch': bsp_arch,
                               'exe': name,
                               'start': datetime.datetime.now(),
                               'end': None,
                               'result': None,
                               'output': None,
                               'header': header }

        self.lock.release()
        if show_header:
            log.notice(header, stdout_only = True)

    def end(self, name, output, output_prefix):
        start = False
        end = False
        state = None
        version = None
        build = None
        tools = None
        timeout = False
        prefixed_output = []
        for line in output:
            if line[0] == output_prefix:
                if line[1].startswith('*** '):
                    banner = line[1][4:]
                    if banner.startswith('BEGIN OF '):
                        start = True
                    elif line[1][4:].startswith('END OF '):
                        end = True
                    elif banner.startswith('TIMEOUT TIMEOUT'):
                        timeout = True
                    elif banner.startswith('TEST VERSION:'):
                        version = banner[13:].strip()
                    elif banner.startswith('TEST STATE:'):
                        # Depending on the RTESM version '-' or '_' is used in
                        # test state strings.  Normalize it to '_'.
                        state = banner[11:].strip().replace('-', '_')
                    elif banner.startswith('TEST BUILD:'):
                        build = ','.join(banner[11:].strip().split(' '))
                    elif banner.startswith('TEST TOOLS:'):
                        tools = banner[11:].strip()
            prefixed_output += [line[0] + ' ' + line[1]]
        self.lock.acquire()
        try:
            if name not in self.results:
                raise error.general('test report missing: %s' % (name))
            if self.results[name]['end'] is not None:
                raise error.general('test already finished: %s' % (name))
            self.results[name]['end'] = datetime.datetime.now()
            if state is not None and state not in ['BENCHMARK',
                                                   'EXPECTED_FAIL',
                                                   'INDETERMINATE',
                                                   'USER_INPUT']:
                if version:
                    if 'version' not in self.config:
                        self.config['version'] = version
                    else:
                        if version != self.config['version']:
                            state = 'WRONG_VERSION'
                if build:
                    if 'build' not in self.config:
                        self.config['build'] = build
                    else:
                        if build != self.config['build']:
                            state = 'WRONG_BUILD'
                if tools:
                    if 'tools' not in self.config:
                        self.config['tools'] = tools
                    else:
                        if tools != self.config['tools']:
                            state = 'WRONG_TOOLS'
            if state is None or state == 'EXPECTED_PASS':
                if start and end:
                    if state is None or state == 'EXPECTED_PASS':
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
                    exe_name = path.basename(name).split('.')[0]
                    if exe_name in test_fail_excludes:
                        status = 'passed'
                        self.passed += 1
                    else:
                        status = 'invalid'
                        self.invalids += 1
            else:
                if state == 'EXPECTED_FAIL':
                    if start and end:
                        status = 'passed'
                        self.passed += 1
                    else:
                        status = 'expected-fail'
                        self.expected_fail += 1
                elif state == 'USER_INPUT':
                    status = 'user-input'
                    self.user_input += 1
                elif state == 'INDETERMINATE':
                    if start and end:
                        status = 'passed'
                        self.passed += 1
                    else:
                        status = 'indeterminate'
                        self.indeterminate += 1
                elif state == 'BENCHMARK':
                    status = 'benchmark'
                    self.benchmark += 1
                elif state == 'WRONG_VERSION':
                    status = 'wrong-version'
                    self.wrong_version += 1
                elif state == 'WRONG_BUILD':
                    status = 'wrong-build'
                    self.wrong_build += 1
                elif state == 'WRONG_TOOLS':
                    status = 'wrong-tools'
                    self.wrong_tools += 1
                else:
                    raise error.general('invalid test state: %s: %s' % (name, state))
            self.results[name]['result'] = status
            self.results[name]['output'] = prefixed_output
            if self.name_max_len < len(path.basename(name)):
                self.name_max_len = len(path.basename(name))
        finally:
            self.lock.release()
        return status

    def log(self, name, mode):
        status_fails = ['failed', 'timeout', 'invalid',
                        'wrong-version', 'wrong-build', 'wrong-tools']
        if mode != 'none':
            self.lock.acquire()
            if name not in self.results:
                self.lock.release()
                raise error.general('test report missing: %s' % (name))
            exe = path.basename(self.results[name]['exe'])
            result = self.results[name]['result']
            time = self.results[name]['end'] - self.results[name]['start']
            failed = result in status_fails
            result = 'Result: %-10s Time: %s %s' % (result, str(time), exe)
            if mode != 'none':
                header = self.results[name]['header']
            if mode == 'all' or failed:
                output = self.results[name]['output']
            else:
                output = None
            self.lock.release()
            if header:
                log.output(header)
            if output:
                log.output(result)
                log.output(output)

    def get_config(self, config, not_found = None):
        if config in self.config:
            return self.config[config]
        return not_found

    def score_card(self, mode = 'full'):
        if mode == 'short':
            wrongs = self.wrong_version + self.wrong_build + self.wrong_tools
            return 'Passed:%d Failed:%d Timeout:%d Invalid:%d Wrong:%d' % \
                (self.passed, self.failed, self.timeouts, self.invalids, wrongs)
        elif mode == 'full':
            l = []
            l += ['Passed:        %*d' % (self.total_len, self.passed)]
            l += ['Failed:        %*d' % (self.total_len, self.failed)]
            l += ['User Input:    %*d' % (self.total_len, self.user_input)]
            l += ['Expected Fail: %*d' % (self.total_len, self.expected_fail)]
            l += ['Indeterminate: %*d' % (self.total_len, self.indeterminate)]
            l += ['Benchmark:     %*d' % (self.total_len, self.benchmark)]
            l += ['Timeout:       %*d' % (self.total_len, self.timeouts)]
            l += ['Invalid:       %*d' % (self.total_len, self.invalids)]
            l += ['Wrong Version: %*d' % (self.total_len, self.wrong_version)]
            l += ['Wrong Build:   %*d' % (self.total_len, self.wrong_build)]
            l += ['Wrong Tools:   %*d' % (self.total_len, self.wrong_tools)]
            l += ['---------------%s' % ('-' * self.total_len)]
            l += ['Total:         %*d' % (self.total_len, self.total)]
            return os.linesep.join(l)
        raise error.general('invalid socre card mode: %s' % (mode))

    def failures(self):
        def show_state(results, state, max_len):
            l = []
            for name in results:
                if results[name]['result'] == state:
                    l += [' %s' % (path.basename(name))]
            return l
        l = []
        if self.failed:
            l += ['Failures:']
            l += show_state(self.results, 'failed', self.name_max_len)
        if self.user_input:
            l += ['User Input:']
            l += show_state(self.results, 'user-input', self.name_max_len)
        if self.expected_fail:
            l += ['Expected Fail:']
            l += show_state(self.results, 'expected-fail', self.name_max_len)
        if self.indeterminate:
            l += ['Indeterminate:']
            l += show_state(self.results, 'indeterminate', self.name_max_len)
        if self.benchmark:
            l += ['Benchmark:']
            l += show_state(self.results, 'benchmark', self.name_max_len)
        if self.timeouts:
            l += ['Timeouts:']
            l += show_state(self.results, 'timeout', self.name_max_len)
        if self.invalids:
            l += ['Invalid:']
            l += show_state(self.results, 'invalid', self.name_max_len)
        if self.wrong_version:
            l += ['Wrong Version:']
            l += show_state(self.results, 'wrong-version', self.name_max_len)
        if self.wrong_build:
            l += ['Wrong Build:']
            l += show_state(self.results, 'wrong-build', self.name_max_len)
        if self.wrong_tools:
            l += ['Wrong Tools:']
            l += show_state(self.results, 'wrong-tools', self.name_max_len)
        return os.linesep.join(l)

    def summary(self):
        log.output()
        log.notice(self.score_card())
        log.output(self.failures())
