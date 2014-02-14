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
# Determine the defaults and load the specific file.
#

import glob
import pprint
import re
import os
import string

from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import git
from rtemstoolkit import log
from rtemstoolkit import macros
from rtemstoolkit import options
from rtemstoolkit import path

import version

class command_line(options.command_line):
    """Process the command line in a common way for all Tool Builder commands."""

    def __init__(self, argv = None, optargs = None, defaults = None, command_path = None):
        if argv is None:
            return
        long_opts = {
            # key             macro            handler      param  defs   init
            '--target'     : ('_target',       "triplet",   True,  None,  False),
            '--timeout'    : ('timeout',       "int",       True,  None,  False),
        }
        long_opts_help = {
            '--target': 'Set the target triplet',
            '--timeout': 'Set the test timeout in seconds (default 180 seconds)'
        }
        super(command_line, self).__init__('rt', argv, optargs, defaults,
                                           long_opts, long_opts_help, command_path);

    def __copy__(self):
        return super(command_line, self).__copy__()

def load(args, optargs = None,
         command_path = None,
         defaults = '%{_rtdir}/rtems/testing/defaults.mc'):
    #
    # The path to this command if not supplied by the upper layers.
    #
    if command_path is None:
        command_path = path.dirname(args[0])
        if len(command_path) == 0:
            command_path = '.'
    #
    # The command line contains the base defaults object all build objects copy
    # and modify by loading a configuration.
    #
    opts = command_line(args,
                        optargs,
                        macros.macros(name = defaults,
                                      rtdir = command_path),
                        command_path)
    options.load(opts)
    return opts

def run(args):
    try:
        _opts = load(args = args, defaults = 'rtems/testing/defaults.mc')
        log.notice('RTEMS Test - Defaults, v%s' % (version.str()))
        _opts.log_info()
        log.notice('Options:')
        log.notice(str(_opts))
        log.notice('Defaults:')
        log.notice(str(_opts.defaults))
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    except error.exit, eerr:
        pass
    except KeyboardInterrupt:
        log.notice('abort: user terminated')
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
    import sys
    run(sys.argv)
