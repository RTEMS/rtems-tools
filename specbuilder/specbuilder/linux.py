#
# $Id$
#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010 Chris Johns (chrisj@rtems.org)
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
    smp_mflags = ''
    processors = '/bin/grep processor /proc/cpuinfo'
    e = execute.capture_execution()
    exit_code, proc, output = e.shell(processors)
    if exit_code == 0:
        cpus = 0
        for l in output.split('\n'):
            count = l.split(':')[1].strip()
            if count > cpus:
                cpus = int(count)
        if cpus > 0:
            smp_mflags = '-j%d' % (cpus) 
    defines = { 
        '_os':                     'linux',
        '_host':                   uname[4] + '-linux-gnu',
        '_host_vendor':            'gnu',
        '_host_os':                'linux',
        '_host_cpu':               uname[4],
        '_host_alias':             '%{nil}',
        '_host_arch':              uname[4],
        '_usr':                    '/usr',
        '_var':                    '/usr/var',
        'optflags':                '-O2 -fasynchronous-unwind-tables',
        '_smp_mflags':             smp_mflags,
        '__bzip2':                 '/usr/bin/bzip2',
        '__gzip':                  '/bin/gzip',
        '__tar':                   '/bin/tar'
        }
    return defines

if __name__ == '__main__':
    pprint.pprint(load())
