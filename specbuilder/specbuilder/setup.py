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
# Setup a series of directories ready for building with the Spec Builder.
#

import errno
import os
import shutil

import build
import defaults
import error
import log

#
# Version of Spec CrossGCC Builder.
#
version = '0.1'

def _notice(opts, text):
    if not opts.quiet() and not log.default.has_stdout():
        print text
    log.output(text)
    log.flush()

class setup:
    """Set up the various directories in a specified path"""

    _dirs = [ 'TARS',
              'SPECS',
              'SOURCES',
              'RPMLIB',
              'BUILD',
              'TMP' ]

    def __init__(self, path, _defaults, opts):
        self.path = path
        self.opts = opts
        self.defaults = _defaults

    def _output(self, text):
        if not self.opts.quiet():
            log.output(text)

    def mkdir(self, path):
        if not self.opts.dry_run():
            self._output('making dir: ' + path)
            try:
                os.makedirs(path)
            except IOError, err:
                raise error.general('error creating path: ' + path)

    def make(self, path):
        for d in setup._dirs:
            try:
                dst = os.path.join(path, d)
                self.mkdir(dst)
            except os.error, oerr:
                if oerr[0] != errno.EEXIST:
                    raise error.general('OS error: ' + str(oerr))
            if d == 'RPMLIB':
                for n in ['perl.prov', 'perl.req']:
                    sf = os.path.join(self.opts.command_path, 'specbuilder', n)
                    df = os.path.join(dst, n)
                    self._output('installing: ' + df)
                    if os.path.isfile(sf):
                        shutil.copy(sf, df)

def run():
    import sys
    try:
        opts, _defaults = defaults.load(sys.argv)
        log.default = log.log(opts.logfiles())
        _notice(opts, 'RTEMS Tools, Setup Spec Builder, v%s' % (version))
        for path in opts.params():
            s = setup(path, _defaults = _defaults, opts = opts)
            s.make(path)
            del s
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    except KeyboardInterrupt:
        print 'user terminated'
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    run()
