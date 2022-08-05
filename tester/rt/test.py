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
from rtemstoolkit import check

import tester.rt.bsps
import tester.rt.config
import tester.rt.console
import tester.rt.options
import tester.rt.report
import tester.rt.coverage

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
        self.config = tester.rt.config.file(index, total, self.report, self.bsp_config, self.opts)

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
        name = 'test[%s]' % path.basename(self.executable)
        self.thread = threading.Thread(target = self.runner,
                                       name = name)
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
    norun = re.compile('.*\.norun.*')
    executables = [e for e in executables if not norun.match(e)]
    return sorted(executables)

def report_finished(reports, log_mode, reporting, finished, job_trace):
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
                reports.log(tst.executable, log_mode)
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

def killall(tests):
    for test in tests:
        test.kill()


def results_to_data(args, reports, start_time, end_time):
    data = {}
    data['report-version'] = 1
    data['tester-version'] = version.string()
    data['command-line'] = args
    data['host'] = host.label(mode='all')
    data['python'] = sys.version.replace('\n', '')
    data['start-time'] = start_time.isoformat()
    data['end-time'] = end_time.isoformat()
    reports_data = []

    for name, run in reports.results.items():
        run_data = {}
        result = run['result']
        run_data['result'] = result
        output = []
        for line in run['output']:
            if line.startswith('] '):
                output.append(line[2:])
            elif line.startswith('=>  exe:'):
                run_data['command-line'] = line[9:].split()
        run_data['output'] = output
        run_data['executable'] = name
        run_data['executable-sha512'] = get_hash512(name)
        run_data['start-time'] = run['start'].isoformat()
        run_data['end-time'] = run['end'].isoformat()
        run_data['bsp'] = run['bsp']
        run_data['arch'] = run['bsp_arch']
        reports_data.append(run_data)

    data['reports'] = sorted(reports_data, key=lambda x: x["executable"])
    return data


def generate_json_report(args, reports, start_time, end_time,
                         _total, json_file):
    import json

    data = results_to_data(args, reports, start_time, end_time);

    with open(json_file, 'w') as outfile:
        json.dump(data, outfile, sort_keys=True, indent=4)


def generate_junit_report(args, reports, start_time, end_time,
                         total, junit_file):

    from junit_xml import TestSuite, TestCase
    import sys
    junit_log = []

    junit_prop = {}
    junit_prop['Command Line'] = ' '.join(args)
    junit_prop['Python'] = sys.version.replace('\n', '')
    junit_prop['test_groups'] = []
    junit_prop['Host'] = host.label(mode = 'all')
    junit_prop['passed_count'] = reports.passed
    junit_prop['failed_count'] = reports.failed
    junit_prop['user-input_count'] = reports.user_input
    junit_prop['expected-fail_count'] = reports.expected_fail
    junit_prop['indeterminate_count'] = reports.indeterminate
    junit_prop['benchmark_count'] = reports.benchmark
    junit_prop['timeout_count'] = reports.timeouts
    junit_prop['test-too-long_count'] = reports.test_too_long
    junit_prop['invalid_count'] = reports.invalids
    junit_prop['wrong-version_count'] = reports.wrong_version
    junit_prop['wrong-build_count'] = reports.wrong_build
    junit_prop['wrong-tools_count'] = reports.wrong_tools
    junit_prop['wrong-header_count'] = reports.wrong_header
    junit_prop['total_count'] = reports.total
    time_delta = end_time - start_time
    junit_prop['average_test_time'] = str(time_delta / total)
    junit_prop['testing_time'] = str(time_delta)

    for name in reports.results:
        result_type = reports.results[name]['result']
        test_parts = name.split('/')
        test_category = test_parts[-2]
        test_name = test_parts[-1]

        junit_result = TestCase(test_name.split('.')[0])
        junit_result.category = test_category
        if result_type == 'failed' or result_type == 'timeout':
            junit_result.add_failure_info(None, reports.results[name]['output'], result_type)

        junit_log.append(junit_result)

    ts = TestSuite('RTEMS Test Suite', junit_log)
    ts.properties = junit_prop
    ts.hostname = host.label(mode = 'all')

    # write out junit log
    with open(junit_file, 'w') as f:
        TestSuite.to_file(f, [ts], prettyprint = True)


def generate_yaml_report(args, reports, start_time, end_time,
                         _total, yaml_file):
    """ Generates a YAML file containing information about the test run,
    including all test outputs """

    import yaml

    data = results_to_data(args, reports, start_time, end_time);

    with open(yaml_file, 'w') as outfile:
        yaml.dump(data, outfile, default_flow_style=False, allow_unicode=True)


def get_hash512(exe):
    """ returns SHA512 hash string of a given binary file passed as argument """
    import hashlib

    hash = hashlib.sha512()
    with open(exe, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            hash.update(byte_block)
    return hash.hexdigest()


report_formatters = {
        'json': generate_json_report,
        'junit': generate_junit_report,
        'yaml': generate_yaml_report
}


def check_report_formats(report_formats, report_location):
    if report_location is None:
        raise error.general('report base path missing')
    for report_format in report_formats:
        if report_format not in report_formatters:
            raise error.general('invalid RTEMS report formatter: %s'
                                % report_format)
        if report_format == 'yaml':
            try:
                import yaml
            except ImportError:
                raise error.general('Please install PyYAML module. HINT: You can '
                                    'use pip to install it!')


def run(args):
    import sys
    tests = []
    stdtty = tester.rt.console.save()
    opts = None
    default_exefilter = '*.exe'
    try:
        optargs = { '--rtems-tools':    'The path to the RTEMS tools',
                    '--rtems-bsp':      'The RTEMS BSP to run the test on',
                    '--user-config':    'Path to your local user configuration INI file',
                    '--report-path':    'Report output base path (file extension will be added)',
                    '--report-format':  'Formats in which to report test results in addition to txt: json, yaml',
                    '--log-mode':       'Reporting modes, failures (default),all,none',
                    '--list-bsps':      'List the supported BSPs',
                    '--debug-trace':    'Debug trace based on specific flags (console,gdb,output,cov)',
                    '--filter':         'Glob that executables must match to run (default: ' +
                              default_exefilter + ')',
                    '--stacktrace':     'Dump a stack trace on a user termination (^C)',
                    '--coverage':       'Perform coverage analysis of test executables.'}
        mailer.append_options(optargs)
        opts = tester.rt.options.load(args, optargs = optargs)
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
        report_location = opts.find_arg('--report-path')
        if report_location is not None:
            report_location = report_location[1]

        report_formats = opts.find_arg('--report-format')
        if report_formats is not None:
            if len(report_formats) != 2:
                raise error.general('invalid RTEMS report formats option')
            report_formats = report_formats[1].split(',')
            check_report_formats(report_formats, report_location)
        else:
            report_formats = []
        log.notice('RTEMS Testing - Tester, %s' % (version.string()))
        if opts.find_arg('--list-bsps'):
            tester.rt.bsps.list(opts)
        exe_filter = opts.find_arg('--filter')
        if exe_filter:
            exe_filter = exe_filter[1]
        else:
            exe_filter = default_exefilter
        opts.log_info()
        log.output('Host: ' + host.label(mode = 'all'))
        executables = find_executables(opts.params(), exe_filter)
        debug_trace = opts.find_arg('--debug-trace')
        if debug_trace:
            if len(debug_trace) != 1:
                debug_trace = debug_trace[1]
            else:
                raise error.general('no debug flags, can be: console,gdb,output,cov')
        else:
            debug_trace = ''
        opts.defaults['exe_trace'] = debug_trace
        job_trace = 'jobs' in debug_trace.split(',')
        rtems_tools = opts.find_arg('--rtems-tools')
        if rtems_tools is not None:
            if len(rtems_tools) != 2:
                raise error.general('invalid RTEMS tools option')
            rtems_tools = rtems_tools[1]
        else:
            rtems_tools = '%{_prefix}'
        bsp = opts.find_arg('--rtems-bsp')
        if bsp is None or len(bsp) != 2:
            raise error.general('RTEMS BSP not provided or an invalid option')
        bsp = tester.rt.config.load(bsp[1], opts)
        bsp_config = opts.defaults.expand(opts.defaults['tester'])
        coverage_enabled = opts.find_arg('--coverage')
        if coverage_enabled:
            cov_trace = 'cov' in debug_trace.split(',')
            if len(coverage_enabled) == 2:
                coverage_runner = tester.rt.coverage.coverage_run(opts.defaults,
                                                           executables,
                                                           rtems_tools,
                                                           symbol_set = coverage_enabled[1],
                                                           trace = cov_trace)
            else:
                coverage_runner = tester.rt.coverage.coverage_run(opts.defaults,
                                                           executables,
                                                           rtems_tools,
                                                           trace = cov_trace)
        log_mode = opts.find_arg('--log-mode')
        if log_mode:
            if log_mode[1] != 'failures' and \
                    log_mode[1] != 'all' and \
                    log_mode[1] != 'none':
                raise error.general('invalid report mode')
            log_mode = log_mode[1]
        else:
            log_mode = 'failures'
        if len(executables) == 0:
            raise error.general('no executables supplied')
        start_time = datetime.datetime.now()
        total = len(executables)
        reports = tester.rt.report.report(total)
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
                                                log_mode,
                                                reporting,
                                                finished,
                                                job_trace)
        finished_time = datetime.datetime.now()
        reporting = report_finished(reports, log_mode,
                                    reporting, finished, job_trace)
        if reporting < total:
            log.warning('finished jobs does match: %d' % (reporting))
            report_finished(reports, log_mode, -1, finished, job_trace)
        reports.summary()
        end_time = datetime.datetime.now()
        average_time = 'Average test time: %s' % (str((end_time - start_time) / total))
        total_time = 'Testing time     : %s' % (str(end_time - start_time))
        log.notice(average_time)
        log.notice(total_time)
        for report_format in report_formats:
            report_formatters[report_format](args, reports, start_time,
                    end_time, total, '.'.join([report_location, report_format]))

        if mail is not None and output is not None:
            m_arch = opts.defaults.expand('%{arch}')
            m_bsp = opts.defaults.expand('%{bsp}')
            build = ' %s:' % (reports.get_config('build', not_found = ''))
            subject = '[rtems-test] %s/%s:%s %s' % (m_arch,
                                                    m_bsp,
                                                    build,
                                                    reports.score_card('short'))
            np = 'Not present in test'
            ver = reports.get_config('version', not_found = np)
            build = reports.get_config('build', not_found = np)
            tools = reports.get_config('tools', not_found = np)
            body = [total_time, average_time,
                    '', 'Host', '====', host.label(mode = 'all'),
                    '', 'Configuration', '=============',
                    'Version: %s' % (ver),
                    'Build  : %s' % (build),
                    'Tools  : %s' % (tools),
                    '', 'Summary', '=======', '',
                    reports.score_card(), '',
                    reports.failures(),
                    'Log', '===', ''] + output.get()
            mail.send(to_addr, subject, os.linesep.join(body))
        if coverage_enabled:
            coverage_runner.run()

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
        tester.rt.console.restore(stdtty)
    sys.exit(0)

if __name__ == "__main__":
    run()
