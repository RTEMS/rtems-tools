#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013-2020 Chris Johns (chrisj@rtems.org)
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
import shlex
import threading

from rtemstoolkit import configuration
from rtemstoolkit import config
from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import log
from rtemstoolkit import path

from rtemstoolkit import stacktraces

import tester.rt.console
import tester.rt.exe
import tester.rt.gdb
import tester.rt.tftp
import tester.rt.wait

timeout = 15

class file(config.file):
    """RTEMS Testing configuration."""

    _directives = ['%execute',
                   '%gdb',
                   '%tftp',
                   '%wait',
                   '%console']

    def __init__(self, index, total, report, name, opts,
                 console_prefix = ']', _directives = _directives):
        super(file, self).__init__(name, opts, directives = _directives)
        self.lock = threading.Lock()
        self.realtime_trace = self.exe_trace('output')
        self.console_trace = self.exe_trace('console')
        self.console_prefix = console_prefix
        self.show_header = not self.defined('test_disable_header')
        self.process = None
        self.console = None
        self.output = None
        self.index = index
        self.total = total
        self.report = report
        self.name = name
        self.timedout = False
        self.test_too_long = False
        self.test_started = False
        self.kill_good = False
        self.kill_on_end = False
        self.test_label = None
        self.target_start_regx = None
        self.target_reset_regx = None
        self.restarts = 0
        self.max_restarts = int(self.expand('%{max_restarts}'))

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

    def _test_too_long(self):
        self._lock()
        self.test_too_long = True
        self._unlock()
        self.capture('*** TEST TOO LONG')

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

    def _target_command(self, command, bsp_arch = None, bsp = None, exe = None, fexe = None):
        if self.defined('target_%s_command' % (command)):
            cmd = self.expand('%%{target_%s_command}' % (command)).strip()
            if bsp_arch is not None and '@ARCH@' in cmd:
                cmd = cmd.replace('@ARCH@', bsp_arch)
            if bsp is not None and '@BSP@' in cmd:
                cmd = cmd.replace('@BSP@', bsp)
            if exe is not None and '@EXE@' in cmd:
                cmd = cmd.replace('@EXE@', exe)
            if fexe is not None and '@FEXE@' in cmd:
                cmd = cmd.replace('@FEXE@', fexe)
            if len(cmd) > 0:
                output = ''
                if not self.opts.dry_run():
                    rs_proc = execute.capture_execution()
                    ec, proc, output = rs_proc.open(cmd, shell = True)
                self._capture_console('target %s: %s' % (command, cmd))
                if len(output) > 0:
                    output = os.linesep.join([' ' + l for l in output.splitlines()])
                    self._capture_console(output)

    def _target_exe_filter(self, exe):
        if self.defined('target_exe_filter'):
            f = self.expand('%{target_exe_filter}').strip()
            # Be like sed and use the first character as the delmiter.
            if len(f) > 0:
                delimiter = f[0]
                pat = ''
                repl = ''
                repl_not_pat = False
                esc = False
                for c in f[1:]:
                    add = True
                    if not esc and c == '\\':
                        esc = True
                        add = False
                    elif esc:
                        if c == delimiter:
                            c = delimiter
                        else:
                            c = '\\' + c
                            esc = False
                    elif c == delimiter:
                        if repl_not_pat:
                            exe = re.sub(r'%s' % (pat), repl, exe)
                            self._capture_console('target exe filter: find:%s subst:%s -> %s' % \
                                                  (pat, repl, exe))
                            return exe
                        repl_not_pat = True
                        add = False
                    if add:
                        if repl_not_pat:
                            repl += c
                        else:
                            pat += c
                raise error.general('invalid exe filter: %s' % (f))
        return exe

    def _output_length(self):
        self._lock()
        try:
            if self.test_started:
                l = len(self.output)
                return l
        finally:
            self._unlock()
        return 0

    def _capture_console(self, text):
        text = [('=> ', l) for l in text.replace(chr(13), '').splitlines()]
        if self.output is not None:
            if self.console_trace:
                self._realtime_trace(text)
            self.output += text

    def _dir_console(self, data):
        if self.console is not None:
            raise error.general(self._name_line_msg('console already configured'))
        if len(data) == 0:
            raise error.general(self._name_line_msg('no console configuration provided'))
        console_trace = trace = self.exe_trace('console')
        if not self.opts.dry_run():
            if data[0] == 'stdio':
                self.console = tester.rt.console.stdio(trace = console_trace)
            elif data[0] == 'tty':
                if len(data) < 2 or len(data) > 3:
                    raise error.general(self._name_line_msg('invalid tty configuration provided'))
                if len(data) == 3:
                    settings = data[2]
                else:
                    settings = None
                self.console = tester.rt.console.tty(data[1],
                                                     output = self.capture,
                                                     setup = settings,
                                                     trace = console_trace)
            else:
                raise error.general(self._name_line_msg('invalid console type'))

    def _dir_execute(self, data, total, index, rexe, bsp_arch, bsp):
        self.process = tester.rt.exe.exe(bsp_arch, bsp, trace = self.exe_trace('exe'))
        if not self.in_error:
            if self.console:
                self.console.open(index, total)
            if not self.opts.dry_run():
                self.process.open(data,
                                  ignore_exit_code = self.defined('exe_ignore_ret'),
                                  output = self.capture,
                                  console = self.capture_console,
                                  timeout = (int(self.expand('%{timeout}')),
                                             int(self.expand('%{max_test_period}')),
                                             self._timeout,
                                             self._test_too_long))
            if self.console:
                self.console.close(index, total)

    def _dir_gdb(self, data, total, index, exe, bsp_arch, bsp):
        if len(data) < 3 or len(data) > 4:
            raise error.general('invalid %gdb arguments')
        self.process = tester.rt.gdb.gdb(bsp_arch, bsp,
                                         trace = self.exe_trace('gdb'),
                                         mi_trace = self.exe_trace('gdb-mi'))
        script = self.expand('%%{%s}' % data[2])
        if script:
            script = [l.strip() for l in script.splitlines()]
        self.kill_on_end = True
        if not self.in_error:
            if self.console:
                self.console.open(index, total)
            if not self.opts.dry_run():
                self.process.open(data[0], data[1],
                                  script = script,
                                  output = self.capture,
                                  gdb_console = self.capture_console,
                                  timeout = (int(self.expand('%{timeout}')),
                                             int(self.expand('%{max_test_period}')),
                                             self._timeout,
                                             self._test_too_long))
            if self.console:
                self.console.close(index, total)

    def _dir_tftp(self, data, total, index, exe, bsp_arch, bsp):
        if len(data) != 2:
            raise error.general('invalid %tftp arguments')
        try:
            port = int(data[1])
        except:
            raise error.general('invalid %tftp port')
        self.kill_on_end = True
        if not self.opts.dry_run():
            if self.defined('session_timeout'):
                session_timeout = int(self.expand('%{session_timeout}'))
            else:
                session_timeout = 120
            self.process = tester.rt.tftp.tftp(bsp_arch, bsp,
                                               session_timeout = session_timeout,
                                               trace = self.exe_trace('tftp'))
            if not self.in_error:
                if self.console:
                    self.console.open(index, total)
                self.process.open(executable = exe,
                                  port = port,
                                  output_length = self._output_length,
                                  console = self.capture_console,
                                  timeout = (int(self.expand('%{timeout}')),
                                             int(self.expand('%{max_test_period}')),
                                             self._timeout,
                                             self._test_too_long))
                if self.console:
                    self.console.close(index, total)

    def _dir_wait(self, data, total, index, exe, bsp_arch, bsp):
        if len(data) != 0:
            raise error.general('%wait has no arguments')
        self.kill_on_end = True
        if not self.opts.dry_run():
            self.process = tester.rt.wait.wait(bsp_arch, bsp,
                                               trace = self.exe_trace('wait'))
            if not self.in_error:
                if self.console:
                    self.console.open(index, total)
                self.process.open(output_length = self._output_length,
                                  console = self.capture_console,
                                  timeout = (int(self.expand('%{timeout}')),
                                             int(self.expand('%{max_test_period}')),
                                             self._timeout,
                                             self._test_too_long))
                if self.console:
                    self.console.close(index, total)

    def _directive_filter(self, results, directive, info, data):
        if results[0] == 'directive':
            _directive = results[1]
            _data = results[2]
            ds = []
            if len(_data):
                ds = [_data[0]]
                if len(_data) > 1:
                    ds += shlex.split(_data[1])
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
                    bsp_arch = self.expand('%{arch}')
                    bsp = self.expand('%{bsp}')
                    fexe = self._target_exe_filter(exe)
                    self.report.start(index, total, exe, fexe,
                                      bsp_arch, bsp, self.show_header)
                    if self.index == 1:
                        self._target_command('on', bsp_arch, bsp, exe, fexe)
                    self._target_command('pretest', bsp_arch, bsp, exe, fexe)
                finally:
                    self._unlock()
                if _directive == '%execute':
                    self._dir_execute(ds, total, index, fexe, bsp_arch, bsp)
                elif _directive == '%gdb':
                    self._dir_gdb(ds, total, index, fexe, bsp_arch, bsp)
                elif _directive == '%tftp':
                    self._dir_tftp(ds, total, index, fexe, bsp_arch, bsp)
                elif _directive == '%wait':
                    self._dir_wait(ds, total, index, fexe, bsp_arch, bsp)
                else:
                    raise error.general(self._name_line_msg('invalid directive'))
                self._lock()
                if self.index == self.total:
                    self._target_command('off', bsp_arch, bsp, exe, fexe)
                self._target_command('posttest', bsp_arch, bsp, exe, fexe)
                try:
                    status = self.report.end(exe, self.output, self.console_prefix)
                    version = self.report.get_config('version', not_found = 'n/p')
                    build = self.report.get_config('build', not_found = 'n/p')
                    tools = self.report.get_config('tools', not_found = 'n/p')
                    self._capture_console('test result: %s' % (status))
                    self._capture_console('test version: %s' % (version))
                    self._capture_console('test build: %s' % (build))
                    self._capture_console('test tools: %s' % (tools))
                    if status == 'timeout':
                        if self.index != self.total:
                            self._target_command('reset', bsp_arch, bsp, exe, fexe)
                    self.process = None
                    self.output = None
                finally:
                    self._unlock()
        return None, None, None

    def _realtime_trace(self, text):
        self._unlock()
        try:
            for l in text:
                log.output(''.join(l))
        except:
            stacktraces.trace()
            raise
        finally:
            self._lock()

    def run(self):
        self.target_start_regx = self._target_regex('target_start_regex')
        self.target_reset_regx = self._target_regex('target_reset_regex')
        self.load(self.name)

    def capture(self, text):
        self._lock()
        try:
            if not self.test_started:
                s = text.find('*** BEGIN OF TEST ')
                if s >= 0:
                    self.test_started = True
                    e = text[s + 3:].find('***')
                    if e >= 0:
                        self.test_label = text[s + len('*** BEGIN OF TEST '):s + e + 3 - 1]
                    self._capture_console('test start: %s' % (self.test_label))
                    self.process.target_start()
            ok_to_kill = '*** TEST STATE: USER_INPUT' in text or \
                         '*** TEST STATE: BENCHMARK' in text
            if ok_to_kill:
                reset_target = True
            else:
                reset_target = False
            if ('*** TIMEOUT TIMEOUT' in text or \
                '*** TEST TOO LONG' in text) and \
                self.defined('target_reset_on_timeout'):
                reset_target = True
            restart = \
                (self.target_start_regx is not None and self.target_start_regx.match(text))
            if restart:
                if self.test_started:
                    self._capture_console('target start detected')
                    ok_to_kill = True
                else:
                    self.restarts += 1
                    if self.restarts > self.max_restarts:
                        self._capture_console('target restart maximum count reached')
                        ok_to_kill = True
                    else:
                        self.process.target_restart(self.test_started)
            if not reset_target and self.target_reset_regx is not None:
                if self.target_reset_regx.match(text):
                    self._capture_console('target reset condition detected')
                    self._target_command('reset')
                    self.process.target_reset(self.test_started)
            if self.kill_on_end:
                if not ok_to_kill and \
                   ('*** END OF TEST ' in text or \
                    '*** FATAL ***' in text or \
                    '[ RTEMS shutdown ]' in text or \
                    '*** TIMEOUT TIMEOUT' in text or \
                    '*** TEST TOO LONG' in text):
                    self._capture_console('test end: %s' % (self.test_label))
                    if self.test_label is not None:
                        ok_to_kill = \
                            '*** END OF TEST %s ***' % (self.test_label) in text or \
                            '*** FATAL ***' in text or \
                            '[ RTEMS shutdown ]' in text or \
                            '*** TIMEOUT TIMEOUT' in text or \
                            '*** TEST TOO LONG' in text
                    self.process.target_end()
            text = [(self.console_prefix, l) for l in text.replace(chr(13), '').splitlines()]
            if self.output is not None:
                if self.realtime_trace:
                    self._realtime_trace(text)
                self.output += text
            if reset_target:
                if self.index == self.total:
                    self._target_command('off')
                else:
                    self._target_command('reset')
        finally:
            self._unlock()
        if ok_to_kill:
            self._ok_kill()

    def capture_console(self, text):
        self._lock()
        try:
            self._capture_console(text)
        finally:
            self._unlock()

    def exe_trace(self, flag):
        dt = self.macros['exe_trace']
        if dt:
            if 'all' in dt or flag in dt.split(','):
                return True
        return False

    def kill(self):
        if self.process:
            try:
                self.process.kill()
            except:
                pass

def load(bsp, opts):
    mandatory = ['bsp', 'arch', 'tester']
    cfg = configuration.configuration()
    path_ = opts.defaults.expand('%%{_configdir}/bsps/%s.ini' % (bsp))
    ini_name = path.basename(path_)
    for p in path.dirname(path_).split(':'):
        if path.exists(path.join(p, ini_name)):
            cfg.load(path.join(p, ini_name))
            if not cfg.has_section(bsp):
                raise error.general('bsp section not found in ini: [%s]' % (bsp))
            item_names = cfg.get_item_names(bsp, err = False)
            for m in mandatory:
                if m not in item_names:
                    raise error.general('mandatory item not found in bsp section: %s' % (m))
            opts.defaults.set_write_map(bsp, add = True)
            for i in cfg.get_items(bsp, flatten = False):
                opts.defaults[i[0]] = i[1]
            if not opts.defaults.set_read_map(bsp):
                raise error.general('cannot set BSP read map: %s' % (bsp))
            # Get a copy of the required fields we need
            requires = cfg.comma_list(bsp, 'requires', err = False)
            del cfg
            user_config = opts.find_arg('--user-config')
            if user_config is not None:
                user_config = path.expanduser(user_config[1])
                if not path.exists(user_config):
                    raise error.general('cannot find user configuration file: %s' % (user_config))
            else:
                if 'HOME' in os.environ:
                    user_config = path.join(os.environ['HOME'], '.rtemstesterrc')
            if user_config:
                if path.exists(user_config):
                    cfg = configuration.configuration()
                    cfg.load(user_config)
                    if cfg.has_section(bsp):
                        for i in cfg.get_items(bsp, flatten = False):
                            opts.defaults[i[0]] = i[1]
            # Check for the required values.
            for r in requires:
                if opts.defaults.get(r) is None:
                    raise error.general('user value missing, BSP %s requires \'%s\': missing: %s' % \
                                        (bsp, ', '.join(requires), r))
            return opts.defaults['bsp']
    raise error.general('cannot find bsp configuration file: %s.ini' % (bsp))
