#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2016-2018 Chris Johns (chrisj@rtems.org)
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

from __future__ import print_function

import argparse
import copy
import datetime
import operator
import os
import re
import sys
import textwrap
import threading
import time
import traceback

import pprint

from rtemstoolkit import configuration
from rtemstoolkit import execute
from rtemstoolkit import error
from rtemstoolkit import host
from rtemstoolkit import log
from rtemstoolkit import mailer
from rtemstoolkit import path
from rtemstoolkit import textbox
from rtemstoolkit import version

#
# Group loggin entries together.
#
log_lock = threading.Lock()

#
# The max build label size in the jobs list.
#
max_build_label = 0

def rtems_version():
    return version.version()

def wrap(line, lineend = '', indent = 0, width = 75):
    if type(line) is tuple or type(line) is list:
        if len(line) >= 2:
            s1 = line[0]
        else:
            s1 = ''
        s2 = line[1:]
    elif type(line) is str:
        s1 = ''
        s2 = [line]
    else:
        raise error.internal('line is not a tuple, list or string')
    s1len = len(s1)
    s = ''
    first = True
    for ss in s2:
        if type(ss) is not str and type(ss) is not unicode:
            raise error.internal('text needs to be a string')
        for l in textwrap.wrap(ss, width = width - s1len - indent - 1):
            s += '%s%s%s%s%s' % (' ' * indent, s1, l, lineend, os.linesep)
            if first and s1len > 0:
                s1 = ' ' * s1len
    if lineend != '':
        s = s[:0 - len(os.linesep) - 1] + os.linesep
    return s

def comma_split(options):
    if options is None:
        return None
    return [o.strip() for o in options.split(',')]

def title():
    return 'RTEMS Tools Project - RTEMS Kernel BSP Builder, %s' % (version.str())

def command_line():
    return wrap(('command: ', ' '.join(sys.argv)), lineend = '\\')

def jobs_option_parse(jobs_option):
    try:
        if '/' not in jobs_option:
            return 1, int(jobs_option)
        jos = jobs_option.split('/')
        if len(jos) != 2:
            raise error.general('invalid jobs option: %s' % (jobs_option))
        return int(jos[0]), int(jos[1])
    except:
        pass
    raise error.general('invalid jobs option: %s' % (jobs_option))

def arch_bsp_build_parse(build):
    if type(build) is str:
        build_key = build
    else:
        build_key = build.key()
    abb = build_key.split('.')
    if len(abb) != 2:
        raise error.general('invalid build key: %s' % (build_key))
    ab = abb[0].split('/')
    if len(ab) != 2:
        raise error.general('invalid build key: %s' % (build_key))
    return ab[0], ab[1], abb[1]

def set_max_build_label(jobs):
    global max_build_label
    for job in jobs:
        if len(job.build.key()) > max_build_label:
            max_build_label = len(job.build.key())
    max_build_label += 2

class arch_bsp_build:

    def __init__(self, arch, bsp, build, build_config):
        self.arch = arch
        self.bsp = bsp
        self.build = build
        self.build_config = build_config
        self.start_time = None
        self.stop_time = None

    def __str__(self):
        return self.key() + ': ' + self.build_config

    def key(self):
        return '%s/%s.%s' % (self.arch, self.bsp, self.build)

    def get_arch_bsp(self):
        return self.arch, self.bsp

    def start(self):
        self.start_time = datetime.datetime.now()

    def stop(self):
        self.stop_time = datetime.datetime.now()

    def duration(self):
        return self.stop_time - self.start_time

class output_worker:

    def __init__(self, we, build):
        self.text = []
        self.warnings_errors = we
        self.build = build

    def output(self, text):
        self.warnings_errors.process_output(text, self.build)
        self.text += text.splitlines()

    def log_output(self, heading):
        log_lock.acquire()
        try:
            log.output(heading + self.text)
        except:
            raise
        finally:
            log_lock.release()

class warnings_errors:

    def __init__(self, source_base, groups):
        self.lock = threading.Lock()
        self.source_base = path.host(source_base)
        self.groups = groups
        self.reset()

    def _get_warnings(self, build):
        self.lock.acquire()
        warnings = [w for w in self.warnings]
        self.lock.release()
        return sorted(warnings)

    def _total(self, archive):
        total = 0
        for a in archive:
            total += archive[a]
        return total

    def _analyze(self, warnings, exclude):
        def _group(data, category, name, warning, count, groups, group_regx):
            if 'groups' not in data:
                data['groups'] = { }
            if category not in data['groups']:
                data['groups'][category] = { }
            if 'totals' not in data['groups'][category]:
                data['groups'][category]['totals'] = { }
            if name not in data['groups'][category]:
                data['groups'][category][name] = { }
            for group in groups:
                if group not in data['groups'][category]['totals']:
                    data['groups'][category]['totals'][group] = 0
                if group not in data['groups'][category][name]:
                    data['groups'][category][name][group] = 0
                if group_regx[group].match(warning):
                    data['groups'][category][name][group] += count
                    data['groups'][category]['totals'][group] += count
                    break

        def _update(data, category, name, warning, count, groups, group_regx):
            if category not in data:
                data[category] = { }
            if name not in data[category]:
                data[category][name] = { }
            if warning not in data[category][name]:
                data[category][name][warning] = 0
            data[category][name][warning] += count
            _group(data, category, name,  w, count, groups, group_regx)

        categories = ['arch', 'arch_bsp', 'build']
        data = { 'groups': { } }
        for category in categories:
            data[category] = { }
            data['groups'][category] = { }
        group_regx = { }
        for group in self.groups['groups']:
            group_regx[group] = re.compile(self.groups[group])
        exclude_regx = re.compile(exclude)
        for warning in self.warnings:
            arch, bsp, build = arch_bsp_build_parse(warning)
            arch_bsp = '%s/%s' % (arch, bsp)
            for w in self.warnings[warning]:
                if not exclude_regx.match(w):
                    count = self.warnings[warning][w]
                    _update(data, 'arch', arch, w, count,
                           self.groups['groups'], group_regx)
                    _update(data, 'arch_bsp', arch_bsp, w, count,
                           self.groups['groups'], group_regx)
                    _update(data, 'build', build, w, count,
                           self.groups['groups'], group_regx)
        for category in categories:
            common = {}
            for name in data[category]:
                for w in data[category][name]:
                    if w not in common:
                        for other in [n for n in data[category] if n != name]:
                            if w in data[category][other]:
                                common[w] = data[category][name][w]
                                _group(data, category, 'common', w, common[w],
                                       self.groups['groups'], group_regx)
            data[category]['common'] = common
        return data

    def _report_category(self, label, warnings, group_counts, summary):
        width = 70
        cols_1 = [width]
        cols_2 = [8, width - 8]
        cols_4 = textbox.even_columns(4, width)
        cols_2_4 = textbox.merge_columns([cols_2, cols_4])
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s += textbox.row(cols_1, [' ' + label], indent = 1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        builds = ['common'] + sorted([b for b in warnings if b != 'common'])
        common = warnings['common']
        for build in builds:
            build_warnings = warnings[build]
            if build is not 'common':
                build_warnings = [w for w in build_warnings if w not in common]
            s += textbox.row(cols_1,
                             [' %s : %d warning(s)' % (build,
                                                       len(build_warnings))],
                             indent = 1)
            if len(build_warnings) == 0:
                s += textbox.line(cols_1, marker = '+', indent = 1)
            else:
                s += textbox.line(cols_4, marker = '+', indent = 1)
                if build not in group_counts:
                    gs = [0 for group in self.groups['groups']]
                else:
                    gs = []
                    for g in range(0, len(self.groups['groups'])):
                        group = self.groups['groups'][g]
                        if group in group_counts[build]:
                            count = group_counts[build][group]
                        else:
                            count = 0
                        gs += ['%*s' % (cols_4[g % 4] - 2,
                                        '%s : %4d' % (group, count))]
                    for row in range(0, len(self.groups['groups']), 4):
                        if row + 4 > len(self.groups['groups']):
                            d = gs[row:] + \
                                ['' for r in range(row,
                                                   len(self.groups['groups']))]
                        else:
                            d = gs[row:+4]
                        s += textbox.row(cols_4, d, indent = 1)
                s += textbox.line(cols_2_4, marker = '+', indent = 1)
                if not summary:
                    vw = sorted([(w, warnings[build][w]) for w in build_warnings],
                                key = operator.itemgetter(1),
                                reverse = True)
                    for w in vw:
                        c1 = '%6d' % w[1]
                        for l in textwrap.wrap(' ' + w[0], width = cols_2[1] - 3):
                            s += textbox.row(cols_2, [c1, l], indent = 1)
                            c1 = ' ' * 6
                    s += textbox.line(cols_2, marker = '+', indent = 1)
        return s

    def _report_warning_map(self):
        builds = self.messages['warnings']
        width = 70
        cols_1 = [width]
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s += textbox.row(cols_1, [' Warning Map'], indent = 1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        for build in builds:
            messages = builds[build]
            s += textbox.row(cols_1, [' %s : %d' % (build, len(messages))], indent = 1)
            s += textbox.line(cols_1, marker = '+', indent = 1)
            for msg in messages:
                for l in textwrap.wrap(msg, width = width - 3):
                    s += textbox.row(cols_1, [' ' + l], indent = 1)
                for l in textwrap.wrap(messages[msg], width = width - 3 - 4):
                    s += textbox.row(cols_1, ['    ' + l], indent = 1)
            s += textbox.line(cols_1, marker = '+', indent = 1)
        return s

    def warnings_report(self, summary = False):
        self.lock.acquire()
        s = ' No warnings' + os.linesep
        try:
            total = 0
            for build in self.warnings:
                total += self._total(self.warnings[build])
            if total != 0:
                data = self._analyze(self.warnings, self.groups['exclude'])
                s = self._report_category('By Architecture (total : %d)' % (total),
                                          data['arch'], data['groups']['arch'],
                                          summary)
                s += os.linesep
                s += self._report_category('By BSP (total : %d)' % (total),
                                           data['arch_bsp'], data['groups']['arch_bsp'],
                                           summary)
                s += os.linesep
                s += self._report_category('By Build (total : %d)' % (total),
                                           data['build'], data['groups']['build'],
                                           summary)
                s += os.linesep
                if not summary:
                    s += self._report_warning_map()
                    s += os.linesep
        finally:
            self.lock.release()
        return s

    def clear_build(self, build):
        self.lock.acquire()
        self.warnings[build.key()] = {}
        self.errors[build.key()] = {}
        self.lock.release()

    def get_warning_count(self):
        self.lock.acquire()
        count = self.warning_count
        self.lock.release()
        return count

    def get_error_count(self):
        self.lock.acquire()
        count = self.error_count
        self.lock.release()
        return count

    def reset(self):
        self.lock.acquire()
        self.warnings = { }
        self.warning_count = 0
        self.errors = { }
        self.error_count = 0
        self.messages = { 'warnings' : { }, 'errors' : { } }
        self.lock.release()

    def _get_messages(self, build, key):
        self.lock.acquire()
        if type(build) is str:
            build_key = build
        else:
            build_key = build.key()
        if build_key not in self.messages[key]:
            messages = []
        else:
            messages = self.messages[key][build_key]
        messages = ['%s %s' % (m, messages[m]) for m in messages]
        self.lock.release()
        return messages

    def get_warning_messages(self, build):
        return self._get_messages(build, 'warning')

    def get_error_messages(self, build):
        return self._get_messages(build, 'errors')

    def process_output(self, text, build):
        def _line_split(line, source_base):
            if line.count(':') < 2:
                return None
            ls = line.split(' ', 1)
            fname = ls[0].strip().split(':', 2)
            if len(fname) != 3:
                return None
            p = path.abspath(fname[0])
            p = p.replace(source_base, '')
            if path.isabspath(p):
                p = p[1:]
            if len(fname[2]) == 0:
                pos = None
            else:
                pos = fname[2]
            return p, fname[1], pos, ls[1]

        def _create_build_errors(build, archive):
            if build.key() not in archive:
                archive[build.key()] = { }
            return archive[build.key()]

        #
        # The GNU linker does not supply 'error' in error messages. There is no
        # line information which is understandable. Look for `bin/ld:` and
        # `collect2:` in the output and then create the error when `collect2:`
        # is seen.
        #
        # The order we inspect each line is important.
        #
        if ' warning:' in text or \
           ' error:' in text or \
           ' Error:' in text or \
           'bin/ld:' in text:
            self.lock.acquire()
            try:
                for l in text.splitlines():
                    if 'bin/ld:' in l:
                        archive =_create_build_errors(build, self.errors)
                        if 'linker' not in archive:
                            archive['linker'] = []
                        archive['linker'] += [l.split(':', 1)[1].strip()]
                        messages = 'errors'
                    elif l.startswith('collect2:'):
                        archive =_create_build_errors(build, self.errors)
                        l = '/ld/collect2:0: error: '
                        if 'linker' not in archive or len(archive['linker']) == 0:
                            l += 'no error message found!'
                        else:
                            l += '; '.join(archive['linker'])
                            archive['linker'] = []
                        messages = 'errors'
                    elif ' warning:' in l:
                        self.warning_count += 1
                        archive = _create_build_errors(build, self.warnings)
                        messages = 'warnings'
                    elif ' error:' in l or ' Error:' in l:
                        self.error_count += 1
                        archive =_create_build_errors(build, self.errors)
                        messages = 'errors'
                    else:
                        continue
                    line_parts = _line_split(l, self.source_base)
                    if line_parts is not None:
                        src, line, pos, msg = line_parts
                        if pos is not None:
                            where = '%s:%s:%s' % (src, line, pos)
                        else:
                            where = '%s:%s' % (src, line)
                        if where not in archive:
                            archive[where] = 1
                        else:
                            archive[where] += 1
                        if build.key() not in self.messages[messages]:
                            self.messages[messages][build.key()] = { }
                        self.messages[messages][build.key()][where] = msg
            finally:
                self.lock.release()

class results:

    def __init__(self, source_base, groups):
        self.lock = threading.Lock()
        self.errors = { 'pass':      0,
                        'configure': 0,
                        'build':     0,
                        'tests':     0,
                        'passes':    { },
                        'fails':     { } }
        self.counts = { 'h'        : 0,
                        'exes'     : 0,
                        'objs'     : 0,
                        'libs'     : 0 }
        self.warnings_errors = warnings_errors(source_base, groups)

    def _arch_bsp(self, arch, bsp):
        return '%s/%s' % (arch, bsp)

    def _arch_bsp_passes(self, build):
        if build.key() not in self.errors['passes']:
            self.errors['passes'][build.key()] = []
        return self.errors['passes'][build.key()]

    def _arch_bsp_fails(self, build):
        if build.key() not in self.errors['fails']:
            self.errors['fails'][build.key()] = []
        return self.errors['fails'][build.key()]

    def _count(self, label):
        count = 0
        for build in self.errors[label]:
            count += len(self.errors[label][build])
        return count

    def _max_col(self, label):
        max_col = 0
        for build in self.errors[label]:
            arch, bsp, build_config = arch_bsp_build_parse(build)
            arch_bsp = self._arch_bsp(arch, bsp)
            if len(arch_bsp) > max_col:
                max_col = len(arch_bsp)
        return max_col

    def get_warning_count(self):
        return self.warnings_errors.get_warning_count()

    def get_error_count(self):
        return self.warnings_errors.get_error_count()

    def get_warning_messages(self, build):
        return self.warnings_errors.get_warning_messages(build)

    def get_error_messages(self, build):
        return self.warnings_errors.get_error_messages(build)

    def status(self):
        self.lock.acquire()
        try:
            s = 'Pass: %4d  Fail: %4d (configure:%d build:%d)' % \
                (self.errors['pass'],
                 self.errors['configure'] + self.errors['build'],
                 self.errors['configure'], self.errors['build'])
        except:
            raise
        finally:
            self.lock.release()
        return s;

    def add_fail(self, phase, build, configure, warnings, error_messages):
        fails = self._arch_bsp_fails(build)
        self.lock.acquire()
        try:
            self.errors[phase] += 1
            fails += [(phase, build.build_config, configure, error_messages)]
        finally:
            self.lock.release()

    def add_pass(self, build, configure, warnings):
        passes = self._arch_bsp_passes(build)
        self.lock.acquire()
        try:
            self.errors['pass'] += 1
            passes += [(build.build_config, configure, warnings, None)]
        finally:
            self.lock.release()

    def pass_count(self):
        return self._count('passes')

    def fail_count(self):
        return self._count('fails')

    def _failures_report(self, build, count):
        if type(build) is str:
            build_key = build
        else:
            build_key = build.key()
        if build_key not in self.errors['fails'] or \
           len(self.errors['fails'][build_key]) == 0:
            return count, 0, ' No failures'
        absize = 0
        bsize = 0
        ssize = 0
        arch, bsp, build_set = arch_bsp_build_parse(build_key)
        arch_bsp = self._arch_bsp(arch, bsp)
        fails = self.errors['fails'][build_key]
        for f in fails:
            if len(f[0]) > ssize:
                ssize = len(f[0])
        s = ''
        for f in fails:
            count += 1
            fcl = ' %3d' % (count)
            state = f[0]
            s += '%s %s %s %-*s:%s' % \
                 (fcl, build_set, arch_bsp, ssize, state, os.linesep)
            s1 = ' ' * 6
            s += wrap((s1, 'configure: ' + f[2]), lineend = '\\', width = 75)
            s1 = ' ' * 5
            for e in self.warnings_errors.get_error_messages(build):
                s += wrap([s1 + 'error: ', e])
        return count, len(fails), s

    def failures_report(self, build = None):
        s = ''
        count = 0
        if build is not None:
            count, build_fails, bs = self._failures_report(build, count)
            if build_fails > 0:
                s += bs + os.linesep
        else:
            self.lock.acquire()
            builds = sorted(self.errors['fails'].keys())
            self.lock.release()
            for build in builds:
                count, build_fails, bs = self._failures_report(build, count)
                if build_fails > 0:
                    s += bs + os.linesep
        if count == 0:
            s = ' No failures' + os.linesep
        return s

    def warnings_report(self, summary = False):
        return self.warnings_errors.warnings_report(summary)

    def report(self):
        self.lock.acquire()
        log_lock.acquire()
        passes = self.pass_count()
        fails = self.fail_count()
        log.notice('Passes: %d   Failures: %d' % (passes, fails))
        log.output()
        log.output('Build Report')
        log.output('   Passes: %d   Failures: %d' % (passes, fails))
        log.output(' Failures:')
        if fails == 0:
            log.output('  None')
        else:
            max_col = self._max_col('fails')
            for build in self.errors['fails']:
                arch, bsp, build_config = arch_bsp_build_parse(build)
                arch_bsp = self._arch_bsp(arch, bsp)
                for f in self.errors['fails'][build]:
                    config_cmd = f[2]
                    config_at = config_cmd.find('configure')
                    if config_at != -1:
                        config_cmd = config_cmd[config_at:]
                    log.output(' %*s:' % (max_col + 2, arch_bsp))
                    s1 = ' ' * 6
                    log.output(wrap([s1, config_cmd], lineend = '\\', width = 75))
                    if f[3] is not None:
                        s1 = ' ' * len(s1)
                        for msg in f[3]:
                            log.output(wrap([s1, msg], lineend = '\\'))
        log.output(' Passes:')
        if passes == 0:
            log.output('  None')
        else:
            max_col = self._max_col('passes')
            for build in self.errors['passes']:
                arch, bsp, build_config = arch_bsp_build_parse(build)
                arch_bsp = self._arch_bsp(arch, bsp)
                for f in self.errors['passes'][build]:
                    config_cmd = f[1]
                    config_at = config_cmd.find('configure')
                    if config_at != -1:
                        config_cmd = config_cmd[config_at:]
                    log.output(' %s (%5d):' % (arch_bsp, f[2]))
                    log.output(wrap([' ' * 6, config_cmd], lineend = '\\', width = 75))
        log_lock.release()
        self.lock.release()

class arch_bsp_builder:

    def __init__(self, results_, build, commands, build_dir, tag):
        self.lock = threading.Lock()
        self.state = 'ready'
        self.thread = None
        self.proc = None
        self.results = results_
        self.build = build
        self.commands = commands
        self.build_dir = build_dir
        self.tag = tag
        self.output = output_worker(results_.warnings_errors, build)
        self.counts = { 'h'        : 0,
                        'exes'     : 0,
                        'objs'     : 0,
                        'libs'     : 0 }

    def _notice(self, text):
        global max_build_label
        arch, bsp, build_set = arch_bsp_build_parse(self.build.key())
        label = '%s/%s (%s)' % (arch, bsp, build_set)
        log.notice('[%s] %-*s %s' % (self.tag, max_build_label, label, text))

    def _build_dir(self):
        return path.join(self.build_dir, self.build.key())

    def _make_build_dir(self):
        if not path.exists(self._build_dir()):
            path.mkdir(self._build_dir())

    def _count_files(self):
        for root, dirs, files in os.walk(self._build_dir()):
            for file in files:
                if file.endswith('.exe'):
                    self.counts['exes'] += 1
                elif file.endswith('.o'):
                    self.counts['objs'] += 1
                elif file.endswith('.a'):
                    self.counts['libs'] += 1
                elif file.endswith('.h'):
                    self.counts['h'] += 1

    def _execute(self, phase):
        exit_code = 0
        cmd = self.commands[phase]
        try:
            # This should locked; not sure how to do that
            self.proc = execute.capture_execution(log = self.output)
            log.output(wrap(('run:', self.build.key(), cmd), lineend = '\\'))
            if not self.commands['dry-run']:
                exit_code, proc, output = self.proc.shell(cmd,
                                                          cwd = path.host(self._build_dir()))
        except:
            traceback.print_exc()
            self.lock.acquire()
            if self.proc is not None:
                self.proc.kill()
            self.lock.release()
            exit_code = 1
        self.lock.acquire()
        self.proc = None
        self.lock.release()
        return exit_code == 0

    def _configure(self):
        return self._execute('configure')

    def _make(self):
        return self._execute('build')

    def _worker(self):
        self.lock.acquire()
        self.state = 'running'
        self.lock.release()
        self.build.start()
        warnings = self.results.get_warning_count()
        ok = False
        try:
            log_lock.acquire()
            try:
                self._notice('Start')
                self._notice('Creating: %s' % (self._build_dir()))
            except:
                raise
            finally:
                log_lock.release()
            self._make_build_dir()
            self._notice('Configuring')
            ok = self._configure()
            if not ok:
                warnings = self.results.get_warning_count() - warnings
                self.results.add_fail('configure',
                                      self.build,
                                      self.commands['configure'],
                                      warnings,
                                      self.results.get_error_messages(self.build))
            self.lock.acquire()
            if self.state == 'killing':
                ok = False
            self.lock.release()
            if ok:
                self._notice('Building')
                ok = self._make()
                if not ok:
                    warnings = self.results.get_warning_count() - warnings
                    self.results.add_fail('build',
                                          self.build,
                                          self.commands['configure'],
                                          warnings,
                                          self.results.get_error_messages(self.build))
            if ok:
                warnings = self.results.get_warning_count() - warnings
                self.results.add_pass(self.build,
                                      self.commands['configure'],
                                      warnings)
        except:
            ok = False
            self._notice('Build Exception')
            traceback.print_exc()
        self.build.stop()
        log_lock.acquire()
        try:
            self._count_files()
            if ok:
                self._notice('PASS')
            else:
                self._notice('FAIL')
            self._notice('Warnings:%d  exes:%d  objs:%d  libs:%d' % \
                         (warnings, self.counts['exes'],
                          self.counts['objs'], self.counts['libs']))
            log.output('  %s: Failure Report:' % (self.build.key()))
            log.output(self.results.failures_report(self.build))
            self._notice('Finished (duration:%s)' % (str(self.build.duration())))
            self._notice('Status: %s' % (self.results.status()))
        except:
            self._notice('Build Exception:')
            traceback.print_exc()
        finally:
            log_lock.release()
        self.lock.acquire()
        self.state = 'finished'
        self.lock.release()

    def get_file_counts(self):
        return self.counts

    def run(self):
        self.lock.acquire()
        try:
            if self.state != 'ready':
                raise error.general('builder already run')
            self.state = 'starting'
            self.thread = threading.Thread(target = self._worker)
            self.thread.start()
        except:
            raise
        finally:
            self.lock.release()

    def kill(self):
        self.lock.acquire()
        if self.thread is not None:
            self.state = 'killing'
            if self.proc is not None:
                try:
                    self.proc.kill()
                except:
                    pass
            self.lock.release()
            self.thread.join(5)
            self.lock.acquire()
        self.state = 'finished'
        self.lock.release()

    def current_state(self):
        self.lock.acquire()
        state = self.state
        self.lock.release()
        return state

    def log_output(self):
        self.output.log_output(['-' * 79, '] %s: Build output:' % (self.build.key())])

    def clean(self):
        if not self.commands['no-clean']:
            self._notice('Cleaning: %s' % (self._build_dir()))
            path.removeall(self._build_dir())

class configuration_:

    def __init__(self):
        self.config = configuration.configuration()
        self.archs = { }
        self.builds_ = { }
        self.profiles = { }

    def __str__(self):
        s = self.name + os.linesep
        s += 'Archs:' + os.linesep + \
             pprint.pformat(self.archs, indent = 1, width = 80) + os.linesep
        s += 'Builds:' + os.linesep + \
             pprint.pformat(self.builds_, indent = 1, width = 80) + os.linesep
        s += 'Profiles:' + os.linesep + \
             pprint.pformat(self.profiles, indent = 1, width = 80) + os.linesep
        return s

    def _build_options(self, build, nesting = 0):
        if ':' in build:
            section, name = build.split(':', 1)
            opts = [self.config.get_item(section, name)]
            return opts
        builds = self.builds_['builds']
        if build not in builds:
            raise error.general('build %s not found' % (build))
        if nesting > 20:
            raise error.general('nesting build %s' % (build))
        options = []
        for option in self.builds_['builds'][build]:
            if ':' in option:
                section, name = option.split(':', 1)
                opts = [self.config.get_item(section, name)]
            else:
                opts = self._build_options(option, nesting + 1)
            for opt in opts:
                if opt not in options:
                    options += [opt]
        return options

    def load(self, name, build):
        self.config.load(name)
        archs = []
        self.profiles['profiles'] = \
            self.config.comma_list('profiles', 'profiles', err = False)
        if len(self.profiles['profiles']) == 0:
            self.profiles['profiles'] = ['tier-%d' % (t) for t in range(1,4)]
        for p in self.profiles['profiles']:
            profile = {}
            profile['name'] = p
            profile['archs'] = self.config.comma_list(profile['name'], 'archs', err = False)
            archs += profile['archs']
            for arch in profile['archs']:
                bsps = 'bsps_%s' % (arch)
                profile[bsps] = self.config.comma_list(profile['name'], bsps)
            self.profiles[profile['name']] = profile
        invalid_chars = re.compile(r'[^a-zA-Z0-9_-]')
        for a in set(archs):
            if len(invalid_chars.findall(a)) != 0:
                raise error.general('invalid character(s) in arch name: %s' % (a))
            arch = {}
            arch['excludes'] = {}
            for exclude in self.config.comma_list(a, 'exclude', err = False):
                arch['excludes'][exclude] = ['all']
            for i in self.config.get_items(a, False):
                if i[0].startswith('exclude-'):
                    exclude = i[0][len('exclude-'):]
                    if exclude not in arch['excludes']:
                        arch['excludes'][exclude] = []
                    arch['excludes'][exclude] += \
                        sorted(set([b.strip() for b in i[1].split(',')]))
            arch['bsps'] = self.config.comma_list(a, 'bsps', err = False)
            for b in arch['bsps']:
                if len(invalid_chars.findall(b)) != 0:
                    raise error.general('invalid character(s) in BSP name: %s' % (b))
                arch[b] = {}
                arch[b]['bspopts'] = \
                    self.config.comma_list(a, 'bspopts_%s' % (b), err = False)
            self.archs[a] = arch
        builds = {}
        builds['default'] = self.config.get_item('builds', 'default')
        if build is None:
            build = builds['default']
        builds['config'] = { }
        for config in self.config.get_items('config'):
            builds['config'][config[0]] = config[1]
        builds['build'] = build
        builds_ = self.config.get_item_names('builds')
        builds['builds'] = {}
        for build in builds_:
            build_builds = self.config.comma_list('builds', build)
            has_config = False
            has_build = False
            for b in build_builds:
                if ':' in b:
                    if has_build:
                        raise error.general('config and build in build: %s' % (build))
                    has_config = True
                else:
                    if has_config:
                        raise error.general('config and build in build: %s' % (build))
                    has_build = True
            builds['builds'][build] = build_builds
        self.builds_ = builds

    def build(self):
        return self.builds_['build']

    def builds(self):
        if self.builds_['build'] in self.builds_['builds']:
            build = copy.copy(self.builds_['builds'][self.builds_['build']])
            if ':' in build[0]:
                return [self.builds_['build']]
            return build
        return None

    def build_options(self, build):
        return ' '.join(self._build_options(build))

    def excludes(self, arch):
        excludes = self.archs[arch]['excludes'].keys()
        for exclude in self.archs[arch]['excludes']:
            if 'all' not in self.archs[arch]['excludes'][exclude]:
                excludes.remove(exclude)
        return sorted(excludes)

    def archs(self):
        return sorted(self.archs.keys())

    def arch_present(self, arch):
        return arch in self.archs

    def arch_bsps(self, arch):
        return sorted(self.archs[arch]['bsps'])

    def bsp_present(self, arch, bsp):
        return bsp in self.archs[arch]['bsps']

    def bsp_excludes(self, arch, bsp):
        excludes = self.archs[arch]['excludes'].keys()
        for exclude in self.archs[arch]['excludes']:
            if 'all' not in self.archs[arch]['excludes'][exclude] and \
               bsp not in self.archs[arch]['excludes'][exclude]:
                excludes.remove(exclude)
        return sorted(excludes)

    def bspopts(self, arch, bsp):
        if arch not in self.archs:
            raise error.general('invalid architecture: %s' % (arch))
        if bsp not in self.archs[arch]:
            raise error.general('invalid BSP: %s' % (bsp))
        return self.archs[arch][bsp]['bspopts']

    def profile_present(self, profile):
        return profile in self.profiles

    def profile_archs(self, profile):
        if profile not in self.profiles:
            raise error.general('invalid profile: %s' % (profile))
        return self.profiles[profile]['archs']

    def profile_arch_bsps(self, profile, arch):
        if profile not in self.profiles:
            raise error.general('invalid profile: %s' % (profile))
        if 'bsps_%s' % (arch) not in self.profiles[profile]:
            raise error.general('invalid profile arch: %s' % (arch))
        return ['%s/%s' % (arch, bsp) for bsp in self.profiles[profile]['bsps_%s' % (arch)]]

    def report(self, profiles = True, builds = True, architectures = True):
        width = 70
        cols_1 = [width]
        cols_2 = [10, width - 10]
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s1 = ' File(s)'
        for f in self.config.files():
            colon = ':'
            for l in textwrap.wrap(f, width = cols_2[1] - 3):
                s += textbox.row(cols_2, [s1, ' ' + l], marker = colon, indent = 1)
                colon = ' '
                s1 = ' ' * len(s1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        s += os.linesep
        if profiles:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            profiles = sorted(self.profiles['profiles'])
            archs = []
            bsps = []
            for profile in profiles:
                archs += self.profiles[profile]['archs']
                for arch in sorted(self.profiles[profile]['archs']):
                    bsps += self.profiles[profile]['bsps_%s' % (arch)]
            archs = len(set(archs))
            bsps = len(set(bsps))
            s += textbox.row(cols_1,
                             [' Profiles : %d (archs:%d, bsps:%d)' % \
                              (len(profiles), archs, bsps)],
                             indent = 1)
            for profile in profiles:
                textbox.row(cols_2,
                            [profile, self.profiles[profile]['name']],
                            indent = 1)
            s += textbox.line(cols_1, marker = '+', indent = 1)
            for profile in profiles:
                s += textbox.row(cols_1, [' %s' % (profile)], indent = 1)
                profile = self.profiles[profile]
                archs = sorted(profile['archs'])
                for arch in archs:
                    arch_bsps = ', '.join(profile['bsps_%s' % (arch)])
                    if len(arch_bsps) > 0:
                        s += textbox.line(cols_2, marker = '+', indent = 1)
                        s1 = ' ' + arch
                        for l in textwrap.wrap(arch_bsps,
                                               width = cols_2[1] - 3):
                            s += textbox.row(cols_2, [s1, ' ' + l], indent = 1)
                            s1 = ' ' * len(s1)
                s += textbox.line(cols_2, marker = '+', indent = 1)
            s += os.linesep
        if builds:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            s += textbox.row(cols_1,
                             [' Builds:  %s (default)' % (self.builds_['default'])],
                             indent = 1)
            builds = self.builds_['builds']
            bsize = 0
            for build in builds:
                if len(build) > bsize:
                    bsize = len(build)
            cols_b = [bsize + 2, width - bsize - 2]
            s += textbox.line(cols_b, marker = '+', indent = 1)
            for build in builds:
                s1 = ' ' + build
                for l in textwrap.wrap(', '.join(builds[build]),
                                       width = cols_b[1] - 3):
                    s += textbox.row(cols_b, [s1, ' ' + l], indent = 1)
                    s1 = ' ' * len(s1)
                s += textbox.line(cols_b, marker = '+', indent = 1)
            configs = self.builds_['config']
            s += textbox.row(cols_1,
                             [' Configure Options: %d' % (len(configs))],
                             indent = 1)
            csize = 0
            for config in configs:
                if len(config) > csize:
                    csize = len(config)
            cols_c = [csize + 3, width - csize - 3]
            s += textbox.line(cols_c, marker = '+', indent = 1)
            for config in configs:
                s1 = ' ' + config
                for l in textwrap.wrap(configs[config], width = cols_c[1] - 3):
                    s += textbox.row(cols_c, [s1, ' ' + l], indent = 1)
                    s1 = ' ' * len(s1)
                s += textbox.line(cols_c, marker = '+', indent = 1)
            s += os.linesep
        if architectures:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            archs = sorted(self.archs.keys())
            bsps = 0
            asize = 0
            for arch in archs:
                if len(arch) > asize:
                    asize = len(arch)
                bsps += len(self.archs[arch]['bsps'])
            s += textbox.row(cols_1,
                             [' Architectures : %d (bsps: %d)' % (len(archs), bsps)],
                             indent = 1)
            cols_a = [asize + 2, width - asize - 2]
            s += textbox.line(cols_a, marker = '+', indent = 1)
            for arch in archs:
                s += textbox.row(cols_a,
                                 [' ' + arch, ' %d' % (len(self.archs[arch]['bsps']))],
                                 indent = 1)
            s += textbox.line(cols_a, marker = '+', indent = 1)
            for archn in archs:
                arch = self.archs[archn]
                if len(arch['bsps']) > 0:
                    bsize = 0
                    for bsp in arch['bsps']:
                        if len(bsp) > bsize:
                            bsize = len(bsp)
                    cols_b = [bsize + 3, width - bsize - 3]
                    s += textbox.row(cols_1, [' ' + archn + ':'], indent = 1)
                    s += textbox.line(cols_b, marker = '+', indent = 1)
                    for bsp in arch['bsps']:
                        s1 = ' ' + bsp
                        bspopts = ' '.join(arch[bsp]['bspopts'])
                        if len(bspopts):
                            for l in textwrap.wrap('bopt: ' + bspopts,
                                                   width = cols_b[1] - 3):
                                s += textbox.row(cols_b, [s1, ' ' + l], indent = 1)
                                s1 = ' ' * len(s1)
                        excludes = []
                        for exclude in arch['excludes']:
                            if 'all' in arch['excludes'][exclude] or \
                               bsp in arch['excludes'][exclude]:
                                excludes += [exclude]
                        excludes = ', '.join(excludes)
                        if len(excludes):
                            for l in textwrap.wrap('ex: ' + excludes,
                                                   width = cols_b[1] - 3):
                                s += textbox.row(cols_b, [s1, ' ' + l], indent = 1)
                                s1 = ' ' * len(s1)
                        if len(bspopts) == 0 and len(excludes) == 0:
                            s += textbox.row(cols_b, [s1, ' '], indent = 1)
                    s += textbox.line(cols_b, marker = '+', indent = 1)
            s += os.linesep
        return s

class build_jobs:

    def __init__(self, config, arch, bsp):
        self.arch = arch
        self.bsp = bsp
        self.builds = config.builds()
        if self.builds is None:
            raise error.general('build not found: %s' % (config.build()))
        excludes = list(set(config.excludes(self.arch) +
                            config.bsp_excludes(self.arch, self.bsp)))
        #
        # The build can be in the buld string delimited by '-'.
        #
        remove = []
        for e in excludes:
            remove += [b for b in self.builds if e in b]
        self.builds = [b for b in self.builds if b not in remove]
        self.build_set = { }
        for build in self.builds:
            self.build_set[build] = config.build_options(build)

    def jobs(self):
        return [arch_bsp_build(self.arch, self.bsp, b, self.build_set[b]) \
                for b in sorted(self.build_set.keys())]

class builder:

    def __init__(self, config, version, prefix, tools, rtems, build_dir, options):
        self.config = config
        self.build_dir = build_dir
        self.rtems_version = version
        self.prefix = prefix
        self.tools = tools
        self.rtems = rtems
        self.options = options
        self.counts = { 'h'        : 0,
                        'exes'     : 0,
                        'objs'     : 0,
                        'libs'     : 0 }
        self.results = results(rtems,
                               { 'groups'  : ['Shared', 'BSP', 'Network', 'Tests',
                                              'LibCPU', 'CPU Kit'],
                                 'exclude' : '.*Makefile.*',
                                 'CPU Kit' : '.*cpukit/.*',
                                 'Network' : '.*libnetworking/.*|.*librpc/.*',
                                 'Tests'   : '.*testsuites/.*',
                                 'BSP'     : '.*libbsp/.*',
                                 'LibCPU'  : '.*libcpu/.*',
                                 'Shared'  : '.*shared/.*' })
        if not path.exists(path.join(rtems, 'configure')) or \
           not path.exists(path.join(rtems, 'Makefile.in')) or \
           not path.exists(path.join(rtems, 'cpukit')):
            raise error.general('RTEMS source path does not look like RTEMS')

    def _bsps(self, arch):
        return self.config.arch_bsps(arch)

    def _create_build_jobs(self, jobs, build_job_count):
        max_job_size = len('%d' % len(jobs))
        build_jobs = []
        job_index = 1
        for job in jobs:
            tag = '%*d/%d' % (max_job_size, job_index, len(jobs))
            build_jobs += [arch_bsp_builder(self.results,
                                            job,
                                            self._commands(job, build_job_count),
                                            self.build_dir,
                                            tag)]
            job_index += 1
        set_max_build_label(build_jobs)
        return build_jobs

    def _commands(self, build, build_jobs):
        commands = { 'dry-run'  : self.options['dry-run'],
                     'no-clean' : self.options['no-clean'],
                     'configure': None,
                     'build'    : None }
        cmds = build.build_config.split()
        cmds += self.config.bspopts(build.arch, build.bsp)
        cmd = [path.join(self.rtems, 'configure')]
        for c in cmds:
            c = c.replace('@PREFIX@', self.prefix)
            c = c.replace('@RTEMS_VERSION@', self.rtems_version)
            c = c.replace('@ARCH@', build.arch)
            c = c.replace('@BSP@', build.bsp)
            cmd += [c]
        commands['configure'] = ' '.join(cmd)
        cmd = 'make -j %s' % (build_jobs)
        commands['build'] = cmd
        return commands

    def _update_file_counts(self, counts):
        for f in self.counts:
            if f in counts:
                self.counts[f] += counts[f]
        return counts

    def _warnings_report(self):
        if self.options['warnings-report'] is not None:
            with open(self.options['warnings-report'], 'w') as f:
                f.write(title() + os.linesep)
                f.write(os.linesep)
                f.write('Date: %s%s' % (datetime.datetime.now().strftime('%c'),
                                        os.linesep))
                f.write(os.linesep)
                f.write(command_line() + os.linesep)
                f.write(self.results.warnings_errors.warnings_report())

    def _failures_report(self):
        if self.options['failures-report'] is not None:
            with open(self.options['failures-report'], 'w') as f:
                f.write(title() + os.linesep)
                f.write(os.linesep)
                f.write('Date: %s%s' % (datetime.datetime.now().strftime('%c'),
                                        os.linesep))
                f.write(os.linesep)
                f.write(command_line() + os.linesep)
                f.write(self.results.failures_report())

    def _finished(self):
        log.notice('Total: Warnings:%d  exes:%d  objs:%d  libs:%d' % \
                   (self.results.get_warning_count(), self.counts['exes'],
                    self.counts['objs'], self.counts['libs']))
        log.output()
        log.output('Warnings:')
        log.output(self.results.warnings_report())
        log.output()
        log.notice('Failures:')
        log.notice(self.results.failures_report())
        self._warnings_report()
        self._failures_report()

    def run_jobs(self, jobs):
        if path.exists(self.build_dir):
            log.notice('Cleaning: %s' % (self.build_dir))
            path.removeall(self.build_dir)
        self.start = datetime.datetime.now()
        self.end = datetime.datetime.now()
        self.duration = self.end - self.start
        self.average = self.duration
        env_path = os.environ['PATH']
        os.environ['PATH'] = path.host(path.join(self.tools, 'bin')) + \
                             os.pathsep + os.environ['PATH']
        job_count, build_job_count = jobs_option_parse(self.options['jobs'])
        builds = self._create_build_jobs(jobs, build_job_count)
        active_jobs = []
        self.jobs_completed = 0
        try:
            while len(builds) > 0 or len(active_jobs) > 0:
                new_jobs = job_count - len(active_jobs)
                if new_jobs > 0:
                    active_jobs += builds[:new_jobs]
                    builds = builds[new_jobs:]
                finished_jobs = []
                for job in active_jobs:
                    state = job.current_state()
                    if state == 'ready':
                        job.run()
                    elif state != 'running':
                        finished_jobs += [job]
                for job in finished_jobs:
                    self._update_file_counts(job.get_file_counts())
                    job.log_output()
                    job.clean()
                    active_jobs.remove(job)
                    self.jobs_completed += 1
                time.sleep(0.250)
        except:
            for job in active_jobs:
                try:
                    job.kill()
                except:
                    pass
            raise
        self.end = datetime.datetime.now()
        os.environ['PATH'] = env_path
        self.duration = self.end - self.start
        if self.jobs_completed == 0:
            self.jobs_completed = 1
        self._finished()
        self.average = self.duration / self.jobs_completed
        log.notice('Average BSP Build Time: %s' % (str(self.average)))
        log.notice('Total Time %s' % (str(self.duration)))

    def arch_bsp_jobs(self, arch, bsps):
        jobs = []
        for bsp in bsps:
            jobs += build_jobs(self.config, arch, bsp).jobs()
        return jobs

    def bsp_jobs(self, bsps):
        jobs = []
        for bsp in bsps:
            if bsp.count('/') != 1:
                raise error.general('invalid bsp format (use: arch/bsp): %s' % (bsp))
            arch, bsp = bsp.split('/')
            jobs += build_jobs(self.config, arch, bsp).jobs()
        return jobs

    def arch_jobs(self, archs):
        jobs = []
        for arch in archs:
            if not self.config.arch_present(arch):
                raise error.general('Architecture not found: %s' % (arch))
            jobs += self.arch_bsp_jobs(arch, self._bsps(arch))
        return jobs

    def profile_jobs(self, profiles):
        jobs = []
        for profile in profiles:
            if not self.config.profile_present(profile):
                raise error.general('Profile not found: %s' % (profile))
            for arch in self.config.profile_archs(profile):
                jobs += self.bsp_jobs(self.config.profile_arch_bsps(profile, arch))
        return jobs

    def build_bsps(self, bsps):
        log.notice('BSPS(s): %s' % (', '.join(bsps)))
        self.run_jobs(self.bsp_jobs(bsps))

    def build_archs(self, archs):
        log.notice('Architecture(s): %s' % (', '.join(archs)))
        self.run_jobs(self.arch_jobs(archs))

    def build_profiles(self, profiles):
        log.notice('Profile(s): %s' % (', '.join(profiles)))
        self.run_jobs(self.profile_jobs(profiles))

def run_args(args):
    b = None
    ec = 0
    try:
        #
        # On Windows MSYS2 prepends a path to itself to the environment
        # path. This means the RTEMS specific automake is not found and which
        # breaks the bootstrap. We need to remove the prepended path. Also
        # remove any ACLOCAL paths from the environment.
        #
        if os.name == 'nt':
            cspath = os.environ['PATH'].split(os.pathsep)
            if 'msys' in cspath[0] and cspath[0].endswith('bin'):
                os.environ['PATH'] = os.pathsep.join(cspath[1:])

        start = datetime.datetime.now()
        top = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
        prefix = '/opt/rtems/%s' % (rtems_version())
        tools = prefix
        build_dir = 'bsp-builds'
        logf = 'bsp-build-%s.txt' % \
               (datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
        config_file = path.join(top, 'share', 'rtems', 'tester',
                                'rtems', 'rtems-bsps.ini')
        if not path.exists(config_file):
            config_file = path.join(top, 'tester', 'rtems', 'rtems-bsps.ini')

        argsp = argparse.ArgumentParser()
        argsp.add_argument('--prefix', help = 'Prefix to build the BSP.',
                           type = str)
        argsp.add_argument('--rtems-tools', help = 'The RTEMS tools directory.',
                           type = str)
        argsp.add_argument('--rtems', help = 'The RTEMS source tree.',
                           type = str)
        argsp.add_argument('--build-path', help = 'Path to build in.',
                           type = str)
        argsp.add_argument('--log', help = 'Log file.', type = str)
        argsp.add_argument('--config-report', help = 'Report the configuration.',
                           type = str, default = None,
                           choices = ['all', 'profiles', 'builds', 'archs'])
        argsp.add_argument('--warnings-report', help = 'Report the warnings to a file.',
                           type = str, default = None)
        argsp.add_argument('--failures-report', help = 'Report the failures to a file.',
                           type = str, default = None)
        argsp.add_argument('--stop-on-error', help = 'Stop on an error.',
                           action = 'store_true')
        argsp.add_argument('--no-clean', help = 'Do not clean the build output.',
                           action = 'store_true')
        argsp.add_argument('--profiles', help = 'Build the listed profiles (profile,profile,..).',
                           type = str, default = 'tier-1')
        argsp.add_argument('--arch', help = 'Build the architectures (arch,arch,..).',
                           type = str)
        argsp.add_argument('--bsp', help = 'Build the BSPs (arch/bsp,arch/bsp,..).',
                           type = str)
        argsp.add_argument('--build', help = 'Build name to build (see --config-report).',
                           type = str, default='all')
        argsp.add_argument('--jobs', help = 'Number of jobs to run.',
                           type = str, default = '1/%d' % (host.cpus()))
        argsp.add_argument('--dry-run', help = 'Do not run the actual builds.',
                           action = 'store_true')
        mailer.add_arguments(argsp)

        opts = argsp.parse_args(args[1:])
        mail = None
        if opts.mail:
            mail = mailer.mail(opts)
            # Request these now to generate any errors.
            from_addr = mail.from_address()
            smtp_host = mail.smtp_host()
            if 'mail_to' in opts and opts.mail_to is not None:
                to_addr = opts.mail_to
            else:
                to_addr = 'build@rtems.org'
        if opts.log is not None:
            logf = opts.log
        log.default = log.log([logf])
        log.notice(title())
        log.output(command_line())
        if mail:
            log.notice('Mail: from:%s to:%s smtp:%s' % (from_addr,
                                                        to_addr,
                                                        smtp_host))

        config = configuration_()
        config.load(config_file, opts.build)

        if opts.config_report:
            log.notice('Configuration Report: %s' % (opts.config_report))
            c_profiles = False
            c_builds = False
            c_archs = False
            if opts.config_report == 'all':
                c_profiles = True
                c_builds = True
                c_archs = True
            elif opts.config_report == 'profiles':
                c_profiles = True
            elif opts.config_report == 'builds':
                c_builds = True
            elif opts.config_report == 'archs':
                c_archs = True
            log.notice(config.report(c_profiles, c_builds, c_archs))
            sys.exit(0)

        if opts.rtems is None:
            raise error.general('No RTEMS source provided on the command line')
        if opts.prefix is not None:
            prefix = path.shell(opts.prefix)
        if opts.rtems_tools is not None:
            tools = path.shell(opts.rtems_tools)
        if opts.build_path is not None:
            build_dir = path.shell(opts.build_path)

        options = { 'stop-on-error'   : opts.stop_on_error,
                    'no-clean'        : opts.no_clean,
                    'dry-run'         : opts.dry_run,
                    'jobs'            : opts.jobs,
                    'warnings-report' : opts.warnings_report,
                    'failures-report' : opts.failures_report }

        b = builder(config, rtems_version(), prefix, tools,
                    path.shell(opts.rtems), build_dir, options)

        profiles = comma_split(opts.profiles)
        archs = comma_split(opts.arch)
        bsps = comma_split(opts.bsp)

        #
        # The default is build a profile.
        #
        if bsps is not None:
            if archs is not None:
                raise error.general('--arch supplied with --bsp;' \
                                    ' use --bsp=arch/bsp,arch/bsp,..')
            what = 'BSPs: %s' % (' '.join(bsps))
            b.build_bsps(bsps)
        elif archs is not None:
            what = 'Archs: %s' % (' '.join(archs))
            b.build_archs(archs)
        else:
            what = 'Profile(s): %s' % (' '.join(profiles))
            b.build_profiles(profiles)
        end = datetime.datetime.now()

        #
        # Email the results of the build.
        #
        if mail is not None:
            subject = '[rtems-bsp-builder] %s: %s' % (str(start).split('.')[0],
                                                      what)
            t = title()
            body = t + os.linesep
            body += '=' * len(t) + os.linesep
            body += os.linesep
            body += 'Host: %s' % (os.uname()[3]) + os.linesep
            body += os.linesep
            body += command_line()
            body += os.linesep
            body += 'Total Time            : %s for %d completed job(s)' % \
                    (str(b.duration), b.jobs_completed)
            body += os.linesep
            body += 'Average BSP Build Time: %s' % (str(b.average))
            body += os.linesep + os.linesep
            body += 'Builds' + os.linesep
            body += '======' + os.linesep
            body += os.linesep.join([' ' + cb for cb in config.builds()])
            body += os.linesep + os.linesep
            body += 'Failures Report' + os.linesep
            body += '===============' + os.linesep
            body += b.results.failures_report()
            body += os.linesep
            body += 'Warnings Report' + os.linesep
            body += '===============' + os.linesep
            body += b.results.warnings_report(summary = True)
            mail.send(to_addr, subject, body)

    except error.general as gerr:
        print(gerr)
        print('BSP Build FAILED', file = sys.stderr)
        ec = 1
    except error.internal as ierr:
        print(ierr)
        print('BSP Build FAILED', file = sys.stderr)
        ec = 1
    except error.exit as eerr:
        pass
    except KeyboardInterrupt:
        log.notice('abort: user terminated')
        ec = 1
    if b is not None:
        b.results.report()
    sys.exit(ec)

def run():
    run_args(sys.argv)

if __name__ == "__main__":
    run()
