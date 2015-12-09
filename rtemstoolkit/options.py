#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2014 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
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
# Determine the defaults and load the specific file.
#

import copy
import glob
import pprint
import re
import os
import string

import error
import execute
import git
import log
import macros
import path
import sys

import version

basepath = 'tb'

#
# Save the host state.
#
host_windows = False

class command_line(object):
    """Process the command line in a common way for all Tool Builder commands."""

    def __init__(self, base_path = None, argv = None, optargs = None,
                 defaults = None, long_opts = None, long_opts_help = None,
                 command_path = None, log_default = None):

        if argv is None:
            return

        global basepath

        if long_opts == None:
            raise error.general('No options provided')

        basepath = base_path

        if log_default is not None and type(log_default) is not list:
            raise error.general('log default is a list')
        self.log_default = log_default

        self.long_opts = {
            # key                 macro                handler            param  defs       init
            '--jobs'           : ('_jobs',             self._lo_jobs,     True,  'default', True),
            '--log'            : ('_logfile',          self._lo_string,   True,  None,      False),
            '--macros'         : ('_macros',           self._lo_string,   True,  None,      False),
            '--force'          : ('_force',            self._lo_bool,     False, '0',       True),
            '--quiet'          : ('_quiet',            self._lo_bool,     False, '0',       True),
            '--trace'          : ('_trace',            self._lo_bool,     False, '0',       True),
            '--dry-run'        : ('_dry_run',          self._lo_bool,     False, '0',       True),
            '--warn-all'       : ('_warn_all',         self._lo_bool,     False, '0',       True),
            '--no-clean'       : ('_no_clean',         self._lo_bool,     False, '0',       True),
            '--keep-going'     : ('_keep_going',       self._lo_bool,     False, '0',       True),
            '--always-clean'   : ('_always_clean',     self._lo_bool,     False, '0',       True),
            '--no-install'     : ('_no_install',       self._lo_bool,     False, '0',       True),
            '--help'           : (None,                self._lo_help,     False, None,      False)
        }
        self.long_opts_help = {
            '--force':                      'Force the build to proceed',
            '--quiet':                      'Quiet output (not used)',
            '--trace':                      'Trace the execution',
            '--dry-run':                    'Do everything but actually run the build',
            '--warn-all':                   'Generate warnings',
            '--no-clean':                   'Do not clean up the build tree',
            '--always-clean':               'Always clean the build tree, even with an error',
            '--keep-going':                 'Do not stop on an error.',
            '--jobs=[0..n,none,half,full]': 'Run with specified number of jobs, default: num CPUs.',
            '--macros file[,file]':         'Macro format files to load after the defaults',
            '--log file':                   'Log file where all build out is written too',
        }
        self.opts = { 'params' : [] }
        self.command_path = command_path
        self.command_name = path.basename(argv[0])
        self.argv = argv
        self.args = argv[1:]
        self.optargs = optargs
        self.defaults = defaults
        for lo in self.long_opts:
            self.opts[lo[2:]] = self.long_opts[lo][3]
            if self.long_opts[lo][4]:
                self.defaults[self.long_opts[lo][0]] = ('none', 'none', self.long_opts[lo][3])
        for lo in long_opts:
            if lo in self.long_opts:
                raise error.general('suplicate option: %s' % (lo))
            self.opts[lo[2:]] = long_opts[lo][3]
            if long_opts[lo][4]:
                self.defaults[long_opts[lo][0]] = ('none', 'none', long_opts[lo][3])
            if long_opts[lo][1] == 'int':
                handler = self._lo_int
            elif long_opts[lo][1] == 'string':
                handler = self._lo_string
            elif long_opts[lo][1] == 'path':
                hanlder = self._lo_path
            elif long_opts[lo][1] == 'jobs':
                handler = self._lo_jobs
            elif long_opts[lo][1] == 'bool':
                handler = self._lo_bool
            elif long_opts[lo][1] == 'triplet':
                handler = self._lo_triplets
            else:
                raise error.general('invalid option handler: %s: %s' % (lo, long_opts[lo][1]))
            self.long_opts[lo] = (long_opts[lo][0], handler, long_opts[lo][2],
                                   long_opts[lo][3], long_opts[lo][4])
            if lo not in long_opts_help:
                raise error.general('no help for option: %s' % (lo))
            self.long_opts_help[lo] = long_opts_help[lo]

    def __copy__(self):
        new = type(self)()
        #new.__dict__.update(self.__dict__)
        new.opts = copy.copy(self.opts)
        new.command_path = copy.copy(self.command_path)
        new.command_name = copy.copy(self.command_name)
        new.argv = self.argv
        new.args = self.args
        new.optargs = copy.copy(self.optargs)
        new.defaults = copy.copy(self.defaults)
        new.long_opts = copy.copy(self.long_opts)
        return new

    def __str__(self):
        def _dict(dd):
            s = ''
            ddl = dd.keys()
            ddl.sort()
            for d in ddl:
                s += '  ' + d + ': ' + str(dd[d]) + '\n'
            return s

        s = 'command: ' + self.command() + \
            '\nargs: ' + str(self.args) + \
            '\nopts:\n' + _dict(self.opts)

        return s

    def _lo_int(self, opt, macro, value):
        if value is None:
            raise error.general('option requires a value: %s' % (opt))
        try:
            num = int(value)
        except:
            raise error.general('option conversion to int failed: %s' % (opt))
        self.opts[opt[2:]] = value
        self.defaults[macro] = value

    def _lo_string(self, opt, macro, value):
        if value is None:
            raise error.general('option requires a value: %s' % (opt))
        self.opts[opt[2:]] = value
        self.defaults[macro] = value

    def _lo_path(self, opt, macro, value):
        if value is None:
            raise error.general('option requires a path: %s' % (opt))
        value = path.abspath(value)
        self.opts[opt[2:]] = value
        self.defaults[macro] = value

    def _lo_jobs(self, opt, macro, value):
        if value is None:
            raise error.general('option requires a value: %s' % (opt))
        ok = False
        if value in ['max', 'none', 'half']:
            ok = True
        else:
            try:
                i = int(value)
                ok = True
            except:
                pass
            if not ok:
                try:
                    f = float(value)
                    ok = True
                except:
                    pass
        if not ok:
            raise error.general('invalid jobs option: %s' % (value))
        self.defaults[macro] = value
        self.opts[opt[2:]] = value

    def _lo_bool(self, opt, macro, value):
        if value is not None:
            raise error.general('option does not take a value: %s' % (opt))
        self.opts[opt[2:]] = '1'
        self.defaults[macro] = '1'

    def _lo_triplets(self, opt, macro, value):
        #
        # This is a target triplet. Run it past config.sub to make make sure it
        # is ok. The target triplet is 'cpu-vendor-os'.
        #
        e = execute.capture_execution()
        config_sub = path.join(self.command_path,
                               basepath, 'config.sub')
        exit_code, proc, output = e.shell(config_sub + ' ' + value)
        if exit_code == 0:
            value = output
        self.defaults[macro] = ('triplet', 'none', value)
        self.opts[opt[2:]] = value
        _cpu = macro + '_cpu'
        _arch = macro + '_arch'
        _vendor = macro + '_vendor'
        _os = macro + '_os'
        _arch_value = ''
        _vendor_value = ''
        _os_value = ''
        dash = value.find('-')
        if dash >= 0:
            _arch_value = value[:dash]
            value = value[dash + 1:]
        dash = value.find('-')
        if dash >= 0:
            _vendor_value = value[:dash]
            value = value[dash + 1:]
        if len(value):
            _os_value = value
        self.defaults[_cpu]    = _arch_value
        self.defaults[_arch]   = _arch_value
        self.defaults[_vendor] = _vendor_value
        self.defaults[_os]     = _os_value

    def _lo_help(self, opt, macro, value):
        self.help()

    def _help_indent(self):
        indent = 0
        if self.optargs:
            for o in self.optargs:
                if len(o) > indent:
                    indent = len(o)
        for o in self.long_opts_help:
            if len(o) > indent:
                indent = len(o)
        return indent

    def help(self):
        print '%s: [options] [args]' % (self.command_name)
        print 'RTEMS Tools Project (c) 2012-2014 Chris Johns'
        print 'Options and arguments:'
        opts = self.long_opts_help.keys()
        if self.optargs:
            opts += self.optargs.keys()
        indent = self._help_indent()
        for o in sorted(opts):
            if o in self.long_opts_help:
                h = self.long_opts_help[o]
            elif self.optargs:
                h = self.optargs[o]
            else:
                raise error.general('invalid help data: %s' %(o))
            print '%-*s : %s' % (indent, o, h)
        raise error.exit()

    def process(self):
        arg = 0
        while arg < len(self.args):
            a = self.args[arg]
            if a == '-?':
                self.help()
            elif a.startswith('--'):
                los = a.split('=')
                lo = los[0]
                if lo in self.long_opts:
                    long_opt = self.long_opts[lo]
                    if len(los) == 1:
                        if long_opt[2]:
                            if arg == len(self.args) - 1:
                                raise error.general('option requires a parameter: %s' % (lo))
                            arg += 1
                            value = self.args[arg]
                        else:
                            value = None
                    else:
                        value = '='.join(los[1:])
                    long_opt[1](lo, long_opt[0], value)
            else:
                self.opts['params'].append(a)
            arg += 1

    def post_process(self):
        # Handle the log first.
        log.default = log.log(self.logfiles())
        if self.trace():
            log.tracing = True
        if self.quiet():
            log.quiet = True
        # Handle the jobs for make
        if '_ncpus' not in self.defaults:
            raise error.general('host number of CPUs not set')
        ncpus = self.jobs(self.defaults['_ncpus'])
        if ncpus > 1:
            self.defaults['_smp_mflags'] = '-j %d' % (ncpus)
        else:
            self.defaults['_smp_mflags'] = self.defaults['nil']
        # Load user macro files
        um = self.user_macros()
        if um:
            checked = path.exists(um)
            if False in checked:
                raise error.general('macro file not found: %s' % (um[checked.index(False)]))
            for m in um:
                self.defaults.load(m)
        # Check if the user has a private set of macros to load
        if 'RSB_MACROS' in os.environ:
            if path.exists(os.environ['RSB_MACROS']):
                self.defaults.load(os.environ['RSB_MACROS'])
        if 'HOME' in os.environ:
            rsb_macros = path.join(os.environ['HOME'], '.rsb_macros')
            if path.exists(rsb_macros):
                self.defaults.load(rsb_macros)

    def local_git(self):
        repo = git.repo(self.defaults.expand('%{_rtdir}'), self)
        if repo.valid():
            repo_valid = '1'
            repo_head = repo.head()
            repo_clean = not repo.dirty()
            repo_id = repo_head
            if not repo_clean:
                repo_id += '-modified'
            repo_mail = repo.email()
        else:
            repo_valid = '0'
            repo_head = '%{nil}'
            repo_clean = '%{nil}'
            repo_id = 'no-repo'
            repo_mail = None
        self.defaults['_local_git_valid'] = repo_valid
        self.defaults['_local_git_head']  = repo_head
        self.defaults['_local_git_clean'] = str(repo_clean)
        self.defaults['_local_git_id']    = repo_id
        if repo_mail is not None:
            self.defaults['_localgit_mail'] = repo_mail

    def command(self):
        return path.join(self.command_path, self.command_name)

    def force(self):
        return self.opts['force'] != '0'

    def dry_run(self):
        return self.opts['dry-run'] != '0'

    def set_dry_run(self):
        self.opts['dry-run'] = '1'

    def quiet(self):
        return self.opts['quiet'] != '0'

    def trace(self):
        return self.opts['trace'] != '0'

    def warn_all(self):
        return self.opts['warn-all'] != '0'

    def keep_going(self):
        return self.opts['keep-going'] != '0'

    def no_clean(self):
        return self.opts['no-clean'] != '0'

    def always_clean(self):
        return self.opts['always-clean'] != '0'

    def no_install(self):
        return self.opts['no-install'] != '0'

    def user_macros(self):
        #
        # Return something even if it does not exist.
        #
        if self.opts['macros'] is None:
            return None
        um = []
        configs = self.defaults.expand('%{_configdir}').split(':')
        for m in self.opts['macros'].split(','):
            if path.exists(m):
                um += [m]
            else:
                # Get the expanded config macros then check them.
                cm = path.expand(m, configs)
                ccm = path.exists(cm)
                if True in ccm:
                    # Pick the first found
                    um += [cm[ccm.index(True)]]
                else:
                    um += [m]
        return um if len(um) else None

    def jobs(self, cpus):
        try:
            cpus = int(cpus)
        except:
            raise error.general('invalid host cpu value')
        opt_jobs = self.opts['jobs']
        if opt_jobs == 'default':
            _jobs = self.defaults.get_value('jobs')
            if _jobs is not None:
                if _jobs == 'none':
                    cpus = 0
                elif _jobs == 'max':
                    pass
                elif _jobs == 'half':
                    cpus = cpus / 2
                else:
                    try:
                        cpus = int(_jobs)
                    except:
                        raise error.general('invalid %%{jobs} value: %s' % (_jobs))
            else:
                opt_jobs = 'max'
        if opt_jobs != 'default':
            if opt_jobs == 'none':
                cpus = 0
            elif opt_jobs == 'max':
                pass
            elif opt_jobs == 'half':
                cpus = cpus / 2
            else:
                ok = False
                try:
                    i = int(opt_jobs)
                    cpus = i
                    ok = True
                except:
                    pass
                if not ok:
                    try:
                        f = float(opt_jobs)
                        cpus = f * cpus
                        ok = True
                    except:
                        pass
                    if not ok:
                        raise error.internal('bad jobs option: %s' % (opt_jobs))
        if cpus <= 0:
            cpu = 1
        return cpus

    def params(self):
        return self.opts['params']

    def get_args(self):
        for arg in self.args:
            yield arg

    def find_arg(self, arg):
        if self.optargs is None or arg not in self.optargs:
            raise error.internal('bad arg: %s' % (arg))
        for a in self.args:
            sa = a.split('=')
            if sa[0].startswith(arg):
                return sa
        return None

    def logfiles(self):
        if 'log' in self.opts and self.opts['log'] is not None:
            log = self.opts['log'].split(',')
        elif self.log_default is None:
            log = ['stdout']
        else:
            log = self.log_default
        return log

    def urls(self):
        if self.opts['url'] is not None:
            return self.opts['url'].split(',')
        return None

    def log_info(self):
        log.output(' Command Line: %s' % (' '.join(self.argv)))
        log.output(' Python: %s' % (sys.version.replace('\n', '')))

def load(opts):
    """
    Copy the defaults, get the host specific values and merge them overriding
    any matching defaults, then create an options object to handle the command
    line merging in any command line overrides. Finally post process the
    command line.
    """

    global host_windows

    overrides = None
    if os.name == 'nt':
        try:
            import windows
            overrides = windows.load()
            host_windows = True
        except:
            raise error.general('failed to load Windows host support')
    elif os.name == 'posix':
        uname = os.uname()
        try:
            if uname[0].startswith('CYGWIN_NT'):
                import windows
                overrides = windows.load()
            elif uname[0] == 'Darwin':
                import darwin
                overrides = darwin.load()
            elif uname[0] == 'FreeBSD':
                import freebsd
                overrides = freebsd.load()
            elif uname[0] == 'NetBSD':
                import netbsd
                overrides = netbsd.load()
            elif uname[0] == 'Linux':
                import linux
                overrides = linux.load()
        except:
            raise error.general('failed to load %s host support' % (uname[0]))
    else:
        raise error.general('unsupported host type; please add')
    if overrides is None:
        raise error.general('no hosts defaults found; please add')
    for k in overrides:
        opts.defaults[k] = overrides[k]

    opts.local_git()
    opts.process()
    opts.post_process()

def run(args):
    try:
        long_opts = {
            # key              macro         handler   param  defs   init
            '--test-path'  : ('_test_path',  'path',   True,  None,  False),
            '--test-jobs'  : ('_test_jobs',  'jobs',   True,  'max', True),
            '--test-bool'  : ('_test_bool',  'bool',   False, '0',   True)
        }
        opts = command_line(base_path = '.',
                            argv = args,
                            optargs = None,
                            defaults = macros.macros(),
                            long_opts = long_opts,
                            command_path = '.')
        load(opts)
        log.notice('RTEMS Tools Project - Defaults, v%s' % (version.str()))
        opts.log_info()
        log.notice('Options:')
        log.notice(str(opts))
        log.notice('Defaults:')
        log.notice(str(opts.defaults))
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    except error.exit, eerr:
        pass
    except KeyboardInterrupt:
        _notice(opts, 'abort: user terminated')
        sys.exit(1)
    sys.exit(0)

if __name__ == '__main__':
    run(sys.argv)
