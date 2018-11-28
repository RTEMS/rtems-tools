#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2017 Chris Johns (chrisj@rtems.org)
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
# Host specifics.
#

from __future__ import print_function

import os

from rtemstoolkit import error

is_windows = False
platform = None
name = None

def _load():

    global is_windows
    global platform
    global name

    if os.name == 'nt':
        name = 'windows'
        is_windows = True
    elif os.name == 'posix':
        uname = os.uname()
        if uname[0].startswith('MINGW64_NT') or uname[0].startswith('CYGWIN_NT'):
            name = 'windows'
        elif uname[0] == 'Darwin':
            name = 'darwin'
        elif uname[0] == 'FreeBSD':
            name = 'freebsd'
        elif uname[0] == 'NetBSD':
            name = 'netbsd'
        elif uname[0] == 'Linux':
            name = 'linux'
        elif uname[0] == 'SunOS':
            name = 'solaris'

    if name is None:
        raise error.general('unsupported host type; please add')

    #try:
    #    try:
    #        platform = __import__(name, globals(), locals(), ['.'])
    #    except:
    #        platform = __import__(name, globals(), locals())
    #except:
    #    raise error.general('failed to load %s host support' % (name))

    platform = __import__(name, globals(), locals(), ['.', ''], 1)

    if platform is None:
        raise error.general('failed to load %s host support' % (name))

def cpus():
    _load()
    return platform.cpus()

def overrides():
    _load()
    return platform.overrides()

def label(mode = 'all'):
    import platform
    if mode == 'system':
        return platform.system()
    compact = platform.platform(aliased = True)
    if mode == 'compact':
        return compact
    extended = ' '.join(platform.uname())
    if mode == 'extended':
        return extended
    if mode == 'all':
        return '%s (%s)' % (compact, extended)
    raise error.general('invalid platform mode: %s' % (mode))

if __name__ == '__main__':
    import pprint
    print('Python\'s OS name: %s' % (os.name))
    _load()
    print('Name      : %s' % (name))
    if is_windows:
        status = 'Yes'
    else:
        status = 'No'
    print('Windows   : %s' % (status))
    print('CPUs      : %d' % (cpus()))
    print('Overrides :')
    pprint.pprint(overrides())
