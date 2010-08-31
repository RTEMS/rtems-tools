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
# Determine the defaults and load the specific file.
#

import glob
import pprint
import re
import os

import error
import execute

defaults = {
# Hack. Suspect a hidden platform or comand line thing
'_with_noarch_subpackages': '1',

# Paths
'_host_platform': '%{_host_cpu}-%{_host_vendor}-%{_host_os}%{?_gnu}',
'_build':         '%{_host}',
'_arch':          '%{_host_arch}',
'_topdir':        os.getcwd(),
'_srcrpmdir':     '%{_topdir}/SRPMS',
'_sourcedir':     '%{_topdir}/SOURCES', 
'_specdir':       '%{_topdir}/SPECS',
'_rpmdir':        '%{_topdir}/TARS',
'_builddir':      '%{_topdir}/BUILD/%{name}-%{version}-%{release}',
'_docdir':        '%{_defaultdocdir}',
'_usrlibrpm':     '%{_topdir}/RPMLIB',
'_tmppath':       '%{_topdir}/TMP',
'buildroot:':     '%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)',

# Not sure.
'_gnu':           '-gnu',

# Cloned stuff from an RPM insall
'___build_args': '-e',
'___build_cmd':  '%{?_sudo:%{_sudo} }%{?_remsh:%{_remsh} %{_remhost} }%{?_remsudo:%{_remsudo} }%{?_remchroot:%{_remchroot} %{_remroot} }%{___build_shell} %{___build_args}',
'___build_post': 'exit 0',
'___build_pre': '''# ___build_pre in defaults.py
RPM_SOURCE_DIR="%{_sourcedir}"
RPM_BUILD_DIR="%{_builddir}"
RPM_OPT_FLAGS="%{optflags}"
RPM_ARCH="%{_arch}"
RPM_OS="%{_os}"
export RPM_SOURCE_DIR RPM_BUILD_DIR RPM_OPT_FLAGS RPM_ARCH RPM_OS
RPM_DOC_DIR="%{_docdir}"
export RPM_DOC_DIR
RPM_PACKAGE_NAME="%{name}"
RPM_PACKAGE_VERSION="%{version}"
RPM_PACKAGE_RELEASE="%{release}"
export RPM_PACKAGE_NAME RPM_PACKAGE_VERSION RPM_PACKAGE_RELEASE
%{?buildroot:RPM_BUILD_ROOT="%{buildroot}"}
export RPM_BUILD_ROOT
%{?_javaclasspath:CLASSPATH="%{_javaclasspath}"; export CLASSPATH}
LANG=C
export LANG
unset DISPLAY || :
umask 022
cd "%{_builddir}"''',
'___build_shell': '%{?_buildshell:%{_buildshell}}%{!?_buildshell:/bin/sh}',
'___build_template': '''#!%{___build_shell}
%{___build_pre}
%{nil}''',
'___setup_shell':      '/bin/sh',
'__aclocal':           'aclocal',
'__ar':                'ar',
'__arch_install_post': '%{nil}',
'__as':                'as',
'__autoconf':          'autoconf',
'__autoheader':        'autoheader',
'__automake':          'automake',
'__awk':               'awk',
'__bash':              '/bin/bash',
'__bzip2':             '/usr/bin/bzip2',
'__cat':               '/bin/cat',
'__cc':                '/usr/bin/gcc',
'__check_files':       '%{_usrlibrpm}/check-files %{buildroot}',
'__chgrp':             '/usr/bin/chgrp',
'__chmod':             '/bin/chmod',
'__chown':             '/usr/sbin/chown',
'__cp':                '/bin/cp',
'__cpio':              '/usr/bin/cpio',
'__cpp':               '/usr/bin/gcc -E',
'__cxx':               '/usr/bin/g++',
'__grep':              '/usr/bin/grep',
'__gzip':              '/usr/bin/gzip',
'__id':                '/usr/bin/id',
'__id_u':              '%{__id} -u',
'__install':           '/usr/bin/install',
'__install_info':      '/usr/bin/install-info',
'__ld':                '/usr/bin/ld',
'__ldconfig':          '/sbin/ldconfig',
'__ln_s':              'ln -s',
'__make':              '/usr/bin/make',
'__mkdir':             '/bin/mkdir',
'__mkdir_p':           '/bin/mkdir -p',
'__mv':                '/bin/mv',
'__nm':                '/usr/bin/nm',
'__objcopy':           '%{_bindir}/objcopy',
'__objdump':           '%{_bindir}/objdump',
'__patch':             '/usr/bin/patch',
'__perl':              'perl',
'__perl_provides':     '%{_usrlibrpm}/perl.prov',
'__perl_requires':     '%{_usrlibrpm}/perl.req',
'__ranlib':            'ranlib',
'__remsh':             '%{__rsh}',
'__rm':                '/bin/rm',
'__rsh':               '/usr/bin/rsh',
'__sed':               '/usr/bin/sed',
'__setup_post':        '%{__chmod} -R a+rX,g-w,o-w .',
'__sh':                '/bin/sh',
'__tar':               '/usr/bin/tar',
'__tar_extract':       '%{__tar} -xvvf',
'__unzip':             '/usr/bin/unzip',
'_datadir':            '%{_prefix}/share',
'_defaultdocdir':      '%{_prefix}/share/doc',
'_exeext':             '',
'_exec_prefix':        '%{_prefix}',
'_lib':                'lib',
'_libdir':             '%{_exec_prefix}/%{_lib}',
'_libexecdir':         '%{_exec_prefix}/libexec',
'_localedir':          '%{_datadir}/locale',
'_localstatedir':      '%{_prefix}/var',
'_usr':                '/usr/local',
'_usrsrc':             '%{_usr}/src',
'_var':                '/usr/local/var',
'_varrun':             '%{_var}/run',
'configure': '''
CFLAGS="${CFLAGS:-%optflags}" ; export CFLAGS ; 
CXXFLAGS="${CXXFLAGS:-%optflags}" ; export CXXFLAGS ; 
FFLAGS="${FFLAGS:-%optflags}" ; export FFLAGS ; 
./configure --build=%{_build} --host=%{_host} \
      --target=%{_target_platform} \
      --program-prefix=%{?_program_prefix} \
      --prefix=%{_prefix} \
      --exec-prefix=%{_exec_prefix} \
      --bindir=%{_bindir} \
      --sbindir=%{_sbindir} \
      --sysconfdir=%{_sysconfdir} \
      --datadir=%{_datadir} \
      --includedir=%{_includedir} \
      --libdir=%{_libdir} \
      --libexecdir=%{_libexecdir} \
      --localstatedir=%{_localstatedir} \
      --sharedstatedir=%{_sharedstatedir} \
      --mandir=%{_mandir} \
      --infodir=%{_infodir}''',
'nil':                 ''
}

class command_line:
    """Process the command line in a common way across all SpecBuilder commands."""

    _defaults = { 'params'   : [],
                  'warn-all' : '0',
                  'quiet'    : '0',
                  'trace'    : '0',
                  'dry-run'  : '0',
                  'no-clean' : '0',
                  'no-smp'   : '0',
                  'rebuild'  : '0' }

    _long_opts = { '--prefix'     : '_prefix',
                   '--prefixbase' : '_prefixbase',
                   '--topdir'     : '_topdir',
                   '--specdir'    : '_specdir',
                   '--builddir'   : '_builddir',
                   '--sourcedir'  : '_sourcedir',
                   '--usrlibrpm'  : '_usrlibrpm',
                   '--tmppath'    : '_tmppath',
                   '--log'        : '_logfile',
                   '--url'        : '_url_base',
                   '--rtems'      : '_rtemssrc' }

    _long_true_opts = { '--trace'    : '_trace',
                        '--dry-run'  : '_dry_run',
                        '--warn-all' : '_warn_all',
                        '--no-clean' : '_no_clean',
                        '--no-smp'   : '_no_smp',
                        '--rebuild'  : '_rebuild' }

    _target_triplets = { '--host'   : '_host',
                         '--build'  : '_build',
                         '--target' : '_target' }

    def __init__(self, argv):
        self.command_path = os.path.dirname(argv[0])
        if len(self.command_path) == 0:
            self.command_path = '.'
        self.command_name = os.path.basename(argv[0])
        self.args = argv[1:]
        self.defaults = {}
        for to in command_line._long_true_opts:
            self.defaults[command_line._long_true_opts[to]] = '0'
        self._process()

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

    def _process(self):

        def _process_lopt(opt, arg, long_opts, args, values = False):
            for lo in long_opts:
                if values and opt.startswith(lo):
                    equals = opt.find('=')
                    if equals < 0:
                        if arg == len(args) - 1:
                            raise error.general('missing option value: ' + lo)
                        arg += 1
                        value = args[arg]
                    else:
                        value = opt[equals + 1:]
                    return lo, long_opts[lo], value, arg
                elif opt == lo:
                    return lo, long_opts[lo], True, arg
            return None, None, None, arg

        self.opts = command_line._defaults
        i = 0
        while i < len(self.args):
            a = self.args[i]
            if a.startswith('-'):
                if a.startswith('--'):
                    if a.startswith('--warn-all'):
                        self.opts['warn-all'] = True
                    else:
                        lo, macro, value, i = _process_lopt(a, i,
                                                            command_line._long_true_opts,
                                                            self.args)
                        if lo:
                            self.defaults[macro] = '1'
                            self.opts[lo[2:]] = '1'
                        else:
                            lo, macro, value, i = _process_lopt(a, i,
                                                                command_line._long_opts,
                                                                self.args, True)
                            if lo:
                                self.defaults[macro] = value
                                self.opts[lo[2:]] = value
                            else:
                                #
                                # The target triplet is 'cpu-vendor-os'.
                                #
                                lo, macro, value, i = _process_lopt(a, i,
                                                                    command_line._target_triplets,
                                                                    self.args, True)
                                if lo:
                                    #
                                    # This is a target triplet. Run it past config.sub to make
                                    # make sure it is ok.
                                    #
                                    e = execute.capture_execution()
                                    config_sub = os.path.join(self.command_path, 
                                                              'specbuilder', 'config.sub')
                                    exit_code, proc, output = e.shell(config_sub + ' ' + value)
                                    if exit_code == 0:
                                        value = output
                                    self.defaults[macro] = value
                                    self.opts[lo[2:]] = value
                                    _arch = macro + '_cpu'
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
                                    self.defaults[_arch] = _arch_value
                                    self.defaults[_vendor] = _vendor_value
                                    self.defaults[_os] = _os_value
                                if not lo:
                                    raise error.general('invalid argument: ' + a)
                else:
                    if a == '-n':
                        self.opts['dry-run'] = '1'
                    elif a == '-q':
                        self.opts['quiet'] = '1'
            else:
                self.opts['params'].append(a)
            i += 1

    def _post_process(self, _defaults):
        if self.no_smp():
            _defaults['_smp_mflags'] = _defaults['nil']
        return _defaults

    def expand(self, s, _defaults):
        """Simple basic expander of spec file macros."""
        mf = re.compile(r'%{[^}]+}')
        expanded = True
        while expanded:
            expanded = False
            for m in mf.findall(s):
                name = m[2:-1]
                if name in _defaults:
                    s = s.replace(m, _defaults[name])
                    expanded = True
                else:
                    raise error.general('cannot process default macro: ' + m)
        return s

    def command(self):
        return os.path.join(self.command_path, self.command_name)
        
    def dry_run(self):
        return self.opts['dry-run'] != '0'

    def quiet(self):
        return self.opts['quiet'] != '0'

    def trace(self):
        return self.opts['trace'] != '0' 

    def warn_all(self):
        return self.opts['warn-all'] != '0'

    def no_clean(self):
        return self.opts['no-clean'] != '0'

    def no_smp(self):
        return self.opts['no-smp'] != '0'

    def rebuild(self):
        return self.opts['rebuild'] != '0'

    def params(self):
        return self.opts['params']

    def get_spec_files(self, spec):
        if spec.find('*') >= 0 or spec.find('?'):
            specdir = os.path.dirname(spec)
            specbase = os.path.basename(spec)
            if len(specbase) == 0:
                specbase = '*'
            if len(specdir) == 0:
                specdir = self.expand(defaults['_specdir'], defaults)
            if not os.path.isdir(specdir):
                raise error.general('specdir is not a directory or does not exist: ' + specdir)
            files = glob.glob(os.path.join(specdir, specbase))
            specs = files
        else:
            specs = [spec]
        return specs

    def spec_files(self):
        specs = []
        for spec in self.opts['params']:
            specs.extend(self.get_spec_files(spec))
        return specs

    def logfiles(self):
        if 'log' in self.opts:
            return self.opts['log'].split(',')
        return ['stdout']

    def urls(self):
        if 'url' in self.opts:
            return self.opts['url'].split(',')
        return None

    def prefixbase(self):
        if 'prefixbase' in self.opts:
            return self.opts['prefixbase']
        return None

def load(args):
    """
    Copy the defaults, get the host specific values and merge
    them overriding any matching defaults, then create an options
    object to handle the command line merging in any command line
    overrides. Finally post process the command line.
    """
    d = defaults
    overrides = None
    uname = os.uname()
    if uname[0] == 'Darwin':
        import darwin
        overrides = darwin.load()
    elif uname[0] == 'Linux':
        import linux 
        overrides = linux.load()
    if overrides is None:
        raise error.general('no hosts defaults found; please add')
    for k in overrides:
        d[k] = overrides[k]
    import rtems
    overrides = rtems.load()
    if overrides is not None:
        for k in overrides:
            d[k] = overrides[k]
    o = command_line(args)
    for k in o.defaults:
        d[k] = o.defaults[k]
    d = o._post_process(d)
    return o, d

if __name__ == '__main__':
    import sys
    try:
        _opts, _defaults = load(args = sys.argv)
        print _opts
        pprint.pprint(_defaults)
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    sys.exit(0)

