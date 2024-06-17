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

import os
import sys
import re
import pytest


from rtemstoolkit import options
from rtemstoolkit import macros
from rtemstoolkit import log


def test_log(capsys, tmpdir):
    file_log = os.path.join(tmpdir, "log.txt")
    l = log.log(['stdout', file_log], tail_size=20)

    for i in range(0, 10):
        l.output('log: hello world: %d\n' % (i))
    l.output('log: hello world CRLF\r\n')
    l.output('log: hello world NONE')
    l.flush()
    stdout, stderr = capsys.readouterr()

    with open(file_log, "r") as fp:
        data = fp.read()

    assert data == stdout
    assert len(l.tail) == 12


@pytest.fixture()
def log_log(tmpdir):
    file_log = os.path.join(tmpdir, "log.txt")

    yield log.log(['stdout', file_log], tail_size=20)

    # Reset due to globals
    log.quiet = False
    log.tracing = False


@pytest.mark.skip("stdout capture issues")
def test_log_quiet_f_trace_f(capfd, log_log):
    log.quiet = False
    log.tracing = False
    log.trace('trace-test')
    log.notice('log-test')
    stdout, stderr = capfd.readouterr()
    assert stdout == "log-test\n"


@pytest.mark.skip("stdout capture issues")
def test_log_quiet_t_trace_f(capfd, log_log):
    log.quiet = True
    log.tracing = False
    log.trace('trace-test')
    log.notice('log-test')
    stdout, stderr = capfd.readouterr()
    assert stdout == "log-test\n"


@pytest.mark.skip("stdout capture issues")
def test_log_quiet_f_trace_t(capfd, log_log):
    log.quiet = False
    log.tracing = True
    log.trace('trace-test')
    log.notice('log-test')
    stdout, stderr = capfd.readouterr()
    assert stdout == "trace-test\nlog-test\n"


@pytest.mark.skip("stdout capture issues")
def test_log_quiet_t_trace_t(capfd, log_log):
    log.quiet = True
    log.tracing = True
    log.trace('trace-test')
    log.notice('log-test')
    stdout, stderr = capfd.readouterr()
    assert stdout == "trace-test\nlog-test\n"
