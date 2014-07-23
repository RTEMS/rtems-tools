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

import copy
import datetime
import os
import sys
import threading
import time

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import path
from rtemstoolkit import stacktraces

import bsps
import config
import console
import options
import report
import version
import fnmatch

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
        self.opts.defaults['bsp_arch'] = '%%{%s_arch}' % (bsp)
        self.opts.defaults['bsp_opts'] = '%%{%s_opts}' % (bsp)
        if not path.isfile(executable):
            raise error.general('cannot find executable: %s' % (executable))
        self.opts.defaults['test_executable'] = executable
        if rtems_tools:
            rtems_tools_bin = path.join(rtems_tools, 'bin')
            if not path.isdir(rtems_tools_bin):
                raise error.general('cannot find RTEMS tools path: %s' % (rtems_tools_bin))
            self.opts.defaults['rtems_tools'] = rtems_tools_bin
        self.config = config.file(self.report, self.bsp_config, self.opts)

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
            raise self.result[0], self.result[1], self.result[2]

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
                print '}} threading:', threading.active_count()
                for t in threading.enumerate():
                    print '}} ', t.name
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
                    '--report-mode': 'Reporting modes, failures (default),all,none',
                    '--list-bsps':   'List the supported BSPs',
                    '--debug-trace': 'Debug trace based on specific flags',
                    '--filter':      'Glob that executables must match to run (default: ' +
                              default_exefilter + ')',
                    '--stacktrace':  'Dump a stack trace on a user termination (^C)' }
        opts = options.load(sys.argv,
                            optargs = optargs,
                            command_path = command_path)
        log.notice('RTEMS Testing - Tester, v%s' % (version.str()))
        if opts.find_arg('--list-bsps'):
            bsps.list(opts)
        exe_filter = opts.find_arg('--filter')
        if exe_filter:
            exe_filter = exe_filter[1]
        else:
            exe_filter = default_exefilter
        opts.log_info()
        debug_trace = opts.find_arg('--debug-trace')
        if debug_trace:
            debug_trace = debug_trace[1]
        else:
            debug_trace = ''
        opts.defaults['debug_trace'] = debug_trace
        job_trace = 'jobs' in debug_trace.split(',')
        rtems_tools = opts.find_arg('--rtems-tools')
        if rtems_tools:
            rtems_tools = rtems_tools[1]
        bsp = opts.find_arg('--rtems-bsp')
        if bsp is None:
            raise error.general('no RTEMS BSP provided')
        opts.defaults.load('%%{_configdir}/bsps/%s.mc' % (bsp[1]))
        bsp = opts.defaults.get('%{bsp}')
        if not bsp:
            raise error.general('BSP definition (%{bsp}) not found in the global map')
        bsp = bsp[2]
        if not opts.defaults.set_read_map(bsp):
            raise error.general('no BSP map found')
        bsp_script = opts.defaults.get(bsp)
        if not bsp_script:
            raise error.general('BSP script not found: %s' % (bsp))
        bsp_config = opts.defaults.expand(opts.defaults[bsp])
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
        invalid_tests = opts.defaults['invalid_tests']
        if invalid_tests:
            reports.set_invalid_tests([l.strip() for l in invalid_tests.splitlines()])
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
        log.notice('Average test time: %s' % (str((end_time - start_time) / total)))
        log.notice('Testing time     : %s' % (str(end_time - start_time)))
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    except error.exit, eerr:
        sys.exit(2)
    except KeyboardInterrupt:
        if opts.find_arg('--stacktrace'):
            print '}} dumping:', threading.active_count()
            for t in threading.enumerate():
                print '}} ', t.name
            print stacktraces.trace()
        log.notice('abort: user terminated')
        killall(tests)
        sys.exit(1)
    finally:
        console.restore(stdtty)
    sys.exit(0)

if __name__ == "__main__":
    run()
