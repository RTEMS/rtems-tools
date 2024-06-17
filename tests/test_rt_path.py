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


from rtemstoolkit import path


rtems_windows = pytest.mark.parametrize("windows", [False, True], \
                ids=["Windows=False", "Windows=True"])


@rtems_windows
def test_host(windows):
    path.windows = windows

    if windows:
        path_str = "a:\\b\\c\\d-e-f"
    else:
        path_str = "/a/b/c/d-e-f"

    assert path.host('//a/b/c/d-e-f') == path_str


@rtems_windows
def test_shell(windows):
    path.windows = windows
    assert path.shell('/w/x/y/z') == "/w/x/y/z"


@rtems_windows
def test_basename(windows):
    path.windows = windows
    assert path.basename('/as/sd/df/fg/me.txt') == "me.txt"

    if windows:
        assert path.basename('x:/sd/df/fg/me.txt') == "me.txt"


@rtems_windows
def test_dirname(windows):
    path.windows = windows
    assert path.dirname('/as/sd/df/fg/me.txt') == "/as/sd/df/fg"

    if windows:
        assert path.dirname('x:/sd/df/fg/me.txt') == "/x/sd/df/fg"


@rtems_windows
def test_join(windows):
    path.windows = windows
    assert path.join('/d', 'g', '/tyty/fgfg') == "/d/g/tyty/fgfg"

    if windows:
        assert path.join('s:/d/e\\f/g', '/h', '/tyty/zxzx',
                         '\\mm\\nn/p') == "/s/d/e/f/g/h/tyty/zxzx/mm/nn/p"
        assert path.join('s:/d/', '/g', '/tyty/fgfg') == "/s/d/g/tyty/fgfg"
