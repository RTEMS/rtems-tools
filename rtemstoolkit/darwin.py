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
# This code is based on what ever doco about spec files I could find and
# RTEMS project's spec files.
#

import os

import execute

def load():
    uname = os.uname()
    sysctl = '/usr/sbin/sysctl '
    e = execute.capture_execution()
    exit_code, proc, output = e.shell(sysctl + 'hw.ncpu')
    if exit_code == 0:
        ncpus = output.split(' ')[1].strip()
    else:
        ncpus = '1'
    defines = {
        '_ncpus':         ('none',    'none',     ncpus),
        '_os':            ('none',    'none',     'darwin'),
        '_host':          ('triplet', 'required', uname[4] + '-apple-darwin' + uname[2]),
        '_host_vendor':   ('none',    'none',     'apple'),
        '_host_os':       ('none',    'none',     'darwin'),
        '_host_cpu':      ('none',    'none',     uname[4]),
        '_host_alias':    ('none',    'none',     '%{nil}'),
        '_host_arch':     ('none',    'none',     uname[4]),
        '_host_prefix':   ('dir',     'optional', '%{_usr}'),
        '_usr':           ('dir',     'optional', '/usr/local'),
        '_var':           ('dir',     'optional', '/usr/local/var'),
        '__ldconfig':     ('exe',     'none',     ''),
        '__cvs':          ('exe',     'required', 'cvs'),
        '__xz':           ('exe',     'required', '%{_usr}/bin/xz'),
        'with_zlib':      ('none',    'none',     '--with-zlib=no'),
        '_forced_static': ('none',    'none',     '')
        }

    defines['_build']        = defines['_host']
    defines['_build_vendor'] = defines['_host_vendor']
    defines['_build_os']     = defines['_host_os']
    defines['_build_cpu']    = defines['_host_cpu']
    defines['_build_alias']  = defines['_host_alias']
    defines['_build_arch']   = defines['_host_arch']

    return defines

if __name__ == '__main__':
    import pprint
    pprint.pprint(load())
