#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2015 Chris Johns (chrisj@rtems.org)
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

import os

windows = os.name == 'nt'
_os_sep = os.sep

def set_compilers():
    if windows:
        #
        # MSVC is not supported so ignore it on Windows.
        #
        import waflib.Tools.compiler_c
        waflib.Tools.compiler_c.c_compiler['win32'] = ['gcc', 'clang']
        import waflib.Tools.compiler_cxx
        waflib.Tools.compiler_cxx.cxx_compiler['win32'] = ['g++', 'clang++']

def set_os_sep():
    if windows:
        #
        # MSYS2 changes the standard definition of os.sep to '/'. Change it
        # back because the change is wrong as it breaks the interface to parts
        # of Windows, eg cmd.exe, which require '\\'.
        #
        if os.sep != '\\':
            os.sep = '\\'

def reset_os_sep():
    if windows:
        if os.sep != _os_sep:
            os.sep = _os_sep
