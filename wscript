#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2014-2018 Chris Johns (chrisj@rtems.org)
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

import os.path

import wafwindows

subdirs = ['rtemstoolkit',
           'config',
           'linkers',
           'misc',
           'tester',
           'tools/gdb/python',
           'trace']

def get_version(ctx):
    #
    # The file config/rtems-versin.ini contains the version.
    #
    from rtemstoolkit import version as rtemsversion
    try:
        version = rtemsversion.version()
        revision = rtemsversion.revision()
    except Exception as e:
        ctx.fatal('invalid version file: %s' % (e))
    release = '%s.%s' % (version, revision)
    return version, release

def recurse(ctx):
    for sd in subdirs:
        ctx.recurse(sd)

def options(ctx):
    ctx.add_option('--c-opts',
                   default = '-O2',
                   dest='c_opts',
                   help = 'Set build options, default: -O2.')
    ctx.add_option('--host',
                   default = 'native',
                   dest='host',
                   help = 'Set host to build for, default: none.')
    recurse(ctx)

def init(ctx):
    wafwindows.set_compilers()
    try:
        import waflib.Options
        import waflib.ConfigSet
        env = waflib.ConfigSet.ConfigSet()
        env.load(waflib.Options.lockfile)
        check_options(ctx, env.options['host'])
        recurse(ctx)
    except:
        pass

def shutdown(ctx):
    pass

def configure(ctx):
    try:
        ctx.load("doxygen", tooldir = 'waf-tools')
    except:
        pass
    ctx.env.RTEMS_VERSION, ctx.env.RTEMS_RELEASE = get_version(ctx)
    ctx.start_msg('Version')
    ctx.end_msg('%s (%s)' % (ctx.env.RTEMS_RELEASE, ctx.env.RTEMS_VERSION))
    ctx.env.C_OPTS = ctx.options.c_opts.split(',')
    check_options(ctx, ctx.options.host)
    #
    # Common Python check.
    #
    ctx.load('python')
    ctx.check_python_version((2,6,6))
    #
    # Find which versions of python are installed for testing.
    #
    ctx.find_program('python', mandatory = False)
    ctx.find_program('python2', mandatory = False)
    ctx.find_program('python3', mandatory = False)
    #
    # Installing the PYO,PYC seems broken on 1.8.19. The path is wrong.
    #
    ctx.env.PYO = 0
    ctx.env.PYC = 0
    recurse(ctx)

def build(ctx):
    if os.path.exists('VERSION'):
        ctx.install_files('${PREFIX}/share/rtems/rtemstoolkit', ['VERSION'])
    recurse(ctx)
    if ctx.cmd == 'test':
        rtemstoolkit_tests(ctx)

def install(ctx):
    recurse(ctx)

def clean(ctx):
    recurse(ctx)

def rebuild(ctx):
    import waflib.Options
    waflib.Options.commands.extend(['clean', 'build'])

def check_options(ctx, host):
    if host in ['mingw32', 'x86_64-w64-mingw32', 'i686-w64-mingw32']:
        ctx.env.HOST = host
        ctx.env.CC = '%s-gcc' % (host)
        ctx.env.CXX = '%s-g++' % (host)
        ctx.env.AR = '%s-ar' % (host)
        ctx.env.PYTHON = 'python'
    elif host != 'native':
        ctx.fatal('unknown host: %s' % (host));

#
# Custom commands
#
import waflib

class test(waflib.Build.BuildContext):
    fun = 'build'
    cmd = 'test'

class doxy(waflib.Build.BuildContext):
    fun = 'build'
    cmd = 'doxy'

#
# RTEMS Toolkit Tests.
#
# Run the tests from the top directory so they are run as python modules.
#
def rtemstoolkit_tests(ctx):
    log = ctx.path.find_or_declare('tests.log')
    ctx.logger = waflib.Logs.make_logger(log.abspath(), 'build')
    failures = False
    from rtemstoolkit import all as toolkit_tests
    from rtemstoolkit import args as toolkit_test_args
    for tt in toolkit_tests:
        for py in ['', '2', '3']:
            PY = 'PYTHON%s' % (py)
            if PY in ctx.env:
                test = 'rtemstoolkit.%s' % (tt)
                ctx.start_msg('Test python%s %s' % (py, test))
                cmd = '%s -m %s' % (ctx.env[PY][0], test)
                if tt in toolkit_test_args:
                    cmd += ' ' + ' '.join(toolkit_test_args[tt])
                ctx.to_log('test command: ' + cmd)
                try:
                    (out, err) = ctx.cmd_and_log(cmd,
                                                 output = waflib.Context.BOTH,
                                                 quiet = waflib.Context.BOTH)
                    ctx.to_log(out)
                    ctx.to_log(err)
                    ctx.end_msg('pass')
                except waflib.Errors.WafError as e:
                    failures = True
                    ctx.to_log(e.stdout)
                    ctx.to_log(e.stderr)
                    ctx.end_msg('fail', color = 'RED')
    if failures:
        ctx.fatal('Test failures')
