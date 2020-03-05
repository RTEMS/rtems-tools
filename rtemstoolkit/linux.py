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
import platform

from rtemstoolkit import execute
from rtemstoolkit import path

def cpus():
    processors = '/bin/grep processor /proc/cpuinfo'
    e = execute.capture_execution()
    exit_code, proc, output = e.shell(processors)
    ncpus = 0
    if exit_code == 0:
        try:
            for l in output.split('\n'):
                count = l.split(':')[1].strip()
                if int(count) > ncpus:
                    ncpus = int(count)
        except:
            pass
    return ncpus + 1

def overrides():
    uname = os.uname()
    smp_mflags = ''
    ncpus = '%d' % cpus()
    if uname[4].startswith('arm'):
        cpu = 'arm'
    else:
        cpu = uname[4]

    defines = {
        '_ncpus':         ('none',    'none',     ncpus),
        '_os':            ('none',    'none',     'linux'),
        '_host':          ('triplet', 'required', cpu + '-linux-gnu'),
        '_host_vendor':   ('none',    'none',     'gnu'),
        '_host_os':       ('none',    'none',     'linux'),
        '_host_cpu':      ('none',    'none',     cpu),
        '_host_alias':    ('none',    'none',     '%{nil}'),
        '_host_arch':     ('none',    'none',     cpu),
        '_usr':           ('dir',     'required', '/usr'),
        '_var':           ('dir',     'required', '/var'),
        '__bzip2':        ('exe',     'required', '/usr/bin/bzip2'),
        '__gzip':         ('exe',     'required', '/bin/gzip'),
        '__tar':          ('exe',     'required', '/bin/tar')
        }

    # Works for LSB distros
    try:
        distro = platform.dist()[0]
        distro_ver = float(platform.dist()[1])
    except (AttributeError, ValueError):
        # Non LSB distro found, use failover"
        distro = ''
        pass

    # Non LSB - fail over to issue
    if distro == '':
        try:
            issue = open('/etc/issue').read()
            distro = issue.split(' ')[0]
            distro_ver = float(issue.split(' ')[2])
        except:
            pass

    # Manage distro aliases
    if distro in ['centos']:
        distro = 'redhat'
    elif distro in ['fedora']:
        if distro_ver < 17:
            distro = 'redhat'
    elif distro in ['centos', 'fedora']:
        distro = 'redhat'
    elif distro in ['Ubuntu', 'ubuntu']:
        distro = 'debian'
    elif distro in ['Arch']:
        distro = 'arch'
    elif distro in ['SuSE']:
        distro = 'suse'

    variations = {
        'debian' : { '__bzip2':        ('exe',     'required', '/bin/bzip2'),
                     '__chgrp':        ('exe',     'required', '/bin/chgrp'),
                     '__chown':        ('exe',     'required', '/bin/chown'),
                     '__grep':         ('exe',     'required', '/bin/grep'),
                     '__sed':          ('exe',     'required', '/bin/sed') },
        'redhat' : { '__bzip2':        ('exe',     'required', '/bin/bzip2'),
                     '__chgrp':        ('exe',     'required', '/bin/chgrp'),
                     '__chown':        ('exe',     'required', '/bin/chown'),
                     '__install_info': ('exe',     'required', '/sbin/install-info'),
                     '__grep':         ('exe',     'required', '/bin/grep'),
                     '__sed':          ('exe',     'required', '/bin/sed'),
                     '__touch':        ('exe',     'required', '/bin/touch') },
        'fedora' : { '__chown':        ('exe',     'required', '/usr/bin/chown'),
                     '__install_info': ('exe',     'required', '/usr/sbin/install-info') },
        'arch'   : { '__gzip':         ('exe',     'required', '/usr/bin/gzip'),
                     '__chown':        ('exe',     'required', '/usr/bin/chown') },
        'suse'   : { '__chgrp':        ('exe',     'required', '/usr/bin/chgrp'),
                     '__chown':        ('exe',     'required', '/usr/sbin/chown') },
        }

    if distro in variations:
        for v in variations[distro]:
            if path.exists(variations[distro][v][2]):
                defines[v] = variations[distro][v]

    defines['_build']        = defines['_host']
    defines['_build_vendor'] = defines['_host_vendor']
    defines['_build_os']     = defines['_host_os']
    defines['_build_cpu']    = defines['_host_cpu']
    defines['_build_alias']  = defines['_host_alias']
    defines['_build_arch']   = defines['_host_arch']

    return defines

if __name__ == '__main__':
    import pprint
    pprint.pprint(cpus())
    pprint.pprint(overrides())
