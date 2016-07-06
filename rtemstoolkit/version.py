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
# To release RTEMS Tools create a git archive and then add a suitable VERSION
# file to the top directory.
#

from __future__ import print_function

import sys

#
# Support to handle use in a package and as a unit test.
# If there is a better way to let us know.
#
try:
    from . import error
    from . import git
    from . import path
except (ValueError, SystemError):
    import error
    import git
    import path

#
# Default to an internal string.
#
_version = '4.12'
_revision = 'not_released'
_version_str = '%s.%s' % (_version, _revision)
_released = False
_git = False

def _at():
    return path.dirname(__file__)

def _load_released_version():
    global _released
    global _version_str
    at = _at()
    for ver in [at, path.join(at, '..')]:
        if path.exists(path.join(ver, 'VERSION')):
            try:
                import configparser
            except ImportError:
                import ConfigParser as configparser
            v = configparser.SafeConfigParser()
            v.read(path.join(ver, 'VERSION'))
            _version_str = v.get('version', 'release')
            _released = True
    return _released

def _load_git_version():
    global _git
    global _version_str
    repo = git.repo(_at())
    if repo.valid():
        head = repo.head()
        if repo.dirty():
            modified = ' modified'
        else:
            modified = ''
        _version_str = '%s (%s%s)' % (_version, head[0:12], modified)
        _git = True
    return _git

def released():
    return _load_released_version()

def version_control():
    return _load_git_version()

def str():
    if not _released and not _git:
        if not _load_released_version():
            _load_git_version()
    return _version_str

def version():
    return _version

if __name__ == '__main__':
    print('Version: %s' % (str()))
