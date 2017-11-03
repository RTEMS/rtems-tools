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

from __future__ import print_function

import copy
import datetime
import fnmatch
import os
import re
import sys
import threading
import time

from rtemstoolkit import configuration
from rtemstoolkit import error
from rtemstoolkit import host
from rtemstoolkit import log
from rtemstoolkit import path
from rtemstoolkit import mailer
from rtemstoolkit import reraise
from rtemstoolkit import stacktraces
from rtemstoolkit import version

from . import bsps
from . import config
from . import console
from . import options
from . import report

class log_capture(object):
    def __init__(self):
        self.log = []
        log.capture = self.capture

    def __str__(self):
        return os.linesep.join(self.log)

    def capture(self, text):
        self.log += [l for l in text.replace(chr(13), '').splitlines()]

    def get(self):
        s = []
        status = []
        status_regx = re.compile('^\[\s*\d+/\s*\d+\] p:.+')
        for l in self.log:
            if status_regx.match(l):
                status += [l]
            else:
                if len(status) == 1:
                    s += [status[0]]
                    status = []
                elif len(status) > 1:
                    s += [status[0]]
                    if len(status) > 2:
                        s += [' <<skipping passes>>']
                    s += [status[-1]]
                    status = []
                s += [l]
        return s

class test(object):
    def __init__(self, index, total, report, executable, rtems_tools, bsp, bsp_config, opts):
        self.index = index
        self.total = total
        self.report = report
        self.bsp = bsp
        self.bsp_config = bsp_config
        self.opts = copy.copy(opts)
        self.opts.defaults['test_index'] = str(index)
        self.opts.defaults['test_total'] = str(total)
        self.opts.defaults['bsp'] = bsp
        self.opts.defaults['bsp_arch'] = '%{arch}'
        if not path.isfile(executable):
            raise error.general('cannot find executable: %s' % (executable))
        self.opts.defaults['test_executable'] = executable
        if rtems_tools:
            rtems_tools_bin = path.join(self.opts.defaults.expand(rtems_tools), 'bin')
            if not path.isdir(rtems_tools_bin):
                raise error.general('cannot find RTEMS tools path: %s' % (rtems_tools_bin))
            self.opts.defaults['rtems_tools'] = rtems_tools_bin
        self.config = config.file(index, total, self.report, self.bsp_config, self.opts)

    def run(self):
        if self.config:
            self.config.run()

    def kill(self):
        if self.config:
            self.config.kill()

class test_run(object):
    def __init__(self, index, total, report, executable, rtems_tools, bsp, bsp_config, opts):
        self.test = None
        self.result = None
        self.start_time = None
        self.end_time = None
        self.index = copy.copy(index)
        self.total = total
        self.report = report
        self.executable = copy.copy(executable)
        self.rtems_tools = rtems_tools
        self.bsp = bsp
        self.bsp_config = bsp_config
        self.opts = opts

    def runner(self):
        self.start_time = datetime.datetime.now()
        try:
            self.test = test(self.index, self.total, self.report,
                             self.executable, self.rtems_tools,
                             self.bsp, self.bsp_config,
                             self.opts)
            self.test.run()
        except KeyboardInterrupt:
            pass
        except:
            self.result = sys.exc_info()
        self.end_time = datetime.datetime.now()

    def run(self):
        self.thread = threading.Thread(target = self.runner,
                                       name = 'test[%s]' % path.basename(self.executable))
        self.thread.start()

    def is_alive(self):
        return self.thread and self.thread.is_alive()

    def reraise(self):
        if self.result is not None:
            reraise.reraise(*self.result)

    def kill(self):
        if self.test:
            self.test.kill()

def find_executables(paths, glob):
    executables = []
    for p in paths:
        if path.isfile(p):
            executables += [p]
        elif path.isdir(p):
            for root, dirs, files in os.walk(p, followlinks = True):
                for f in files:
                    if fnmatch.fnmatch(f.lower(), glob):
                        executables += [path.join(root, f)]
    return sorted(executables)

def report_finished(reports, report_mode, reporting, finished, job_trace):
    processing = True
    while processing:
        processing = False
        reported = []
        for tst in finished:
            if tst not in reported and \
                    (reporting < 0 or tst.index == reporting):
                if job_trace:
                    log.notice('}} %*d: %s: %s (%d)' % (len(str(tst.total)), tst.index,
                                                        path.basename(tst.executable),
                                                        'reporting',
                                                        reporting))
                processing = True
                reports.log(tst.executable, report_mode)
                reported += [tst]
                reporting += 1
        finished[:] = [t for t in finished if t not in reported]
        if len(reported):
            del reported[:]
            if job_trace:
                print('}} threading:', threading.active_count())
                for t in threading.enumerate():
                    print('}} ', t.name)
    return reporting

def _job_trace(tst, msg, total, exe, active, reporting):
    s = ''
    for a in active:
        s += ' %d:%s' % (a.index, path.basename(a.executable))
    log.notice('}} %*d: %s: %s (%d %d %d%s)' % (len(str(tst.total)), tst.index,
                                                path.basename(tst.executable),
                                                msg,
                                                reporting, total, exe, s))

def list_bsps(opts):
    path_ = opts.defaults.expand('%%{_configdir}/bsps/*.mc')
    bsps = path.collect_files(path_)
    log.notice(' BSP List:')
    for bsp in bsps:
        log.notice('  %s' % (path.basename(bsp[:-3])))
    raise error.exit()

def killall(tests):
    for test in tests:
        test.kill()

def run(command_path = None):
    import sys
    tests = []
    stdtty = console.save()
    opts = None
    default_exefilter = '*.exe'
    try:
        optargs = { '--rtems-tools': 'The path to the RTEMS tools',
                    '--rtems-bsp':   'The RTEMS BSP to run the test on',
                    '--user-config': 'Path to your local user configuration INI file',
                    '--report-mode': 'Reporting modes, failures (default),all,none',
                    '--list-bsps':   'List the supported BSPs',
                    '--debug-trace': 'Debug trace based on specific flags',
                    '--filter':      'Glob that executables must match to run (default: ' +
                              default_exefilter + ')',
                    '--stacktrace':  'Dump a stack trace on a user termination (^C)' }
        mailer.append_options(optargs)
        opts = options.load(sys.argv,
                            optargs = optargs,
                            command_path = command_path)
        mail = None
        output = None
        if opts.find_arg('--mail'):
            mail = mailer.mail(opts)
            # Request these now to generate any errors.
            from_addr = mail.from_address()
            smtp_host = mail.smtp_host()
            to_addr = opts.find_arg('--mail-to')
            if to_addr:
                to_addr = to_addr[1]
            else:
                to_addr = 'build@rtems.org'
            output = log_capture()
        log.notice('RTEMS Testing - Tester, %s' % (version.str()))
        if opts.find_arg('--list-bsps'):
            bsps.list(opts)
        exe_filter = opts.find_arg('--filter')
        if exe_filter:
            exe_filter = exe_filter[1]
        else:
            exe_filter = default_exefilter
        opts.log_info()
        log.output('Host: ' + host.label(mode = 'all'))
        debug_trace = opts.find_arg('--debug-trace')
        if debug_trace:
            if len(debug_trace) != 1:
                debug_trace = debug_trace[1]
            else:
                raise error.general('no debug flags, can be: console,gdb,output')
        else:
            debug_trace = ''
        opts.defaults['exe_trace'] = debug_trace
        job_trace = 'jobs' in debug_trace.split(',')
        rtems_tools = opts.find_arg('--rtems-tools')
        if rtems_tools:
            if len(rtems_tools) != 2:
                raise error.general('invalid RTEMS tools option')
            rtems_tools = rtems_tools[1]
        else:
            rtems_tools = '%{_prefix}'
        bsp = opts.find_arg('--rtems-bsp')
        if bsp is None or len(bsp) != 2:
            raise error.general('RTEMS BSP not provided or an invalid option')
        bsp = config.load(bsp[1], opts)
        bsp_config = opts.defaults.expand(opts.defaults['tester'])
        report_mode = opts.find_arg('--report-mode')
        if report_mode:
            if report_mode[1] != 'failures' and \
                    report_mode[1] != 'all' and \
                    report_mode[1] != 'none':
                raise error.general('invalid report mode')
            report_mode = report_mode[1]
        else:
            report_mode = 'failures'
        executables = find_executables(opts.params(), exe_filter)
        if len(executables) == 0:
            raise error.general('no executables supplied')
        start_time = datetime.datetime.now()
        total = len(executables)
        reports = report.report(total)
        reporting = 1
        jobs = int(opts.jobs(opts.defaults['_ncpus']))
        exe = 0
        finished = []
        if jobs > len(executables):
            jobs = len(executables)
        while exe < total or len(tests) > 0:
            if exe < total and len(tests) < jobs:
                tst = test_run(exe + 1, total, reports,
                               executables[exe],
                               rtems_tools, bsp, bsp_config,
                               opts)
                exe += 1
                tests += [tst]
                if job_trace:
                    _job_trace(tst, 'create',
                               total, exe, tests, reporting)
                tst.run()
            else:
                dead = [t for t in tests if not t.is_alive()]
                tests[:] = [t for t in tests if t not in dead]
                for tst in dead:
                    if job_trace:
                        _job_trace(tst, 'dead',
                                   total, exe, tests, reporting)
                    finished += [tst]
                    tst.reraise()
                del dead
                if len(tests) >= jobs or exe >= total:
                    time.sleep(0.250)
                if len(finished):
                    reporting = report_finished(reports,
                                                report_mode,
                                                reporting,
                                                finished,
                                                job_trace)
        finished_time = datetime.datetime.now()
        reporting = report_finished(reports, report_mode,
                                    reporting, finished, job_trace)
        if reporting < total:
            log.warning('finished jobs does match: %d' % (reporting))
            report_finished(reports, report_mode, -1, finished, job_trace)
        reports.summary()
        end_time = datetime.datetime.now()
        average_time = 'Average test time: %s' % (str((end_time - start_time) / total))
        total_time = 'Testing time     : %s' % (str(end_time - start_time))
        log.notice(average_time)
        log.notice(total_time)
        if mail is not None and output is not None:
            subject = '[rtems-test] %s: %s' % (str(start_time).split('.')[0], bsp)
            body = [total_time, average_time,
                    '', 'Host', '====', host.label(mode = 'all'),
                    '', 'Summary', '=======', '',
                    reports.score_card(), '',
                    reports.failures(),
                    '', 'Log', '===', ''] + output.get()
            mail.send(to_addr, subject, os.linesep.join(body))

    except error.general as gerr:
        print(gerr)
        sys.exit(1)
    except error.internal as ierr:
        print(ierr)
        sys.exit(1)
    except error.exit:
        sys.exit(2)
    except KeyboardInterrupt:
        if opts is not None and opts.find_arg('--stacktrace'):
            print('}} dumping:', threading.active_count())
            for t in threading.enumerate():
                print('}} ', t.name)
            print(stacktraces.trace())
        log.notice('abort: user terminated')
        killall(tests)
        sys.exit(1)
    finally:
        console.restore(stdtty)
    sys.exit(0)

if __name__ == "__main__":
    run()
