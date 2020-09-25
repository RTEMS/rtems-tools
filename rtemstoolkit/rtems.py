#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2016-2018 Chris Johns (chrisj@rtems.org)
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

import copy
import os
import re
import sys
import textwrap

from rtemstoolkit import configuration as configuration_
from rtemstoolkit import error
from rtemstoolkit import path
from rtemstoolkit import textbox

#
# The default path we install RTEMS under
#
_prefix_path = '/opt/rtems'

def default_prefix():
    from rtemstoolkit import version
    return path.join(_prefix_path, version.version())

def clean_windows_path():
    '''On Windows MSYS2 prepends a path to itself to the environment path. This
    means the RTEMS specific automake is not found and which breaks the
    bootstrap. We need to remove the prepended path. Also remove any ACLOCAL
    paths from the environment.

    '''
    if os.name == 'nt':
        cspath = os.environ['PATH'].split(os.pathsep)
        if 'msys' in cspath[0] and cspath[0].endswith('bin'):
            os.environ['PATH'] = os.pathsep.join(cspath[1:])

def configuration_path(prog = None):
    '''Return the path the configuration data path for RTEMS. The path is relative
    to the installed executable. Mangage the installed package and the in source
    tree when running from within the rtems-tools repo.
    Note:
     1. This code assumes the executable is wrapped and not using 'env'.
     2. Ok to directly call os.path.
    '''
    if prog is None:
        exec_name = sys.argv[0]
    else:
        exec_name = prog
    exec_name = os.path.abspath(exec_name)
    for top in [os.path.dirname(os.path.dirname(exec_name)),
                os.path.dirname(exec_name)]:
        config_path = path.join(top, 'share', 'rtems', 'config')
        if path.exists(config_path):
            break
        config_path = path.join(top, 'config')
        if path.exists(config_path):
            break
        config_path = None
    return config_path

def configuration_file(config, prog = None):
    '''Return the path to a configuration file for RTEMS. The path is relative to
    the installed executable or we are testing and running from within the
    rtems-tools repo.

    '''
    return path.join(configuration_path(prog = prog), config)

def bsp_configuration_file(prog = None):
    '''Return the path to the BSP configuration file for RTEMS. The path is
    relative to the installed executable or we are testing and running from
    within the rtems-tools repo.

    '''
    return configuration_file('rtems-bsps.ini', prog = prog)

class configuration:

    def __init__(self):
        self.config = configuration_.configuration(raw = False)
        self.archs = { }
        self.profiles = { }

    def __str__(self):
        s = self.name + os.linesep
        s += 'Archs:' + os.linesep + \
             pprint.pformat(self.archs, indent = 1, width = 80) + os.linesep
        s += 'Profiles:' + os.linesep + \
             pprint.pformat(self.profiles, indent = 1, width = 80) + os.linesep
        return s

    def _build_options(self, build, nesting = 0):
        if ':' in build:
            section, name = build.split(':', 1)
            opts = [self.config.get_item(section, name)]
            return opts
        builds = self.builds_['builds']
        if build not in builds:
            raise error.general('build %s not found' % (build))
        if nesting > 20:
            raise error.general('nesting build %s' % (build))
        options = []
        for option in self.builds_['builds'][build]:
            if ':' in option:
                section, name = option.split(':', 1)
                opts = [self.config.get_item(section, name)]
            else:
                opts = self._build_options(option, nesting + 1)
            for opt in opts:
                if opt not in options:
                    options += [opt]
        return options

    def load(self, name, build):
        self.config.load(name)
        archs = []
        self.profiles['profiles'] = \
            self.config.comma_list('profiles', 'profiles', err = False)
        if len(self.profiles['profiles']) == 0:
            self.profiles['profiles'] = ['tier-%d' % (t) for t in range(1,4)]
        for p in self.profiles['profiles']:
            profile = {}
            profile['name'] = p
            profile['archs'] = self.config.comma_list(profile['name'], 'archs', err = False)
            archs += profile['archs']
            for arch in profile['archs']:
                bsps = 'bsps_%s' % (arch)
                profile[bsps] = self.config.comma_list(profile['name'], bsps)
            self.profiles[profile['name']] = dict(profile)
            profile = None
        invalid_chars = re.compile(r'[^a-zA-Z0-9_-]')
        archs = sorted(list(set(archs)))
        for a in set(archs):
            if len(invalid_chars.findall(a)) != 0:
                raise error.general('invalid character(s) in arch name: %s' % (a))
            arch = {}
            arch['excludes'] = {}
            for exclude in self.config.comma_list(a, 'exclude', err = False):
                arch['excludes'][exclude] = ['all']
            for i in self.config.get_items(a, False):
                if i[0].startswith('exclude-'):
                    exclude = i[0][len('exclude-'):]
                    if exclude not in arch['excludes']:
                        arch['excludes'][exclude] = []
                    arch['excludes'][exclude] += \
                        sorted(set([b.strip() for b in i[1].split(',')]))
            arch['bsps'] = self.config.comma_list(a, 'bsps', err = False)
            for b in arch['bsps']:
                if len(invalid_chars.findall(b)) != 0:
                    raise error.general('invalid character(s) in BSP name: %s' % (b))
                arch[b] = {}
                arch[b]['bspopts'] = \
                    self.config.comma_list(a, 'bspopts_%s' % (b), err = False)
            self.archs[a] = arch
        builds = {}
        builds['default'] = self.config.get_item('builds', 'default')
        if build is None:
            build = builds['default']
        builds['config'] = { }
        for config in self.config.get_items('config'):
            builds['config'][config[0]] = config[1]
        builds['build'] = build
        builds_ = self.config.get_item_names('builds')
        builds['builds'] = {}
        for build in builds_:
            build_builds = self.config.comma_list('builds', build)
            has_config = False
            has_build = False
            for b in build_builds:
                if ':' in b:
                    if has_build:
                        raise error.general('config and build in build: %s' % (build))
                    has_config = True
                else:
                    if has_config:
                        raise error.general('config and build in build: %s' % (build))
                    has_build = True
            builds['builds'][build] = build_builds
        self.builds_ = builds

    def configs(self):
        return sorted(list(self.builds_['config'].keys()))

    def config_flags(self, config):
        if config not in self.builds_['config']:
            raise error.general('config entry not found: %s' % (config))
        return self.builds_['config'][config]

    def build(self):
        return self.builds_['build']

    def builds(self):
        if self.builds_['build'] in self.builds_['builds']:
            build = copy.copy(self.builds_['builds'][self.builds_['build']])
            if ':' in build[0]:
                return [self.builds_['build']]
            return build
        return None

    def build_options(self, build):
        return ' '.join(self._build_options(build))

    def excludes(self, arch, bsp):
        return list(set(self.arch_excludes(arch) + self.bsp_excludes(arch, bsp)))

    def exclude_options(self, arch, bsp):
        return ' '.join([self.config_flags('no-' + e) for e in self.excludes(arch, bsp)])

    def archs(self):
        return sorted(list(self.archs.keys()))

    def arch_present(self, arch):
        return arch in self.archs

    def arch_excludes(self, arch):
        excludes = list(self.archs[arch]['excludes'].keys())
        for exclude in self.archs[arch]['excludes']:
            if 'all' not in self.archs[arch]['excludes'][exclude]:
                excludes.remove(exclude)
        return sorted(excludes)

    def arch_bsps(self, arch):
        return sorted(self.archs[arch]['bsps'])

    def bsp_present(self, arch, bsp):
        return bsp in self.archs[arch]['bsps']

    def bsp_excludes(self, arch, bsp):
        excludes = list(self.archs[arch]['excludes'].keys())
        for exclude in self.archs[arch]['excludes']:
            if 'all' not in self.archs[arch]['excludes'][exclude] and \
               bsp not in self.archs[arch]['excludes'][exclude]:
                excludes.remove(exclude)
        return sorted(excludes)

    def bspopts(self, arch, bsp):
        if arch not in self.archs:
            raise error.general('invalid architecture: %s' % (arch))
        if bsp not in self.archs[arch]:
            raise error.general('invalid BSP: %s' % (bsp))
        return self.archs[arch][bsp]['bspopts']

    def profile_present(self, profile):
        return profile in self.profiles

    def profile_archs(self, profile):
        if profile not in self.profiles:
            raise error.general('invalid profile: %s' % (profile))
        return self.profiles[profile]['archs']

    def profile_arch_bsps(self, profile, arch):
        if profile not in self.profiles:
            raise error.general('invalid profile: %s' % (profile))
        if 'bsps_%s' % (arch) not in self.profiles[profile]:
            raise error.general('invalid profile arch: %s' % (arch))
        return ['%s/%s' % (arch, bsp) for bsp in self.profiles[profile]['bsps_%s' % (arch)]]

    def report(self, profiles = True, builds = True, architectures = True):
        width = 70
        cols_1 = [width]
        cols_2 = [10, width - 10]
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s1 = ' File(s)'
        for f in self.config.files():
            colon = ':'
            for l in textwrap.wrap(f, width = cols_2[1] - 3):
                s += textbox.row(cols_2, [s1, ' ' + l], marker = colon, indent = 1)
                colon = ' '
                s1 = ' ' * len(s1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        s += os.linesep
        if profiles:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            profiles = sorted(self.profiles['profiles'])
            archs = []
            bsps = []
            for profile in profiles:
                archs += self.profiles[profile]['archs']
                for arch in sorted(self.profiles[profile]['archs']):
                    bsps += self.profiles[profile]['bsps_%s' % (arch)]
            archs = len(set(archs))
            bsps = len(set(bsps))
            s += textbox.row(cols_1,
                             [' Profiles : %d (archs:%d, bsps:%d)' % \
                              (len(profiles), archs, bsps)],
                             indent = 1)
            for profile in profiles:
                textbox.row(cols_2,
                            [profile, self.profiles[profile]['name']],
                            indent = 1)
            s += textbox.line(cols_1, marker = '+', indent = 1)
            for profile in profiles:
                s += textbox.row(cols_1, [' %s' % (profile)], indent = 1)
                profile = self.profiles[profile]
                archs = sorted(profile['archs'])
                for arch in archs:
                    arch_bsps = ', '.join(profile['bsps_%s' % (arch)])
                    if len(arch_bsps) > 0:
                        s += textbox.line(cols_2, marker = '+', indent = 1)
                        s1 = ' ' + arch
                        for l in textwrap.wrap(arch_bsps,
                                               width = cols_2[1] - 3):
                            s += textbox.row(cols_2, [s1, ' ' + l], indent = 1)
                            s1 = ' ' * len(s1)
                s += textbox.line(cols_2, marker = '+', indent = 1)
            s += os.linesep
        if builds:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            s += textbox.row(cols_1,
                             [' Builds:  %s (default)' % (self.builds_['default'])],
                             indent = 1)
            builds = self.builds_['builds']
            bsize = 0
            for build in builds:
                if len(build) > bsize:
                    bsize = len(build)
            cols_b = [bsize + 2, width - bsize - 2]
            s += textbox.line(cols_b, marker = '+', indent = 1)
            for build in builds:
                s1 = ' ' + build
                for l in textwrap.wrap(', '.join(builds[build]),
                                       width = cols_b[1] - 3):
                    s += textbox.row(cols_b, [s1, ' ' + l], indent = 1)
                    s1 = ' ' * len(s1)
                s += textbox.line(cols_b, marker = '+', indent = 1)
            configs = self.builds_['config']
            s += textbox.row(cols_1,
                             [' Configure Options: %d' % (len(configs))],
                             indent = 1)
            csize = 0
            for config in configs:
                if len(config) > csize:
                    csize = len(config)
            cols_c = [csize + 3, width - csize - 3]
            s += textbox.line(cols_c, marker = '+', indent = 1)
            for config in configs:
                s1 = ' ' + config
                for l in textwrap.wrap(configs[config], width = cols_c[1] - 3):
                    s += textbox.row(cols_c, [s1, ' ' + l], indent = 1)
                    s1 = ' ' * len(s1)
                s += textbox.line(cols_c, marker = '+', indent = 1)
            s += os.linesep
        if architectures:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            archs = sorted(list(self.archs.keys()))
            bsps = 0
            asize = 0
            for arch in archs:
                if len(arch) > asize:
                    asize = len(arch)
                bsps += len(self.archs[arch]['bsps'])
            s += textbox.row(cols_1,
                             [' Architectures : %d (bsps: %d)' % (len(archs), bsps)],
                             indent = 1)
            cols_a = [asize + 2, width - asize - 2]
            s += textbox.line(cols_a, marker = '+', indent = 1)
            for arch in archs:
                s += textbox.row(cols_a,
                                 [' ' + arch, ' %d' % (len(self.archs[arch]['bsps']))],
                                 indent = 1)
            s += textbox.line(cols_a, marker = '+', indent = 1)
            for archn in archs:
                arch = self.archs[archn]
                if len(arch['bsps']) > 0:
                    bsize = 0
                    for bsp in arch['bsps']:
                        if len(bsp) > bsize:
                            bsize = len(bsp)
                    cols_b = [bsize + 3, width - bsize - 3]
                    s += textbox.row(cols_1, [' ' + archn + ':'], indent = 1)
                    s += textbox.line(cols_b, marker = '+', indent = 1)
                    for bsp in arch['bsps']:
                        s1 = ' ' + bsp
                        bspopts = ' '.join(arch[bsp]['bspopts'])
                        if len(bspopts):
                            for l in textwrap.wrap('bopt: ' + bspopts,
                                                   width = cols_b[1] - 3):
                                s += textbox.row(cols_b, [s1, ' ' + l], indent = 1)
                                s1 = ' ' * len(s1)
                        excludes = []
                        for exclude in arch['excludes']:
                            if 'all' in arch['excludes'][exclude] or \
                               bsp in arch['excludes'][exclude]:
                                excludes += [exclude]
                        excludes = ', '.join(excludes)
                        if len(excludes):
                            for l in textwrap.wrap('ex: ' + excludes,
                                                   width = cols_b[1] - 3):
                                s += textbox.row(cols_b, [s1, ' ' + l], indent = 1)
                                s1 = ' ' * len(s1)
                        if len(bspopts) == 0 and len(excludes) == 0:
                            s += textbox.row(cols_b, [s1, ' '], indent = 1)
                    s += textbox.line(cols_b, marker = '+', indent = 1)
            s += os.linesep
        return s
