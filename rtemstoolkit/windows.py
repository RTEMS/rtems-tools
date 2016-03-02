#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2016 Chris Johns (chrisj@rtems.org)
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
# Windows specific support and overrides.
#

import pprint
import os

#
# Support to handle use in a package and as a unit test.
# If there is a better way to let us know.
#
try:
    from . import error
    from . import execute
except (ValueError, SystemError):
    import error
    import execute

def load():
    # Default to the native Windows Python.
    uname = 'win32'
    system = 'mingw32'
    if 'HOSTTYPE' in os.environ:
        hosttype = os.environ['HOSTTYPE']
    else:
        hosttype = 'i686'
    host_triple = hosttype + '-pc-' + system
    build_triple = hosttype + '-pc-' + system

    # See if this is actually Cygwin Python
    if os.name == 'posix':
        try:
            uname = os.uname()
            hosttype = uname[4]
            uname = uname[0]
            if uname.startswith('CYGWIN'):
                if uname.endswith('WOW64'):
                    uname = 'cygwin'
                    build_triple = hosttype + '-pc-' + uname
                    hosttype = 'x86_64'
                    host_triple = hosttype + '-w64-' + system
                else:
                    raise error.general('invalid uname for Windows')
            else:
                raise error.general('invalid POSIX python')
        except:
            pass

    if 'NUMBER_OF_PROCESSORS' in os.environ:
        ncpus = os.environ['NUMBER_OF_PROCESSORS']
    else:
        ncpus = '1'

    defines = {
        '_ncpus':         ('none',    'none',     ncpus),
        '_os':            ('none',    'none',     'win32'),
        '_build':         ('triplet', 'required', build_triple),
        '_build_vendor':  ('none',    'none',     'microsoft'),
        '_build_os':      ('none',    'none',     'win32'),
        '_build_cpu':     ('none',    'none',     hosttype),
        '_build_alias':   ('none',    'none',     '%{nil}'),
        '_build_arch':    ('none',    'none',     hosttype),
        '_host':          ('triplet', 'required', host_triple),
        '_host_vendor':   ('none',    'none',     'microsoft'),
        '_host_os':       ('none',    'none',     'win32'),
        '_host_cpu':      ('none',    'none',     hosttype),
        '_host_alias':    ('none',    'none',     '%{nil}'),
        '_host_arch':     ('none',    'none',     hosttype),
        '_usr':           ('dir',     'optional', '/opt/local'),
        '_var':           ('dir',     'optional', '/opt/local/var'),
        '__bash':         ('exe',     'required', 'bash'),
        '__bzip2':        ('exe',     'required', 'bzip2'),
        '__bison':        ('exe',     'required', 'bison'),
        '__cat':          ('exe',     'required', 'cat'),
        '__cc':           ('exe',     'required', 'gcc'),
        '__chgrp':        ('exe',     'required', 'chgrp'),
        '__chmod':        ('exe',     'required', 'chmod'),
        '__chown':        ('exe',     'required', 'chown'),
        '__cp':           ('exe',     'required', 'cp'),
        '__cvs':          ('exe',     'required', 'cvs'),
        '__cxx':          ('exe',     'required', 'g++'),
        '__flex':         ('exe',     'required', 'flex'),
        '__git':          ('exe',     'required', 'git'),
        '__grep':         ('exe',     'required', 'grep'),
        '__gzip':         ('exe',     'required', 'gzip'),
        '__id':           ('exe',     'required', 'id'),
        '__install':      ('exe',     'required', 'install'),
        '__install_info': ('exe',     'required', 'install-info'),
        '__ld':           ('exe',     'required', 'ld'),
        '__ldconfig':     ('exe',     'none',     ''),
        '__makeinfo':     ('exe',     'required', 'makeinfo'),
        '__mkdir':        ('exe',     'required', 'mkdir'),
        '__mv':           ('exe',     'required', 'mv'),
        '__nm':           ('exe',     'required', 'nm'),
        '__nm':           ('exe',     'required', 'nm'),
        '__objcopy':      ('exe',     'required', 'objcopy'),
        '__objdump':      ('exe',     'required', 'objdump'),
        '__patch':        ('exe',     'required', 'patch'),
        '__patch_bin':    ('exe',     'required', 'patch'),
        '__rm':           ('exe',     'required', 'rm'),
        '__sed':          ('exe',     'required', 'sed'),
        '__sh':           ('exe',     'required', 'sh'),
        '__tar':          ('exe',     'required', 'bsdtar'),
        '__touch':        ('exe',     'required', 'touch'),
        '__unzip':        ('exe',     'required', 'unzip'),
        '__xz':           ('exe',     'required', 'xz'),
        '_buildshell':    ('exe',     'required', '%{__sh}'),
        '___setup_shell': ('exe',     'required', '%{__sh}')
        }
    return defines

if __name__ == '__main__':
    pprint.pprint(load())
