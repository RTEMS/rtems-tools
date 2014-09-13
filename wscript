#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2014 Chris Johns (chrisj@rtems.org)
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

subdirs = ['rtemstoolkit',
           'linkers',
           'tester',
           'tools/gdb/python']

def recurse(ctx):
    for sd in subdirs:
        ctx.recurse(sd)

def options(ctx):
    ctx.add_option('--rtems-version',
                   default = '4.11',
                   dest='rtems_version',
                   help = 'Set the RTEMS version')
    ctx.add_option('--c-opts',
                   default = '-O2',
                   dest='c_opts',
                   help = 'Set build options, default: -O2.')
    recurse(ctx)

def configure(ctx):
    try:
        ctx.load("doxygen", tooldir = 'waf-tools')
    except:
        pass
    ctx.env.C_OPTS = ctx.options.c_opts.split(',')
    ctx.env.RTEMS_VERSION = ctx.options.rtems_version
    recurse(ctx)

def build(ctx):
    recurse(ctx)

def install(ctx):
    recurse(ctx)

def clean(ctx):
    recurse(ctx)

def rebuild(ctx):
    import waflib.Options
    waflib.Options.commands.extend(['clean', 'build'])

#
# The doxy command.
#
from waflib import Build
class doxy(Build.BuildContext):
    fun = 'build'
    cmd = 'doxy'
