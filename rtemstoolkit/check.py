#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2016 Chris Johns (chrisj@rtems.org)
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
# Check the defaults for a specific host.
#

from __future__ import print_function

import os

from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import log
from rtemstoolkit import options
from rtemstoolkit import path
from rtemstoolkit import version

def _check_none(_opts, macro, value, constraint):
    return True


def _check_triplet(_opts, macro, value, constraint):
    return True


def _check_dir(_opts, macro, value, constraint, silent = False):
    if constraint != 'none' and not path.isdir(value):
        if constraint == 'required':
            if not silent:
                log.notice('error: dir: not found: (%s) %s' % (macro, value))
            return False
        if not silent and _opts.warn_all():
            log.notice('warning: dir: not found: (%s) %s' % (macro, value))
    return True


def _check_exe(_opts, macro, value, constraint, silent = False):

    if len(value) == 0 or constraint == 'none':
        return True

    orig_value = value

    if path.isabspath(value):
        if path.isfile(value):
            return True
        if os.name == 'nt':
            if path.isfile('%s.exe' % (value)):
                return True
        value = path.basename(value)
        absexe = True
    else:
        absexe = False

    paths = os.environ['PATH'].split(os.pathsep)

    if _check_paths(value, paths):
        if absexe:
            if not silent:
                log.notice('warning: exe: absolute exe found in path: (%s) %s' % (macro, orig_value))
        return True

    if constraint == 'optional':
        if not silent:
            log.trace('warning: exe: optional exe not found: (%s) %s' % (macro, orig_value))
        return True

    if not silent:
        log.notice('error: exe: not found: (%s) %s' % (macro, orig_value))
    return False


def _check_paths(name, paths):
    for p in paths:
        exe = path.join(p, name)
        if path.isfile(exe):
            return True
        if os.name == 'nt':
            if path.isfile('%s.exe' % (exe)):
                return True
    return False


def host_setup(opts):
    """ Basic sanity check. All executables and directories must exist."""

    checks = { 'none':    _check_none,
               'triplet': _check_triplet,
               'dir':     _check_dir,
               'exe':     _check_exe }

    sane = True

    for d in list(opts.defaults.keys()):
        try:
            (test, constraint, value) = opts.defaults.get(d)
        except:
            if opts.defaults.get(d) is None:
                raise error.general('invalid default: %s: not found' % (d))
            else:
                raise error.general('invalid default: %s [%r]' % (d, opts.defaults.get(d)))
        if test != 'none':
            value = opts.defaults.expand(value)
            if test not in checks:
                raise error.general('invalid check test: %s [%r]' % (test, opts.defaults.get(d)))
            ok = checks[test](opts, d, value, constraint)
            if ok:
                tag = ' '
            else:
                tag = '*'
            log.trace('%c %15s: %r -> "%s"' % (tag, d, opts.defaults.get(d), value))
            if sane and not ok:
                sane = False

    return sane


def check_exe(label, exe, silent = True):
    return _check_exe(None, label, exe, None, silent)


def check_dir(label, path, silent = True):
    return _check_dir(None, label, path, 'required', silent)


def run(args):
    import sys
    try:
        _opts = options.command_line(argv = args)
        options.load(_opts)
        log.notice('RTEMS Toolkit - Check, v%s' % (version.string()))
        if host_setup(_opts):
            print('Environment is ok')
        else:
            print('Environment is not correctly set up')
    except error.general as gerr:
        print(gerr)
        sys.exit(1)
    except error.internal as ierr:
        print (ierr)
        sys.exit(1)
    except error.exit:
        pass
    except KeyboardInterrupt:
        log.notice('abort: user terminated')
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    run(['tester'])
