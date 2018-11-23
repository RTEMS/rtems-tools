#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2017 Chris Johns (chrisj@rtems.org)
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
# Manage paths locally. The internally the path is in Unix or shell format and
# we convert to the native format when performing operations at the Python
# level. This allows macro expansion to work.
#

from __future__ import print_function

import copy
import os

from rtemstoolkit import error

def line(cols, line = '-', marker = '|', indent = 0, linesep = os.linesep):
    s = ' ' * indent + marker
    for c in cols:
        s += line[0] * int((c - 1)) + marker
    return s + linesep

def row(cols, data, indent = 0, marker = '|', linesep = os.linesep):
    if len(cols) != len(data):
        raise error.internal('data size (%d) does not' \
                             ' match columns (%d)' % (len(data), len(cols)))
    s = ' ' * indent + '|'
    for c in range(0, len(cols)):
        if c < len(cols) - 1:
            m = marker
        else:
            m = '|'
        s += '%-*s%s' % (int(cols[c] - 1), str(data[c]), m)
    return s + linesep

def even_columns(cols, width = 80):
    per_col = width / cols
    columns = [per_col for c in range(0, cols)]
    for remainder in range(0, int(width - (per_col * cols))):
        if remainder % 2 == 0:
            columns[remainder] += 1
        else:
            columns[len(columns) - remainder] += 1
    return columns

def merge_columns(columns):
    columns = copy.deepcopy(columns)
    cols = []
    while True:
        empty = True
        for c in columns:
            if len(c) != 0:
                empty = False
                break
        if empty:
            break
        lowest = 0
        lowest_size = 99999
        for c in range(0, len(columns)):
            if len(columns[c]) > 0 and columns[c][0] < lowest_size:
                lowest = c
                lowest_size = columns[c][0]
        cols += [lowest_size]
        columns[lowest] = columns[lowest][1:]
        for c in range(0, len(columns)):
            if len(columns[c]) > 0 and c != lowest:
                if columns[c][0] != lowest_size:
                    columns[c][0] -= lowest_size
                else:
                    columns[c] = columns[c][1:]
    return cols

if __name__ == '__main__':
    width = 75
    cols_1 = [width]
    cols_2 = [10, width - 10]
    cols_3 = even_columns(3, width = width)
    cols_4 = even_columns(4, width = width)
    print(line(cols_1, marker = 'X', linesep = ''))
    print(line(cols_1, marker = '+', indent = 1, linesep = ''))
    print(line(cols_1, marker = '+', indent = 2, linesep = ''))
    print(line(cols_2, marker = '+', indent = 2, linesep = ''))
    print(line(cols_3, marker = '+', indent = 2, linesep = ''))
    print(line(cols_4, marker = '+', indent = 2, linesep = ''))
    print(row(cols_4, [' %d' % (i) for i in [1, 2, 3, 4]], indent = 2, linesep = ''))
    print(line(cols_4, marker = '+', indent = 2, linesep = ''))
    print(line(merge_columns([cols_2, cols_3, cols_4]), indent = 2, linesep = ''))
