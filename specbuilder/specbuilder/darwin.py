#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2012 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# RTEMS Tools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTEMS Tools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RTEMS Tools.  If not, see <http://www.gnu.org/licenses/>.
#

#
# This code is based on what ever doco about spec files I could find and
# RTEMS project's spec files.
#

import pprint
import os

import execute

def load():
    uname = os.uname()
    sysctl = '/usr/sbin/sysctl '
    e = execute.capture_execution()
    exit_code, proc, output = e.shell(sysctl + 'hw.ncpu')
    if exit_code == 0:
        smp_mflags = '-j' + output.split(' ')[1].strip()
    else:
        smp_mflags = ''
    defines = {
        '_os':                     'darwin',
        '_host':                   uname[4] + '-apple-darwin' + uname[2],
        '_host_vendor':            'apple',
        '_host_os':                'darwin',
        '_host_cpu':               uname[4],
        '_host_alias':             '%{nil}',
        '_host_arch':              uname[4],
        '_usr':                    '/opt/local',
        '_var':                    '/opt/local/var',
        'optflags':                '-O2 -fasynchronous-unwind-tables',
        '_smp_mflags':             smp_mflags,
        '__xz':                    '/usr/local/bin/xz',
        # Work around the broken sed code on BSD sed
        #'without_gcc_std':         'True',
        'with_zlib':               '--with-zlib=no',
        }
    return defines

if __name__ == '__main__':
    pprint.pprint(load())
