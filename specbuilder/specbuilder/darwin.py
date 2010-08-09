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

def load():
    uname = os.uname()
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
        '_smp_mflags':             '-j4',
        # Build readline with gdb.
        'without_system_readline': 'without_system_readline'
        }
    return defines

if __name__ == '__main__':
    pprint.pprint(load())
