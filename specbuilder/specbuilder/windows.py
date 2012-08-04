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
# Windows specific support and overrides.
#

import pprint
import os

import execute

def load():
    uname = 'win32'
    if os.environ.has_key('NUMBER_OF_PROCESSORS'):
        ncpus = int(os.environ['NUMBER_OF_PROCESSORS'])
    else:
        ncpus = 0
    if ncpus > 1:
        smp_mflags = '-j' + str(ncpus)
    else:
        smp_mflags = ''
    if os.environ.has_key('HOSTTYPE'):
        hosttype = os.environ['HOSTTYPE']
    else:
        hosttype = 'i686'
    system = 'mingw32'
    defines = {
        '_os':                     'win32',
        '_host':                   hosttype + '-pc-' + system,
        '_host_vendor':            'microsoft',
        '_host_os':                'win32',
        '_host_cpu':               hosttype,
        '_host_alias':             '%{nil}',
        '_host_arch':              hosttype,
        '_usr':                    '/opt/local',
        '_var':                    '/opt/local/var',
        'optflags':                '-O2 -fasynchronous-unwind-tables',
        '_smp_mflags':             smp_mflags,
        '__sh':                    'sh',
        '_buildshell':             '%{__sh}',
        '___setup_shell':          '%{__sh}'
        }
    return defines

if __name__ == '__main__':
    pprint.pprint(load())
