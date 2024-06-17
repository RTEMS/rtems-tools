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

from rtemstoolkit import textbox


@pytest.fixture
def width():
    return 75


@pytest.fixture
def cols_1(width):
    return [width]


@pytest.fixture
def cols_2(width):
    return [10, width - 10]


@pytest.fixture
def cols_3(width):
    return textbox.even_columns(3, width=width)


@pytest.fixture
def cols_4(width):
    return textbox.even_columns(4, width=width)


def test_cols_1_1(cols_1):
    assert textbox.line(cols_1, marker = 'X', linesep = '') == \
                        "X--------------------------------------------------------------------------X"


def test_cols_1_2(cols_1):
    assert textbox.line(cols_1, marker = '+', indent = 1, linesep = '') ==\
                        " +--------------------------------------------------------------------------+"


def test_cols_1_3(cols_1):
    assert textbox.line(cols_1, marker = '+', indent = 2, linesep = '') == \
                        "  +--------------------------------------------------------------------------+"


def test_cols_1_cols_2(cols_1, cols_2):
    assert textbox.line(cols_2, marker = '+', indent = 2, linesep = '') == \
                        "  +---------+----------------------------------------------------------------+"


def test_cols_3(cols_3):
    assert textbox.line(cols_3, marker = '+', indent = 2, linesep = '') == \
                        "  +------------------------+------------------------+------------------------+"


def test_cols_4_1(cols_4):
    assert textbox.line(cols_4, marker = '+', indent = 2, linesep = '') == \
                        "  +------------------+-----------------+------------------+------------------+"


def test_cols_4_2(cols_4):
    assert textbox.row(cols_4, [' %d' % (i) for i in [1, 2, 3, 4]], indent = 2,
                       linesep = '') == \
                        "  | 1                | 2               | 3                | 4                |"


def test_cols_4_3(cols_4):
    assert textbox.line(cols_4, marker = '+', indent = 2, linesep = '') == \
                        "  +------------------+-----------------+------------------+------------------+"


def test_cols_2_cols_3_cols_4(cols_2, cols_3, cols_4):
    assert textbox.line(textbox.merge_columns([cols_2, cols_3, cols_4]),
                        indent = 2, linesep = '') == \
                        "  |---------|--------|-----|-----------|------------|-----|------------------|"
