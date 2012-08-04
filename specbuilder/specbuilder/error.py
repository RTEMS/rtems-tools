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
# Various errors we can raise.
#

class error(Exception):
    """Base class for Builder exceptions."""
    def set_output(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class general(error):
    """Raise for a general error."""
    def __init__(self, what):
        self.set_output('error: ' + what)

class internal(error):
    """Raise for an internal error."""
    def __init__(self, what):
        self.set_output('internal error: ' + what)

if __name__ == '__main__':
    try:
        raise general('a general error')
    except general, gerr:
        print 'caught:', gerr
    try:
        raise internal('an internal error')
    except internal, ierr:
        print 'caught:', ierr
