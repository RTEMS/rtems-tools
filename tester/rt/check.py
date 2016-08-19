#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2016 Chris Johns (chrisj@rtems.org)
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

from __future__ import print_function

import argparse
import datetime
import operator
import os
import sys

try:
    import configparser
except:
    import ConfigParser as configparser

from rtemstoolkit import execute
from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import path
from rtemstoolkit import version

def rtems_version():
    return version.version()

class warnings_counter:

    def __init__(self, rtems):
        self.rtems = path.host(rtems)
        self.reset()

    def report(self):
        str = ''
        sw = sorted(self.warnings.items(), key = operator.itemgetter(1), reverse = True)
        for w in sw:
            str += ' %5d %s%s' % (w[1], w[0], os.linesep)
        return str

    def accumulate(self, total):
        for w in self.warnings:
            if w not in total.warnings:
                total.warnings[w] = self.warnings[w]
            else:
                total.warnings[w] += self.warnings[w]
        total.count += self.count

    def get(self):
        return self.count

    def reset(self):
        self.warnings = { }
        self.count = 0

    def output(self, text):
        for l in text.splitlines():
            if ' warning:' in l:
                self.count += 1
                ws = l.split(' ')
                if len(ws) > 0:
                    ws = ws[0].split(':')
                    w = path.abspath(ws[0])
                    w = w.replace(self.rtems, '')
                    if path.isabspath(w):
                        w = w[1:]
                    #
                    # Ignore compiler option warnings.
                    #
                    if len(ws) >= 3:
                        w = '%s:%s:%s' % (w, ws[1], ws[2])
                        if w not in self.warnings:
                            self.warnings[w] = 0
                        self.warnings[w] += 1
        log.output(text)

class results:

    def __init__(self):
        self.passes = []
        self.fails = []

    def add(self, good, arch, bsp, configure, warnings):
        if good:
            self.passes += [(arch, bsp, configure, warnings)]
        else:
            self.fails += [(arch, bsp, configure, 0)]

    def report(self):
        log.notice('* Passes: %d   Failures: %d' %
                   (len(self.passes), len(self.fails)))
        log.output()
        log.output('Build Report')
        log.output('   Passes: %d   Failures: %d' %
                   (len(self.passes), len(self.fails)))
        log.output(' Failures:')
        if len(self.fails) == 0:
            log.output('None')
        else:
            for f in self.fails:
                arch_bsp = '%s/%s' % (f[0], f[1])
                config_cmd = f[2]
                config_at = config_cmd.find('configure')
                if config_at != -1:
                    config_cmd = config_cmd[config_at:]
                log.output(' %30s:  %s' % (arch_bsp, config_cmd))
        log.output(' Passes:')
        if len(self.passes) == 0:
            log.output('None')
        else:
            for f in self.passes:
                arch_bsp = '%s/%s' % (f[0], f[1])
                config_cmd = f[2]
                config_at = config_cmd.find('configure')
                if config_at != -1:
                    config_cmd = config_cmd[config_at:]
                log.output(' %20s:  %d  %s' % (arch_bsp, f[3], config_cmd))

class configuration:

    def __init__(self):
        self.config = configparser.ConfigParser()
        self.name = None
        self.archs = { }
        self.builds = { }
        self.profiles = { }

    def __str__(self):
        import pprint
        s = self.name + os.linesep
        s += 'Archs:' + os.linesep + \
             pprint.pformat(self.archs, indent = 1, width = 80) + os.linesep
        s += 'Builds:' + os.linesep + \
             pprint.pformat(self.builds, indent = 1, width = 80) + os.linesep
        s += 'Profiles:' + os.linesep + \
             pprint.pformat(self.profiles, indent = 1, width = 80) + os.linesep
        return s

    def _get_item(self, section, label, err = True):
        try:
            rec = self.config.get(section, label).replace(os.linesep, ' ')
            return rec
        except:
            if err:
                raise error.general('config: no %s found in %s' % (label, section))
        return None

    def _get_items(self, section, err = True):
        try:
            items = self.config.items(section)
            return items
        except:
            if err:
                raise error.general('config: section %s not found' % (section))
        return []

    def _comma_list(self, section, label, error = True):
        items = self._get_item(section, label, error)
        if items is None:
            return []
        return sorted(set([a.strip() for a in items.split(',')]))

    def load(self, name):
        if not path.exists(name):
            raise error.general('config: cannot read configuration: %s' % (name))
        self.name = name
        try:
            self.config.read(name)
        except configparser.ParsingError as ce:
            raise error.general('config: %s' % (ce))
        archs = []
        self.profiles['profiles'] = self._comma_list('profiles', 'profiles', error = False)
        if len(self.profiles['profiles']) == 0:
            self.profiles['profiles'] = ['tier_%d' % (t) for t in range(1,4)]
        for p in self.profiles['profiles']:
            profile = {}
            profile['name'] = p
            profile['archs'] = self._comma_list(profile['name'], 'archs')
            archs += profile['archs']
            for arch in profile['archs']:
                bsps = 'bsps_%s' % (arch)
                profile[bsps] = self._comma_list(profile['name'], bsps)
            self.profiles[profile['name']] = profile
        for a in set(archs):
            arch = {}
            arch['excludes'] = {}
            for exclude in self._comma_list(a, 'exclude', error = False):
                arch['excludes'][exclude] = ['all']
            for i in self._get_items(a, False):
                if i[0].startswith('exclude_'):
                    exclude = i[0][len('exclude_'):]
                    if exclude not in arch['excludes']:
                        arch['excludes'][exclude] = []
                    arch['excludes'][exclude] += sorted(set([b.strip() for b in i[1].split(',')]))
            arch['bsps'] = self._comma_list(a, 'bsps', error = False)
            for b in arch['bsps']:
                arch[b] = {}
                arch[b]['bspopts'] = self._comma_list(a, 'bspopts_%s' % (b), error = False)
            self.archs[a] = arch
        builds = {}
        builds['default'] = self._get_item('builds', 'default').split()
        builds['variations'] = self._comma_list('builds', 'variations')
        builds['var_options'] = {}
        for v in builds['variations']:
            builds['var_options'][v] = self._get_item('builds', v).split()
        self.builds = builds

    def variations(self):
        return self.builds['variations']

    def excludes(self, arch):
        excludes = self.archs[arch]['excludes'].keys()
        for exclude in self.archs[arch]['excludes']:
            if 'all' not in self.archs[arch]['excludes'][exclude]:
                excludes.remove(exclude)
        return sorted(excludes)

    def archs(self):
        return sorted(self.archs.keys())

    def arch_present(self, arch):
        return arch in self.archs

    def arch_bsps(self, arch):
        return sorted(self.archs[arch]['bsps'])

    def bsp_present(self, arch, bsp):
        return bsp in self.archs[arch]['bsps']

    def bsp_excludes(self, arch, bsp):
        excludes = self.archs[arch]['excludes'].keys()
        for exclude in self.archs[arch]['excludes']:
            if bsp not in self.archs[arch]['excludes'][exclude]:
                excludes.remove(exclude)
        return sorted(excludes)

    def bspopts(self, arch, bsp):
        return self.archs[arch][bsp]['bspopts']

    def defaults(self):
        return self.builds['default']

    def variant_options(self, variant):
        if variant in self.builds['var_options']:
            return self.builds['var_options'][variant]
        return []

    def profile_present(self, profile):
        return profile in self.profiles

    def profile_archs(self, profile):
        return self.profiles[profile]['archs']

    def profile_arch_bsps(self, profile, arch):
        return self.profiles[profile]['bsps_%s' % (arch)]

class build:

    def __init__(self, config, version, prefix, tools, rtems, build_dir, options):
        self.config = config
        self.build_dir = build_dir
        self.rtems_version = version
        self.prefix = prefix
        self.tools = tools
        self.rtems = rtems
        self.options = options
        self.errors = { 'configure': 0,
                        'build':     0,
                        'tests':     0 }
        self.counts = { 'h'        : 0,
                        'exes'     : 0,
                        'objs'     : 0,
                        'libs'     : 0 }
        self.warnings = warnings_counter(rtems)
        self.results = results()
        if not path.exists(path.join(rtems, 'configure')) or \
           not path.exists(path.join(rtems, 'Makefile.in')) or \
           not path.exists(path.join(rtems, 'cpukit')):
            raise error.general('RTEMS source path does not look like RTEMS')

    def _error_str(self):
        return 'Status: configure:%d build:%d' % \
            (self.errors['configure'], self.errors['build'])

    def _path(self, arch, bsp):
        return path.join(self.build_dir, arch, bsp)

    def _archs(self, build_data):
        return sorted(build_data.keys())

    def _bsps(self, arch):
        return self.config.arch_bsps(arch)

    def _variations(self, arch, bsp):
        def _match(var, vars):
            matches = []
            for v in vars:
                if var in v.split('-'):
                    matches += [v]
            return matches

        vars = self.config.variations()
        for v in self.config.excludes(arch):
            for m in _match(v, vars):
                vars.remove(m)
        for v in self.config.bsp_excludes(arch, bsp):
            for m in _match(v, vars):
                vars.remove(m)
        return vars

    def _arch_bsp_dir_make(self, arch, bsp):
        if not path.exists(self._path(arch, bsp)):
            path.mkdir(self._path(arch, bsp))

    def _arch_bsp_dir_clean(self, arch, bsp):
        if path.exists(self._path(arch, bsp)):
            path.removeall(self._path(arch, bsp))

    def _config_command(self, commands, arch, bsp):
        cmd = [path.join(self.rtems, 'configure')]
        commands += self.config.bspopts(arch, bsp)
        for c in commands:
            c = c.replace('@PREFIX@', self.prefix)
            c = c.replace('@RTEMS_VERSION@', self.rtems_version)
            c = c.replace('@ARCH@', arch)
            c = c.replace('@BSP@', bsp)
            cmd += [c]
        return ' '.join(cmd)

    def _build_set(self, variations):
        build_set = { }
        bs = self.config.defaults()
        for var in variations:
            build_set[var] = bs + self.config.variant_options(var)
        return build_set

    def _build_dir(self, arch, bsp, build):
        return path.join(self._path(arch, bsp), build)

    def _count_files(self, arch, bsp, build):
        counts = { 'h'    : 0,
                   'exes' : 0,
                   'objs' : 0,
                   'libs' : 0 }
        for root, dirs, files in os.walk(self._build_dir(arch, bsp, build)):
            for file in files:
                if file.endswith('.exe'):
                    counts['exes'] += 1
                elif file.endswith('.o'):
                    counts['objs'] += 1
                elif file.endswith('.a'):
                    counts['libs'] += 1
                elif file.endswith('.h'):
                    counts['h'] += 1
        for f in self.counts:
            if f in counts:
                self.counts[f] = counts[f]
        return counts

    def build_arch_bsp(self, arch, bsp):
        if not self.config.bsp_present(arch, bsp):
            raise error.general('BSP not found: %s/%s' % (arch, bsp))
        log.output('-' * 70)
        log.notice('] BSP: %s/%s' % (arch, bsp))
        log.notice('. Creating: %s' % (self._path(arch, bsp)))
        self._arch_bsp_dir_clean(arch, bsp)
        self._arch_bsp_dir_make(arch, bsp)
        variations = self._variations(arch, bsp)
        build_set = self._build_set(variations)
        bsp_start = datetime.datetime.now()
        bsp_warnings = warnings_counter(self.rtems)
        env_path = os.environ['PATH']
        os.environ['PATH'] = path.host(path.join(self.tools, 'bin')) + \
                             os.pathsep + os.environ['PATH']
        for bs in sorted(build_set.keys()):
            warnings = warnings_counter(self.rtems)
            start = datetime.datetime.now()
            log.output('- ' * 35)
            log.notice('. Configuring: %s' % (bs))
            try:
                result = '+ Pass'
                bpath = self._build_dir(arch, bsp, bs)
                path.mkdir(bpath)
                config_cmd = self._config_command(build_set[bs], arch, bsp)
                cmd = config_cmd
                e = execute.capture_execution(log = warnings)
                log.output('run: ' + cmd)
                if self.options['dry-run']:
                    exit_code = 0
                else:
                    exit_code, proc, output = e.shell(cmd, cwd = path.host(bpath))
                if exit_code != 0:
                    result = '- FAIL'
                    self.errors['configure'] += 1
                    log.notice('- Configure failed: %s' % (bs))
                    log.output('cmd failed: %s' % (cmd))
                    if self.options['stop-on-error']:
                        raise error.general('Configuring %s failed' % (bs))
                else:
                    log.notice('. Building: %s' % (bs))
                    cmd = 'make'
                    if 'jobs' in self.options:
                        cmd += ' -j %s' % (self.options['jobs'])
                    log.output('run: ' + cmd)
                    if self.options['dry-run']:
                        exit_code = 0
                    else:
                        exit_code, proc, output = e.shell(cmd, cwd = path.host(bpath))
                    if exit_code != 0:
                        result = '- FAIL'
                        self.errors['build'] += 1
                        log.notice('- FAIL: %s: %s' % (bs, self._error_str()))
                        log.output('cmd failed: %s' % (cmd))
                        if self.options['stop-on-error']:
                            raise error.general('Building %s failed' % (bs))
                    files = self._count_files(arch, bsp, bs)
                log.notice('%s: %s: warnings:%d  exes:%d  objs:%s  libs:%d' % \
                           (result, bs, warnings.get(),
                            files['exes'], files['objs'], files['libs']))
                log.notice('  %s' % (self._error_str()))
                self.results.add(result[0] == '+', arch, bsp, config_cmd, warnings.get())
            finally:
                end = datetime.datetime.now()
                if not self.options['no-clean']:
                    log.notice('. Cleaning: %s' % (self._build_dir(arch, bsp, bs)))
                    path.removeall(self._build_dir(arch, bsp, bs))
            log.notice('^ Time %s' % (str(end - start)))
            log.output('Warnings Report:')
            log.output(warnings.report())
            warnings.accumulate(bsp_warnings)
            warnings.accumulate(self.warnings)
        bsp_end = datetime.datetime.now()
        log.notice('^ BSP Time %s' % (str(bsp_end - bsp_start)))
        log.output('BSP Warnings Report:')
        log.output(bsp_warnings.report())
        os.environ['PATH'] = env_path

    def build_arch(self, arch):
        start = datetime.datetime.now()
        log.output('=' * 70)
        log.notice(']] Architecture: %s' % (arch))
        if not self.confif.arch_present(arch):
            raise error.general('Architecture not found: %s' % (arch))
        for bsp in self._bsps(arch):
            self.build_arch_bsp(arch, bsp)
        log.notice('^ Architecture Time %s' % (str(end - start)))
        log.notice('  warnings:%d  exes:%d  objs:%s  libs:%d' % \
                   self.warnings.get(), self.counts['exes'],
                   self.counts['objs'], self.counts['libs'])
        log.output('Architecture Warnings:')
        log.output(self.warnings.report())

    def build(self):
        for arch in self.config.archs():
            self.build_arch(arch)
        log.notice('^ Profile Time %s' % (str(end - start)))
        log.notice('+  warnings:%d  exes:%d  objs:%s  libs:%d' % \
                   self.warnings.get(), self.counts['exes'],
                   self.counts['objs'], self.counts['libs'])
        log.output('Profile Warnings:')
        log.output(self.warnings.report())

    def build_profile(self, profile):
        if not self.config.profile_present(profile):
            raise error.general('BSP not found: %s/%s' % (arch, bsp))
        start = datetime.datetime.now()
        log.notice(']] Profile: %s' % (profile))
        for arch in self.config.profile_archs(profile):
            for bsp in self.config.profile_arch_bsps(profile, arch):
                self.build_arch_bsp(arch, bsp)
        end = datetime.datetime.now()
        log.notice('^ Profile Time %s' % (str(end - start)))
        log.notice('  warnings:%d  exes:%d  objs:%d  libs:%d' % \
                   (self.warnings.get(), self.counts['exes'],
                    self.counts['objs'], self.counts['libs']))
        log.output('Profile Warnings:')
        log.output(self.warnings.report())

def run_args(args):
    b = None
    ec = 0
    try:
        #
        # On Windows MSYS2 prepends a path to itself to the environment
        # path. This means the RTEMS specific automake is not found and which
        # breaks the bootstrap. We need to remove the prepended path. Also
        # remove any ACLOCAL paths from the environment.
        #
        if os.name == 'nt':
            cspath = os.environ['PATH'].split(os.pathsep)
            if 'msys' in cspath[0] and cspath[0].endswith('bin'):
                os.environ['PATH'] = os.pathsep.join(cspath[1:])

        top = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
        prefix = '/opt/rtems/%s' % (rtems_version())
        tools = prefix
        build_dir = 'bsp-builds'
        logf = 'bsp-build-%s.txt' % (datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
        config_file = path.join(top, 'share', 'rtems', 'tester', 'rtems', 'rtems-bsps.ini')
        if not path.exists(config_file):
            config_file = path.join(top, 'tester', 'rtems', 'rtems-bsps.ini')

        argsp = argparse.ArgumentParser()
        argsp.add_argument('--prefix', help = 'Prefix to build the BSP.', type = str)
        argsp.add_argument('--rtems-tools', help = 'The RTEMS tools directory.', type = str)
        argsp.add_argument('--rtems', help = 'The RTEMS source tree.', type = str)
        argsp.add_argument('--build-path', help = 'Path to build in.', type = str)
        argsp.add_argument('--log', help = 'Log file.', type = str)
        argsp.add_argument('--stop-on-error', help = 'Stop on an error.', action = 'store_true')
        argsp.add_argument('--no-clean', help = 'Do not clean the build output.', action = 'store_true')
        argsp.add_argument('--profiles', help = 'Build the listed profiles.', type = str, default = 'tier-1')
        argsp.add_argument('--arch', help = 'Build the specific architecture.', type = str)
        argsp.add_argument('--bsp', help = 'Build the specific BSP.', type = str)
        argsp.add_argument('--dry-run', help = 'Do not run the actual builds.', action = 'store_true')

        opts = argsp.parse_args(args[1:])
        if opts.log is not None:
            logf = opts.log
        log.default = log.log([logf])
        log.notice('RTEMS Tools Project - RTEMS Kernel BSP Builder, %s' % (version.str()))
        if opts.rtems is None:
            raise error.general('No RTEMS source provided on the command line')
        if opts.prefix is not None:
            prefix = path.shell(opts.prefix)
        if opts.rtems_tools is not None:
            tools = path.shell(opts.rtems_tools)
        if opts.build_path is not None:
            build_dir = path.shell(opts.build_path)
        if opts.bsp is not None and opts.arch is None:
            raise error.general('BSP provided but no architecture')

        config = configuration()
        config.load(config_file)

        options = { 'stop-on-error' : opts.stop_on_error,
                    'no-clean'      : opts.no_clean,
                    'dry-run'       : opts.dry_run,
                    'jobs'          : 8 }

        b = build(config, rtems_version(), prefix, tools, path.shell(opts.rtems), build_dir, options)
        if opts.arch is not None:
            if opts.bsp is not None:
                b.build_arch_bsp(opts.arch, opts.bsp)
            else:
                b.build_arch(opts.arch)
        else:
            for profile in opts.profiles.split(','):
                b.build_profile(profile.strip())

    except error.general as gerr:
        print(gerr)
        print('BSP Build FAILED', file = sys.stderr)
        ec = 1
    except error.internal as ierr:
        print(ierr)
        print('BSP Build FAILED', file = sys.stderr)
        ec = 1
    except error.exit as eerr:
        pass
    except KeyboardInterrupt:
        log.notice('abort: user terminated')
        ec = 1
    if b is not None:
        b.results.report()
    sys.exit(ec)

def run():
    run_args(sys.argv)

if __name__ == "__main__":
    run()
