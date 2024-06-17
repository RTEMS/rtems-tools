# SPDX-License-Identifier: BSD-2-Clause

# RTEMS Tools Project (http://www.rtems.org/)
# Copyright (C) 2020-2024 Amar Takhar <amar@rtems.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import pytest
import sys
import re


from rtemstoolkit import options
from rtemstoolkit import macros
from rtemstoolkit import log

# version needs fixing
#from rtemstoolkit import version


def test_options(capsys):
    args = ["file_exec"]

    long_opts = {
        # key              macro         handler   param  defs   init
        '--test-path': ('_test_path', 'path', True, None, False),
        '--test-jobs': ('_test_jobs', 'jobs', True, 'max', True),
        '--test-bool': ('_test_bool', 'bool', False, '0', True)
    }
    opts = options.command_line(base_path='.',
                                argv=args,
                                optargs=None,
                                defaults=macros.macros(),
                                long_opts=long_opts,
                                command_path='.')
    options.load(opts)

    opts.log_info()
    stdout, stderr = capsys.readouterr()
    assert re.match("^ Command Line: file_exec\n Host:.*\n Python:.*$", stdout)

    # This doesn't work because version needs fixing
    #    log.notice('RTEMS Tools Project - Defaults, v%s' % (version.string()))

    log.notice(str(opts))
    stdout, stderr = capsys.readouterr()
    assert re.match("^command: ./file_exec\nargs: \\[\\]\nopts:.*", stdout)

    log.notice(str(opts.defaults))
    stdout, stderr = capsys.readouterr()
    assert re.match("^\\[global\\] w,-.*", stdout)
