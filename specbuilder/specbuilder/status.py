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
# Extract the status of the spec files found in the SPECS directory.
# The status is the name, source and patches.
#

import os

import defaults
import error
import spec

class status:
    """Return the status of a package given a spec file."""

    def __init__(self, name, _defaults, opts):
        self.opts = opts
        self.spec = spec.file(name, _defaults = _defaults, opts = opts)

    def __str__(self):

        def str_package(package):
            sources = package.sources()
            patches = package.patches()
            version = package.version()
            release = package.release()
            buildarch = package.buildarch()
            s = '\n name: ' + package.name()
            if buildarch:
                s += '\n    arch: ' + buildarch
            if version:
                s += '\n version: ' + version
            if release:
                s += '\n release: ' + release
            if len(sources):
                s += '\n sources: %d' % (len(sources))
                c = 1
                for i in sources:
                    s += '\n  %d: ' % (c) + sources[i][0]
                    c += 1
                s += '\n patches: %d' % (len(patches))
                c = 1
                for i in patches:
                    s += '\n  %d: ' % (c) + patches[i][0]
                    c += 1
            return s

        packages = self.spec.packages()
        s = 'spec: ' + os.path.basename(self.spec.name) + \
            str_package(packages['main'])
        for p in packages:
            if p != 'main':
                s += str_package(packages[p])
        return s

def run():
    import sys
    try:
        opts, _defaults = defaults.load(sys.argv)
        for spec_file in opts.spec_files():
            s = status(spec_file, _defaults = _defaults, opts = opts)
            print s
            del s
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    run()
