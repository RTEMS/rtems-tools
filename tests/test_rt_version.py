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
import re
import sys

import pytest


@pytest.skip("rtemstoolkit/version.py uses globals", allow_module_level=True)
# Since we aren't executing this from a binary we need to hack
# a file path in.

@pytest.fixture(autouse=True)
def patch_argv(rt_topdir):
    sys.argv = os.path.join(rt_topdir, "somefile")


def test_version(rt_topdir, patch_argv):
    sys.argv = os.path.join(rt_topdir, "somefile")

    from rtemstoolkit import version
    assert type(version.version()) is int

def test_revision():
    revision = version.revision()

    # The version can sometimes have -modified at the end if there are changes
    # waiting to be committed.
    assert re.match("^[a-z0-9].*$", revision)
    assert len(revision) == 12 or 21


def test_string():
    assert re.match("^[0-9]* \\([a-z0-9].*\\)$", version.string())


def test_undefined():
    assert version.version() != "undefined"
