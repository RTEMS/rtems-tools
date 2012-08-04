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
import execute
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

    def _install_files(self, files, dst):
        for src in files:
            try:
                name = os.path.basename(src)
                self._output('installing: ' + name)
                shutil.copy(src, os.path.join(dst, name))
            except IOError, ioerr:
                raise error.general('copy failed: ' + src + ': ' + str(ioerr))
            except OSError, oerr:
                 raise error.general('copy failed: ' + src + ': ' + str(oerr))

    def _get_file_list(self, top, path, ext):
        filelist = []
        for root, dirs, files in os.walk(top):
            if root[len(top) + 1:].startswith(path):
                for f in files:
                    n, e = os.path.splitext(f)
                    if e[1:] == ext:
                        filelist.append(os.path.join(root, f))
        return filelist

    def run(self, command, shell_opts = '', cwd = None):
        e = execute.capture_execution(log = log.default, dump = self.opts.quiet())
        cmd = self.opts.expand('%{___setup_shell} -ex ' + \
                                   shell_opts + ' ' + command, self.defaults)
        self._output('run: ' + cmd)
        exit_code, proc, output = e.shell(cmd, cwd = cwd)
        if exit_code != 0:
            raise error.general('shell cmd failed: ' + cmd)

    def check_version(self, cmd, macro):
        vcmd = cmd + ' --version'
        vcmd = self.opts.expand('%{___setup_shell} -e ' + vcmd, self.defaults)
        e = execute.capture_execution()
        exit_code, proc, output = e.shell(vcmd)
        if exit_code != 0 and len(output) != 0:
            raise error.general('shell cmd failed: ' + vcmd)
        version = output.split('\n')[0].split(' ')[-1:][0]
        need = self.opts.expand(macro, self.defaults)
        if version < need:
            _notice(self.opts, 'warning: ' + cmd + \
                        ' version is invalid, need ' + need + ' or higher, found ' + version)
            return False
        return True

    def get_specs(self, path):
        return self._get_file_list(path, 'autotools', 'spec') + \
            self._get_file_list(path, 'rtems', 'spec')

    def get_patches(self, path):
        return self._get_file_list(path, 'patches', 'diff')

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
                if oerr[0] != errno.EEXIST and oerr[0] != 183:
                    raise error.general('OS error: ' + str(oerr))
            if d == 'RPMLIB':
                files = []
                for file in ['perl.prov', 'perl.req']:
                    files.append(os.path.join(self.opts.command_path, 'specbuilder', file))
                self._install_files(files, dst)

    def build_crossrpms(self, path):
        if 'rtems' in self.opts.opts:
            crossrpms = os.path.abspath(os.path.expanduser(self.opts.opts['rtems']))
            if not os.path.isdir(crossrpms):
                raise error.general('no crossrpms directory found under: ' + crossrpms)
            if self.opts.rebuild():
                if self.check_version('autoconf', '%{__setup_autoconf}'):
                    self.run('../../bootstrap -c', '-c', crossrpms)
                    self.run('../../bootstrap', '-c', crossrpms)
                    self.run('./configure', '-c', crossrpms)
            self._install_files(self.get_specs(crossrpms), os.path.join(path, 'SPECS'))
            self._install_files(self.get_patches(crossrpms), os.path.join(path, 'SOURCES'))

def run():
    import sys
    try:
        opts, _defaults = defaults.load(sys.argv)
        log.default = log.log(opts.logfiles())
        _notice(opts, 'RTEMS Tools, Setup Spec Builder, v%s' % (version))
        for path in opts.params():
            s = setup(path, _defaults = _defaults, opts = opts)
            s.make(path)
            s.build_crossrpms(path)
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


