#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2017 Chris Johns (chrisj@rtems.org)
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

from rtemstoolkit import check
from rtemstoolkit import execute

def cpus():
    sysctl = '/sbin/sysctl '
    e = execute.capture_execution()
    exit_code, proc, output = e.shell(sysctl + 'hw.ncpu')
    if exit_code == 0:
        ncpus = int(output.split(' ')[1].strip())
    else:
        ncpus = 1
    return ncpus

def overrides():
    uname = os.uname()
    ncpus = '%d' % (cpus())
    if uname[4] == 'amd64':
        cpu = 'x86_64'
    else:
        cpu = uname[4]
    version = uname[2]
    if version.find('-') > 0:
        version = version.split('-')[0]
    defines = {
        '_ncpus':        ('none',    'none',     ncpus),
        '_os':           ('none',    'none',     'freebsd'),
        '_host':         ('triplet', 'required', cpu + '-freebsd' + version),
        '_host_vendor':  ('none',    'none',     'pc'),
        '_host_os':      ('none',    'none',     'freebsd'),
        '_host_cpu':     ('none',    'none',     cpu),
        '_host_alias':   ('none',    'none',     '%{nil}'),
        '_host_arch':    ('none',    'none',     cpu),
        '_usr':          ('dir',     'required', '/usr/local'),
        '_var':          ('dir',     'optional', '/usr/local/var'),
        '__bash':        ('exe',     'optional', '/usr/local/bin/bash'),
        '__bison':       ('exe',     'required', '/usr/local/bin/bison'),
        '__git':         ('exe',     'required', '/usr/local/bin/git'),
        '__xz':          ('exe',     'optional', '/usr/bin/xz'),
        '__make':        ('exe',     'required', 'gmake'),
        '__patch_opts':  ('none',     'none',    '-E')
        }

    defines['_build']        = defines['_host']
    defines['_build_vendor'] = defines['_host_vendor']
    defines['_build_os']     = defines['_host_os']
    defines['_build_cpu']    = defines['_host_cpu']
    defines['_build_alias']  = defines['_host_alias']
    defines['_build_arch']   = defines['_host_arch']

    for gv in ['47', '48', '49']:
        gcc = '%s-portbld-freebsd%s-gcc%s' % (cpu, version, gv)
        if check.check_exe(gcc, gcc):
            defines['__cc'] = gcc
            break
    for gv in ['47', '48', '49']:
        gxx = '%s-portbld-freebsd%s-g++%s' % (cpu, version, gv)
        if check.check_exe(gxx, gxx):
            defines['__cxx'] = gxx
            break

    return defines
