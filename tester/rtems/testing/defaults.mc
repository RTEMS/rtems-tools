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
# All paths in defaults must be Unix format. Do not store any Windows format
# paths in the defaults.
#
# Every entry must describe the type of checking a host must pass.
#
# Records:
#  key: type, attribute, value
#   type     : none, dir, exe, triplet
#   attribute: none, required, optional
#   value    : 'single line', '''multi line'''
#

#
# Global defaults
#
[global]

# Nothing
nil:                 none,    none,     ''

# Paths
_topdir:             dir,     required, '%{_cwd}'
_docdir:             dir,     none,     '%{_defaultdocdir}'
_tmppath:            dir,     none,     '%{_topdir}/build/tmp'
_tmproot:            dir,     none,     '%{_tmppath}/rt/%{_bset}'
_datadir:            dir,     none,     '%{_prefix}/share'
_defaultdocdir:      dir,     none,     '%{_prefix}/share/doc'
_exeext:             none,    none,     ''
_exec_prefix:        dir,     none,     '%{_prefix}'
_bindir:             dir,     none,     '%{_exec_prefix}/bin'
_sbindir:            dir,     none,     '%{_exec_prefix}/sbin'
_libexecdir:         dir,     none,     '%{_exec_prefix}/libexec'
_datarootdir:        dir,     none,     '%{_prefix}/share'
_datadir:            dir,     none,     '%{_datarootdir}'
_sysconfdir:         dir,     none,     '%{_prefix}/etc'
_sharedstatedir:     dir,     none,     '%{_prefix}/com'
_localstatedir:      dir,     none,     '%{prefix}/var'
_includedir:         dir,     none,     '%{_prefix}/include'
_lib:                dir,     none,     'lib'
_libdir:             dir,     none,     '%{_exec_prefix}/%{_lib}'
_libexecdir:         dir,     none,     '%{_exec_prefix}/libexec'
_mandir:             dir,     none,     '%{_datarootdir}/man'
_infodir:            dir,     none,     '%{_datarootdir}/info'
_localedir:          dir,     none,     '%{_datarootdir}/locale'
_localedir:          dir,     none,     '%{_datadir}/locale'
_localstatedir:      dir,     none,     '%{_prefix}/var'
_usr:                dir,     none,     '/usr/local'
_usrsrc:             dir,     none,     '%{_usr}/src'
_var:                dir,     none,     '/usr/local/var'
_varrun:             dir,     none,     '%{_var}/run'

# Defaults, override in platform specific modules.
__arch_install_post: exe,     none,     '%{nil}'
__bash:              exe,     optional, '/bin/bash'
__bzip2:             exe,     required, '/usr/bin/bzip2'
__cat:               exe,     required, '/bin/cat'
__chgrp:             exe,     required, '/usr/bin/chgrp'
__chmod:             exe,     required, '/bin/chmod'
__chown:             exe,     required, '/usr/sbin/chown'
__cp:                exe,     required, '/bin/cp'
__git:               exe,     required, '/usr/bin/git'
__grep:              exe,     required, '/usr/bin/grep'
__gzip:              exe,     required, '/usr/bin/gzip'
__id:                exe,     required, '/usr/bin/id'
__id_u:              exe,     none,     '%{__id} -u'
__ln_s:              exe,     none,     'ln -s'
__make:              exe,     required, 'make'
__mkdir:             exe,     required, '/bin/mkdir'
__mkdir_p:           exe,     none,     '/bin/mkdir -p'
__mv:                exe,     required, '/bin/mv'
__patch_bin:         exe,     required, '/usr/bin/patch'
__patch_opts:        none,    none,     '%{nil}'
__patch:             exe,     none,     '%{__patch_bin} %{__patch_opts}'
__svn:               exe,     optional, '/usr/bin/svn'
__rm:                exe,     required, '/bin/rm'
__rmfile:            exe,     none,     '%{__rm} -f'
__rmdir:             exe,     none,     '%{__rm} -rf'
__sed:               exe,     required, '/usr/bin/sed'
__sh:                exe,     required, '/bin/sh'
__tar:               exe,     required, '/usr/bin/tar'
__tar_extract:       exe,     none,     '%{__tar} -xvvf'
__touch:             exe,     required, '/usr/bin/touch'
__unzip:             exe,     required, '/usr/bin/unzip'
__xz:                exe,     required, '/usr/bin/xz'

# Default settings
_target:             none,    none,     '%{nil}'

# Paths
_rtbase:             none,    none,     '%{_rtdir}'
_rttesting:          none,    none,     '%{_rtbase}/rtems/testing'
_configdir:          none,    none,     '%{_rtbase}/config:%{_rttesting}'

# Include the testing macros.
%include %{_rttesting}/testing.mc
