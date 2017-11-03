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

from __future__ import print_function

import copy
import datetime
import fnmatch
import os
import re
import sys
import threading
import time

from rtemstoolkit import error
from rtemstoolkit import host
from rtemstoolkit import log
from rtemstoolkit import path
from rtemstoolkit import stacktraces
from rtemstoolkit import version

from . import bsps
from . import config
from . import console
from . import options
from . import report

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
        self.config = config.file(index, total, self.report, self.bsp_config, self.opts, '')

    def run(self):
        if self.config:
            self.config.run()

    def kill(self):
        if self.config:
            self.config.kill()

def find_executables(files):
    executables = []
    for f in files:
        if not path.isfile(f):
            raise error.general('executable is not a file: %s' % (f))
        executables += [f]
    return sorted(executables)

def list_bsps(opts):
    path_ = opts.defaults.expand('%%{_configdir}/bsps/*.mc')
    bsps = path.collect_files(path_)
    log.notice(' BSP List:')
    for bsp in bsps:
        log.notice('  %s' % (path.basename(bsp[:-3])))
    raise error.exit()

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
                    '--list-bsps':   'List the supported BSPs',
                    '--debug-trace': 'Debug trace based on specific flags',
                    '--stacktrace':  'Dump a stack trace on a user termination (^C)' }
        opts = options.load(sys.argv,
                            optargs = optargs,
                            command_path = command_path)
        log.notice('RTEMS Testing - Run, %s' % (version.str()))
        if opts.find_arg('--list-bsps'):
            bsps.list(opts)
        opts.log_info()
        log.output('Host: ' + host.label(mode = 'all'))
        debug_trace = opts.find_arg('--debug-trace')
        if debug_trace:
            if len(debug_trace) != 1:
                debug_trace = 'output,' + debug_trace[1]
            else:
                raise error.general('no debug flags, can be: console,gdb,output')
        else:
            debug_trace = 'output'
        opts.defaults['debug_trace'] = debug_trace
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
        executables = find_executables(opts.params())
        if len(executables) != 1:
            raise error.general('one executable required, found %d' % (len(executables)))
        start_time = datetime.datetime.now()
        opts.defaults['exe_trace'] = debug_trace
        tst = test(1, 1, None, executables[0], rtems_tools, bsp, bsp_config, opts)
        tst.run()
        end_time = datetime.datetime.now()
        total_time = 'Run time     : %s' % (str(end_time - start_time))
        log.notice(total_time)

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
        sys.exit(1)
    finally:
        console.restore(stdtty)
    sys.exit(0)

if __name__ == "__main__":
    run()
