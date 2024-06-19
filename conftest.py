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
import os


def _get_top():
    return os.path.dirname(os.path.abspath(__file__))


def _get_git():
    if os.path.exists(os.path.join(_get_top(), "VERSION")):
        return False
    return True


# Always find the parent directory of rtemstoolkit/ no matter where
# pytest is run.
def pytest_runtest_setup(item):
    sys.path.append(_get_top())

    git = _get_git()

    mark_map = [mark.name for mark in item.iter_markers()]

    if "win32" in mark_map and sys.platform != "win32":
        pytest.skip("Skipping Windows test")

    if "unix" in mark_map and sys.platform == "win32":
        pytest.skip("Skipping UNIX test")

    if "git" in mark_map and not _get_git():
        pytest.skip("Skipping Git tests in release version")


@pytest.fixture
def rt_topdir():
    """Top rtems-tools directory"""
    return _get_top()


@pytest.fixture()
def patch_argv(rt_topdir):
    """Fake running as an executable file"""
    sys.argv = [os.path.join(rt_topdir, "somefile")]

    # Used for nicer debug output
    return rt_topdir


def pytest_configure(config):
    config.addinivalue_line("markers", "win32: Only run test on Windows")
    config.addinivalue_line("markers", "unix: Only run test on UNIX")
    config.addinivalue_line("markers",
                            "git: Git tests only run in devel version")
