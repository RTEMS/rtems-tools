#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2016 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# Provide some basic access to the git command.
#

import os

#
# Support to handle use in a package and as a unit test.
# If there is a better way to let us know.
#
try:
    from . import error
    from . import execute
    from . import log
    from . import path
except (ValueError, SystemError):
    import error
    import execute
    import log
    import path

class repo:
    """An object to manage a git repo."""

    def _git_exit_code(self, ec, cmd, output):
        if ec:
            log.notice('git: cmd: ' + ' '.join(cmd))
            log.notice('git: output: ' + output)
            raise error.general('git command failed (%s): %d' % (self.git, ec))

    def _run(self, args, check = False):
        e = execute.capture_execution()
        if path.exists(self.path):
            cwd = self.path
        else:
            cwd = None
        cmd = [self.git] + args
        log.trace('cmd: (%s) %s' % (str(cwd), ' '.join(cmd)))
        exit_code, proc, output = e.spawn(cmd, cwd = path.host(cwd))
        log.trace(output)
        if check:
            self._git_exit_code(exit_code, cmd, output)
        return exit_code, output

    def __init__(self, _path, opts = None, macros = None):
        self.path = _path
        self.opts = opts
        if macros is None and opts is not None:
            self.macros = opts.defaults
        else:
            self.macros = macros
        if self.macros is None:
            self.git = 'git'
        else:
            self.git = self.macros.expand('%{__git}')

    def git_version(self):
        ec, output = self._run(['--version'], True)
        gvs = output.split()
        if len(gvs) < 3:
            raise error.general('invalid version string from git: %s' % (output))
        vs = gvs[2].split('.')
        if len(vs) == 4:
            return (int(vs[0]), int(vs[1]), int(vs[2]), int(vs[3]))
        if len(vs) == 3:
            return (int(vs[0]), int(vs[1]), int(vs[2]))
        raise error.general('invalid version number from git: %s' % (gvs[2]))

    def clone(self, url, _path):
        ec, output = self._run(['clone', url, path.host(_path)], check = True)

    def fetch(self):
        ec, output = self._run(['fetch'], check = True)

    def merge(self):
        ec, output = self._run(['merge'], check = True)

    def pull(self):
        ec, output = self._run(['pull'], check = True)

    def reset(self, args):
        if type(args) == str:
            args = [args]
        ec, output = self._run(['reset'] + args, check = True)

    def branch(self):
        ec, output = self._run(['branch'])
        if ec == 0:
            for b in output.split('\n'):
                if b[0] == '*':
                    return b[2:]
        return None

    def checkout(self, branch = 'master'):
        ec, output = self._run(['checkout', branch], check = True)

    def submodule(self, module):
        ec, output = self._run(['submodule', 'update', '--init', module], check = True)

    def clean(self, args = []):
        if type(args) == str:
            args = [args]
        ec, output = self._run(['clean'] + args, check = True)

    def status(self):
        _status = {}
        if path.exists(self.path):
            ec, output = self._run(['status'])
            if ec == 0:
                state = 'none'
                for l in output.split('\n'):
                    if l.startswith('# '):
                        l = l[2:]
                    if l.startswith('On branch '):
                        _status['branch'] = l[len('On branch '):]
                    elif l.startswith('Changes to be committed:'):
                        state = 'staged'
                    elif l.startswith('Changes not staged for commit:'):
                        state = 'unstaged'
                    elif l.startswith('Untracked files:'):
                        state = 'untracked'
                    elif l.startswith('HEAD detached'):
                        state = 'detached'
                    elif state != 'none' and len(l.strip()) != 0:
                        if l[0].isspace():
                            l = l.strip()
                            if l[0] != '(':
                                if state not in _status:
                                    _status[state] = []
                                l = l[1:]
                                if ':' in l:
                                    l = l.split(':')[1]
                                _status[state] += [l.strip()]
        return _status

    def dirty(self):
        _status = self.status()
        return not (len(_status) == 1 and 'branch' in _status)

    def valid(self):
        if path.exists(self.path):
            ec, output = self._run(['status'])
            return ec == 0
        return False

    def remotes(self):
        _remotes = {}
        ec, output = self._run(['config', '--list'])
        if ec == 0:
            for l in output.split('\n'):
                if l.startswith('remote'):
                    ls = l.split('=')
                    if len(ls) >= 2:
                        rs = ls[0].split('.')
                        if len(rs) == 3:
                            r_name = rs[1]
                            r_type = rs[2]
                            if r_name not in _remotes:
                                _remotes[r_name] = {}
                            if r_type not in _remotes[r_name]:
                                _remotes[r_name][r_type] = []
                            _remotes[r_name][r_type] = '='.join(ls[1:])
        return _remotes

    def email(self):
        _email = None
        _name = None
        ec, output = self._run(['config', '--list'])
        if ec == 0:
            for l in output.split('\n'):
                if l.startswith('user.email'):
                    ls = l.split('=')
                    if len(ls) >= 2:
                        _email = ls[1]
                elif l.startswith('user.name'):
                    ls = l.split('=')
                    if len(ls) >= 2:
                        _name = ls[1]
        if _email is not None:
            if _name is not None:
                _email = '%s <%s>' % (_name, _email)
            return _email
        return None

    def head(self):
        hash = ''
        ec, output = self._run(['log', '-n', '1'])
        if ec == 0:
            l1 = output.split('\n')[0]
            if l1.startswith('commit '):
                hash = l1[len('commit '):]
        return hash

if __name__ == '__main__':
    import sys
    import options
    long_opts = {
       # key              macro        handler   param  defs   init
    }
    opts = options.command_line(base_path = '.',
                                argv = sys.argv,
                                long_opts = long_opts)
    options.load(opts)
    g = repo('.', opts)
    print('version:', g.git_version())
    print('valid:', g.valid())
    print('status:', g.status())
    print('dirty:', g.dirty())
    print('remotes:', g.remotes())
    print('email:', g.email())
    print('head:', g.head())
