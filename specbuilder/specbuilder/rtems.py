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
# Any RTEMS specific overrides to make things work.
#

import pprint

def load():
    defines = { 
        # Build readline with gdb.
        'without_system_readline': 'without_system_readline',
        # Work around a spec file issue.
        'mpc_provided':            '0',
        'mpfr_provided':           '0',
        'gmp_provided':            '0',
        'libelf_provided':         '0',
        '__setup_autoconf':        '2.65'
        }
    return defines

if __name__ == '__main__':
    pprint.pprint(load())
