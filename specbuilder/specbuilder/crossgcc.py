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
# This code builds a cross-gcc compiler suite of tools given an architecture.
#

import distutils.dir_util
import operator
import os

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

class crossgcc:
    """Build a Cross GCC Compiler tool suite."""

    _order = { 'binutils': 0,
               'gcc'     : 1,
               'gdb'     : 2 }

    def __init__(self, arch, _defaults, opts):
        self.arch = arch
        self.opts = opts
        self.defaults = _defaults

    def _output(self, text):
        if not self.opts.quiet():
            log.output(text)

    def copy(self, src, dst):
        what = src + ' -> ' + dst
        _notice(self.opts, 'coping: ' + what)
        try:
            files = distutils.dir_util.copy_tree(src, dst)
            for f in files:
                self._output(f)
        except IOError, err:
            raise error.general('coping tree: ' + what + ': ' + str(err))

    def first_package(self, _build):
        path = os.path.join(_build.spec.abspath('%{_tmppath}'),
                            _build.spec.expand('crossgcc-%(%{__id_u} -n)'))
        _build.rmdir(path)
        _build.mkdir(path)
        prefix = os.path.join(_build.spec.expand('%{_prefix}'), 'bin')
        if prefix[0] == os.sep:
            prefix = prefix[1:]
        binpath = os.path.join(path, prefix)
        os.environ['PATH'] = binpath + os.pathsep + os.environ['PATH']
        self._output('path: ' + os.environ['PATH'])
        return path

    def every_package(self, _build, path):
        self.copy(_build.spec.abspath('%{buildroot}'), path)
        _build.cleanup()

    def last_package(self, _build, path):
        tar = os.path.join('%{_rpmdir}', self.arch + '-tools.tar.bz2')
        cmd = _build.spec.expand("'cd " + path + \
                                     " && %{__tar} -cf - . | %{__bzip2} > " + tar + "'")
        _build.run(cmd, shell_opts = '-c', cwd = path)
        if not self.opts.no_clean():
            _build.rmdir(path)

    def make(self):

        def _sort(specs):
            _specs = {}
            for spec in specs:
                for order in crossgcc._order:
                    if spec.lower().find(order) >= 0:
                        _specs[spec] = crossgcc._order[order]
            sorted_specs = sorted(_specs.iteritems(), key = operator.itemgetter(1))
            specs = []
            for s in range(0, len(sorted_specs)):
                specs.append(sorted_specs[s][0])
            return specs

        _notice(self.opts, 'arch: ' + self.arch)
        specs = self.opts.get_spec_files('*' + self.arch + '*')
        arch_specs = []
        for spec in specs:
            if spec.lower().find(self.arch.lower()) >= 0:
                arch_specs.append(spec)
        arch_specs = _sort(arch_specs)
        self.opts.opts['no-clean'] = '1'
        current_path = os.environ['PATH']
        try:
            for s in range(0, len(arch_specs)):
                b = build.build(arch_specs[s], _defaults = self.defaults, opts = self.opts)
                if s == 0:
                    path = self.first_package(b)
                b.make()
                self.every_package(b, path)
                if s == len(arch_specs) - 1:
                    self.last_package(b, path)
                del b
        finally:
            os.environ['PATH'] = current_path

def run():
    import sys
    try:
        opts, _defaults = defaults.load(sys.argv)
        log.default = log.log(opts.logfiles())
        _notice(opts, 'RTEMS Tools, CrossGCC Spec Builder, v%s' % (version))
        for arch in opts.params():
            c = crossgcc(arch, _defaults = _defaults, opts = opts)
            c.make()
            del c
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    except KeyboardInterrupt:
        _notice(opts, 'user terminated')
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    run()
