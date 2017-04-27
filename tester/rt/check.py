#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2016-2017 Chris Johns (chrisj@rtems.org)
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
import re
import sys
import textwrap

import pprint

try:
    import configparser
except:
    import ConfigParser as configparser

from rtemstoolkit import execute
from rtemstoolkit import error
from rtemstoolkit import host
from rtemstoolkit import log
from rtemstoolkit import path
from rtemstoolkit import textbox
from rtemstoolkit import version

def rtems_version():
    return version.version()

def wrap(line, lineend = '', indent = 0, width = 75):
    if type(line) is tuple or type(line) is list:
        if len(line) >= 2:
            s1 = line[0]
        else:
            s1 = ''
        s2 = line[1:]
    elif type(line) is str:
        s1 = ''
        s2 = [line]
    else:
        raise error.internal('line is not a tuple, list or string')
    s1len = len(s1)
    s = ''
    first = True
    for ss in s2:
        if type(ss) is not str and type(ss) is not unicode:
            raise error.internal('text needs to be a string')
        for l in textwrap.wrap(ss, width = width - s1len - indent - 1):
            s += '%s%s%s%s%s' % (' ' * indent, s1, l, lineend, os.linesep)
            if first and s1len > 0:
                s1 = ' ' * s1len
    if lineend != '':
        s = s[:0 - len(os.linesep) - 1] + os.linesep
    return s

def title():
    return 'RTEMS Tools Project - RTEMS Kernel BSP Builder, %s' % (version.str())

def command_line():
    return wrap(('command: ', ' '.join(sys.argv)), lineend = '\\')

class warnings_errors:

    def __init__(self, rtems):
        self.rtems = path.host(rtems)
        self.reset()
        self.groups = { 'groups'  : ['Shared', 'BSP', 'Network', 'Tests',
                                     'LibCPU', 'CPU Kit'],
                        'exclude' : '.*Makefile.*',
                        'CPU Kit' : '.*cpukit/.*',
                        'Network' : '.*libnetworking/.*|.*librpc/.*',
                        'Tests'   : '.*testsuites/.*',
                        'BSP'     : '.*libbsp/.*',
                        'LibCPU'  : '.*libcpu/.*',
                        'Shared'  : '.*shared/.*' }
        self.arch = None
        self.bsp = None
        self.build = None

    def _opts(self, arch = None, bsp = None, build = None):
        if arch is None:
            arch = self.arch
        if bsp is None:
            bsp = self.bsp
        if build is None:
            build = self.build
        return arch, bsp, build

    def _key(self, arch, bsp, build):
        arch, bsp, build = self._opts(arch, bsp, build)
        return '%s/%s-%s' % (arch, bsp, build)

    def _get_warnings(self, arch = None, bsp = None, build = None):
        arch, bsp, build = self._opts(arch = arch, bsp = bsp, build = build)
        if arch is None:
            arch = '.*'
        if bsp is None:
            bsp = '.*'
        if build is None:
            build = '.*'
        selector = re.compile('^%s/%s-%s$' % (arch, bsp, build))
        warnings = [w for w in self.warnings if selector.match(w)]
        return sorted(warnings)

    def _total(self, archive):
        total = 0
        for a in archive:
            total += archive[a]
        return total

    def _analyze(self, warnings, exclude):
        def _group(data, category, name, warning, count, groups, group_regx):
            if 'groups' not in data:
                data['groups'] = { }
            if category not in data['groups']:
                data['groups'][category] = { 'totals' : { } }
            if name not in data['groups'][category]:
                data['groups'][category][name] = { }
            for group in groups:
                if group not in data['groups'][category]['totals']:
                    data['groups'][category]['totals'][group] = 0
                if group not in data['groups'][category][name]:
                    data['groups'][category][name][group] = 0
                if group_regx[group].match(warning):
                    data['groups'][category][name][group] += count
                    data['groups'][category]['totals'][group] += count
                    break

        def _update(data, category, name, warning, count, groups, group_regx):
            if category not in data:
                data[category] = { }
            if name not in data[category]:
                data[category][name] = { }
            if warning not in data[category][name]:
                data[category][name][warning] = 0
            data[category][name][warning] += count
            _group(data, category, name,  w, count, groups, group_regx)

        data = { }
        group_regx = { }
        for group in self.groups['groups']:
            group_regx[group] = re.compile(self.groups[group])
        exclude_regx = re.compile(exclude)
        for warning in warnings:
            arch = warning.split('/', 1)[0]
            arch_bsp = warning.split('-', 1)[0]
            build = warning.split('-', 1)[1]
            for w in self.warnings[warning]:
                if not exclude_regx.match(w):
                    count = self.warnings[warning][w]
                    _update(data, 'arch',     arch,     w, count,
                           self.groups['groups'], group_regx)
                    _update(data, 'arch_bsp', arch_bsp, w, count,
                           self.groups['groups'], group_regx)
                    _update(data, 'build',  build,  w, count,
                           self.groups['groups'], group_regx)
        for category in ['arch', 'arch_bsp', 'build']:
            common = {}
            for name in data[category]:
                for w in data[category][name]:
                    if w not in common:
                        for other in [n for n in data[category] if n != name]:
                            if w in data[category][other]:
                                common[w] = data[category][name][w]
                                _group(data, category, 'common', w, common[w],
                                       self.groups['groups'], group_regx)
            data[category]['common'] = common
        return data

    def _report_category(self, label, warnings, group_counts):
        width = 70
        cols_1 = [width]
        cols_2 = [8, width - 8]
        cols_4 = textbox.even_columns(4, width)
        cols_2_4 = textbox.merge_columns([cols_2, cols_4])
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s += textbox.row(cols_1, [' ' + label], indent = 1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        builds = ['common'] + sorted([b for b in warnings if b != 'common'])
        common = warnings['common']
        for build in builds:
            build_warnings = warnings[build]
            if build is not 'common':
                build_warnings = [w for w in build_warnings if w not in common]
            s += textbox.row(cols_1,
                             [' %s : %d warning(s)' % (build,
                                                       len(build_warnings))],
                             indent = 1)
            if len(build_warnings) == 0:
                s += textbox.line(cols_1, marker = '+', indent = 1)
            else:
                s += textbox.line(cols_4, marker = '+', indent = 1)
                if build not in group_counts:
                    gs = [0 for group in self.groups['groups']]
                else:
                    gs = []
                    for g in range(0, len(self.groups['groups'])):
                        group = self.groups['groups'][g]
                        if group in group_counts[build]:
                            count = group_counts[build][group]
                        else:
                            count = 0
                        gs += ['%*s' % (cols_4[g % 4] - 2,
                                        '%s : %4d' % (group, count))]
                    for row in range(0, len(self.groups['groups']), 4):
                        if row + 4 > len(self.groups['groups']):
                            d = gs[row:] + \
                                ['' for r in range(row,
                                                   len(self.groups['groups']))]
                        else:
                            d = gs[row:+4]
                        s += textbox.row(cols_4, d, indent = 1)
                s += textbox.line(cols_2_4, marker = '+', indent = 1)
                vw = sorted([(w, warnings[build][w]) for w in build_warnings],
                            key = operator.itemgetter(1),
                            reverse = True)
                for w in vw:
                    c1 = '%6d' % w[1]
                    for l in textwrap.wrap(' ' + w[0], width = cols_2[1] - 3):
                        s += textbox.row(cols_2, [c1, l], indent = 1)
                        c1 = ' ' * 6
                s += textbox.line(cols_2, marker = '+', indent = 1)
        return s

    def _report_warning_map(self):
        builds = self.messages['warnings']
        width = 70
        cols_1 = [width]
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s += textbox.row(cols_1, [' Warning Map'], indent = 1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        for build in builds:
            messages = builds[build]
            s += textbox.row(cols_1, [' %s : %d' % (build, len(messages))], indent = 1)
            s += textbox.line(cols_1, marker = '+', indent = 1)
            for msg in messages:
                for l in textwrap.wrap(msg, width = width - 3):
                    s += textbox.row(cols_1, [' ' + l], indent = 1)
                for l in textwrap.wrap(messages[msg], width = width - 3 - 4):
                    s += textbox.row(cols_1, ['    ' + l], indent = 1)
            s += textbox.line(cols_1, marker = '+', indent = 1)
        return s

    def report(self):
        arch, bsp, build = self._opts()
        warnings = self._get_warnings(arch, bsp, build)
        total = 0
        for build in warnings:
            total += self._total(self.warnings[build])
        if total == 0:
            s = ' No warnings'
        else:
            data = self._analyze(warnings, self.groups['exclude'])
            s = self._report_category('By Architecture (total : %d)' % (total),
                                      data['arch'], data['groups']['arch'])
            s += os.linesep
            s += self._report_category('By BSP (total : %d)' % (total),
                                       data['arch_bsp'], data['groups']['arch_bsp'])
            s += os.linesep
            s += self._report_category('By Build (total : %d)' % (total),
                                       data['build'], data['groups']['build'])
            s += os.linesep
            s += self._report_warning_map()
            s += os.linesep

        return s

    def set_build(self, arch, bsp, build):
        self.arch = arch
        self.bsp = bsp
        self.build = build
        self.build_key = '%s/%s-%s' % (arch, bsp, build)
        if self.build_key not in self.warnings:
            self.warnings[self.build_key] = {}
        if self.build_key not in self.errors:
            self.errors[self.build_key] = {}

    def clear_build(self):
        self.arch = None
        self.bsp = None
        self.build = None
        self.build_key = None

    def get_warning_count(self):
        return self.warning_count

    def get_error_count(self):
        return self.error_count

    def reset(self):
        self.warnings = { }
        self.warning_count = 0
        self.errors = { }
        self.error_count = 0
        self.messages = { 'warnings' : { }, 'errors' : { } }

    def get_warning_messages(self, arch = None, bsp = None, build = None):
        key = self._key(arch, bsp, build)
        if key not in self.messages['warnings']:
            return []
        messages = self.messages['warnings'][key]
        return ['%s %s' % (m, messages[m]) for m in messages]

    def get_error_messages(self, arch = None, bsp = None, build = None):
        key = self._key(arch, bsp, build)
        if key not in self.messages['errors']:
            return []
        messages = self.messages['errors'][key]
        return ['%s %s' % (m, messages[m]) for m in messages]

    def output(self, text):
        def _line_split(line, source_base):
            ls = line.split(' ', 1)
            fname = ls[0].split(':')
            #
            # Ignore compiler option warnings.
            #
            if len(fname) < 4:
                return None
            p = path.abspath(fname[0])
            p = p.replace(source_base, '')
            if path.isabspath(p):
                p = p[1:]
            return p, fname[1], fname[2], ls[1]

        if self.build_key is not None and \
           (' warning:' in text or ' error:' in text):
            for l in text.splitlines():
                if ' warning:' in l:
                    self.warning_count += 1
                    archive = self.warnings[self.build_key]
                    messages = 'warnings'
                elif ' error:' in l:
                    self.error_count += 1
                    archive = self.errors[self.build_key]
                    messages = 'errors'
                else:
                    continue
                line_parts = _line_split(l, self.rtems)
                if line_parts is not None:
                    src, line, pos, msg = line_parts
                    where = '%s:%s:%s' % (src, line, pos)
                    if where not in archive:
                        archive[where] = 1
                    else:
                        archive[where] += 1
                    if self.build_key not in self.messages[messages]:
                        self.messages[messages][self.build_key] = { }
                    self.messages[messages][self.build_key][where] = msg

        log.output(text)

class results:

    def __init__(self):
        self.passes = []
        self.fails = []

    def _arch_bsp(self, arch, bsp):
        return '%s/%s' % (arch, bsp)

    def add(self, good, arch, bsp, configure, warnings, error_messages):
        if good:
            self.passes += [(arch, bsp, configure, warnings, None)]
        else:
            self.fails += [(arch, bsp, configure, warnings, error_messages)]

    def report(self):
        log.notice('* Passes: %d   Failures: %d' %
                   (len(self.passes), len(self.fails)))
        log.output()
        log.output('Build Report')
        log.output('   Passes: %d   Failures: %d' %
                   (len(self.passes), len(self.fails)))
        log.output(' Failures:')
        if len(self.fails) == 0:
            log.output('  None')
        else:
            max_col = 0
            for f in self.fails:
                arch_bsp = self._arch_bsp(f[0], f[1])
                if len(arch_bsp) > max_col:
                    max_col = len(arch_bsp)
            for f in self.fails:
                config_cmd = f[2]
                config_at = config_cmd.find('configure')
                if config_at != -1:
                    config_cmd = config_cmd[config_at:]
                log.output(' %*s:' % (max_col + 2, self._arch_bsp(f[0], f[1])))
                s1 = ' ' * 6
                log.output(wrap([s1, config_cmd], lineend = '\\', width = 75))
                if f[4] is not None:
                    s1 = ' ' * len(s1)
                    for msg in f[4]:
                        log.output(wrap([s1, msg], lineend = '\\'))
        log.output(' Passes:')
        if len(self.passes) == 0:
            log.output('  None')
        else:
            max_col = 0
            for f in self.passes:
                arch_bsp = self._arch_bsp(f[0], f[1])
                if len(arch_bsp) > max_col:
                    max_col = len(arch_bsp)
            for f in self.passes:
                config_cmd = f[2]
                config_at = config_cmd.find('configure')
                if config_at != -1:
                    config_cmd = config_cmd[config_at:]
                log.output(' %s (%5d):' % (self._arch_bsp(f[0], f[1]), f[3]))
                log.output(wrap([' ' * 6, config_cmd], lineend = '\\', width = 75))

class configuration:

    def __init__(self):
        self.config = configparser.ConfigParser()
        self.name = None
        self.archs = { }
        self.builds_ = { }
        self.profiles = { }
        self.configurations = { }

    def __str__(self):
        import pprint
        s = self.name + os.linesep
        s += 'Archs:' + os.linesep + \
             pprint.pformat(self.archs, indent = 1, width = 80) + os.linesep
        s += 'Builds:' + os.linesep + \
             pprint.pformat(self.builds_, indent = 1, width = 80) + os.linesep
        s += 'Profiles:' + os.linesep + \
             pprint.pformat(self.profiles, indent = 1, width = 80) + os.linesep
        return s

    def _get_item(self, section, label, err = True):
        try:
            rec = self.config.get(section, label).replace(os.linesep, ' ')
            return rec
        except:
            if err:
                raise error.general('config: no "%s" found in "%s"' % (label, section))
        return None

    def _get_items(self, section, err = True):
        try:
            items = [(name, key.replace(os.linesep, ' ')) \
                     for name, key in self.config.items(section)]
            return items
        except:
            if err:
                raise error.general('config: section "%s" not found' % (section))
        return []

    def _comma_list(self, section, label, error = True):
        items = self._get_item(section, label, error)
        if items is None:
            return []
        return sorted(set([a.strip() for a in items.split(',')]))

    def _get_item_names(self, section, err = True):
        try:
            return [item[0] for item in self.config.items(section)]
        except:
            if err:
                raise error.general('config: section "%s" not found' % (section))
        return []

    def _build_options(self, build, nesting = 0):
        if ':' in build:
            section, name = build.split(':', 1)
            opts = [self._get_item(section, name)]
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
                opts = [self._get_item(section, name)]
            else:
                opts = self._options(option, nesting + 1)
            for opt in opts:
                if opt not in options:
                    options += [opt]
        return options

    def load(self, name, build):
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
            self.profiles['profiles'] = ['tier-%d' % (t) for t in range(1,4)]
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
        builds['default'] = self._get_item('builds', 'default')
        if build is None:
            build = builds['default']
        builds['config'] = { }
        for config in self._get_items('config'):
            builds['config'][config[0]] = config[1]
        builds['build'] = build
        builds_ = self._get_item_names('builds')
        builds['builds'] = {}
        for build in builds_:
            build_builds = self._comma_list('builds', build)
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

    def build(self):
        return self.builds_['build']

    def builds(self):
        if self.builds_['build'] in self.builds_['builds']:
            build = self.builds_['builds'][self.builds_['build']]
            if ':' in build[0]:
                return [self.builds_['build']]
            return build
        return None

    def build_options(self, build):
        return ' '.join(self._build_options(build))

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

    def profile_present(self, profile):
        return profile in self.profiles

    def profile_archs(self, profile):
        return self.profiles[profile]['archs']

    def profile_arch_bsps(self, profile, arch):
        return self.profiles[profile]['bsps_%s' % (arch)]

    def report(self, profiles = True, builds = True, architectures = True):
        width = 70
        cols_1 = [width]
        cols_2 = [10, width - 10]
        s = textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        s1 = ' File'
        colon = ':'
        for l in textwrap.wrap(self.name, width = cols_2[1] - 3):
            s += textbox.row(cols_2, [s1, ' ' + l], marker = colon, indent = 1)
            colon = ' '
            s1 = ' ' * len(s1)
        s += textbox.line(cols_1, marker = '+', indent = 1)
        s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
        if profiles:
            profiles = sorted(self.profiles['profiles'])
            bsps = 0
            for profile in profiles:
                archs = sorted(self.profiles[profile]['archs'])
                for arch in archs:
                    bsps += len(self.profiles[profile]['bsps_%s' % (arch)])
            s += textbox.row(cols_1,
                             [' Profiles : %d/%d' % (len(archs), bsps)],
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
                    s += textbox.line(cols_2, marker = '+', indent = 1)
                    s1 = ' ' + arch
                    for l in textwrap.wrap(', '.join(profile['bsps_%s' % (arch)]),
                                           width = cols_2[1] - 2):
                        s += textbox.row(cols_2, [s1, ' ' + l], indent = 1)
                        s1 = ' ' * len(s1)
                s += textbox.line(cols_2, marker = '+', indent = 1)
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
                                       width = cols_b[1] - 2):
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
        if architectures:
            s += textbox.line(cols_1, line = '=', marker = '+', indent = 1)
            archs = sorted(self.archs.keys())
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
                            if bsp in arch['excludes'][exclude]:
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
        return s

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
                        'tests':     0,
                        'fails':     []}
        self.counts = { 'h'        : 0,
                        'exes'     : 0,
                        'objs'     : 0,
                        'libs'     : 0 }
        self.warnings_errors = warnings_errors(rtems)
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

    def _build(self):
        return self.config.build()

    def _builds(self, arch, bsp):
        builds = self.config.builds()
        if builds is None:
            return None
        for b in self.config.excludes(arch):
            if b in builds:
                builds.remove(b)
        for b in self.config.bsp_excludes(arch, bsp):
            if b in builds:
                builds.remove(b)
        return builds

    def _arch_bsp_dir_make(self, arch, bsp):
        if not path.exists(self._path(arch, bsp)):
            path.mkdir(self._path(arch, bsp))

    def _arch_bsp_dir_clean(self, arch, bsp):
        if path.exists(self._path(arch, bsp)):
            path.removeall(self._path(arch, bsp))

    def _config_command(self, commands, arch, bsp):
        if type(commands) is not list:
            commands = [commands]
        cmd = [path.join(self.rtems, 'configure')]
        commands += self.config.bspopts(arch, bsp)
        for c in commands:
            c = c.replace('@PREFIX@', self.prefix)
            c = c.replace('@RTEMS_VERSION@', self.rtems_version)
            c = c.replace('@ARCH@', arch)
            c = c.replace('@BSP@', bsp)
            cmd += [c]
        return ' '.join(cmd)

    def _build_set(self, builds):
        build_set = { }
        for build in builds:
            build_set[build] = self.config.build_options(build)
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
                self.counts[f] += counts[f]
        return counts

    def _have_failures(self, fails):
        return len(fails) != 0

    def _warnings_report(self):
        if self.options['warnings-report'] is not None:
            with open(self.options['warnings-report'], 'w') as f:
                f.write(title() + os.linesep)
                f.write(os.linesep)
                f.write('Date: %s%s' % (datetime.datetime.now().strftime('%c'),
                                        os.linesep))
                f.write(os.linesep)
                f.write(command_line() + os.linesep)
                f.write(self.warnings_errors.report())

    def _finished(self):
        log.notice('+  warnings:%d  exes:%d  objs:%d  libs:%d' % \
                   (self.warnings_errors.get_warning_count(), self.counts['exes'],
                    self.counts['objs'], self.counts['libs']))
        log.output()
        log.output('Warnings:')
        log.output(self.warnings_errors.report())
        log.output()
        log.notice('Failures:')
        log.notice(self.failures_report(self.errors['fails']))
        self._warnings_report()

    def failures_report(self, fails):
        if not self._have_failures(fails):
            return ' No failure(s)'
        absize = 0
        bsize = 0
        ssize = 0
        for f in fails:
            arch_bsp = '%s/%s' % (f[1], f[2])
            if len(arch_bsp) > absize:
                absize = len(arch_bsp)
            if len(f[3]) > bsize:
                bsize = len(f[3])
            if len(f[0]) > ssize:
                ssize = len(f[0])
        fc = 1
        s = ''
        for f in fails:
            fcl = ' %3d' % (fc)
            arch_bsp = '%s/%s' % (f[1], f[2])
            state = f[0]
            s += '%s %-*s %-*s %-*s:%s' % \
                 (fcl, bsize, f[3], absize, arch_bsp, ssize, state, os.linesep)
            s1 = ' ' * 6
            s += wrap((s1, 'configure: ' + f[4]), lineend = '\\', width = 75)
            for e in self.warnings_errors.get_error_messages(f[1], f[2], f[3]):
                s += wrap([s1, 'error: ' + e])
            fc += 1
        return s

    def build_arch_bsp(self, arch, bsp):
        if not self.config.bsp_present(arch, bsp):
            raise error.general('BSP not found: %s/%s' % (arch, bsp))
        log.output('-' * 70)
        log.notice('] BSP: %s/%s' % (arch, bsp))
        log.notice('. Creating: %s' % (self._path(arch, bsp)))
        self._arch_bsp_dir_clean(arch, bsp)
        self._arch_bsp_dir_make(arch, bsp)
        builds = self._builds(arch, bsp)
        if builds is None:
            raise error.general('build not found: %s' % (self._build()))
        build_set = self._build_set(builds)
        bsp_start = datetime.datetime.now()
        env_path = os.environ['PATH']
        os.environ['PATH'] = path.host(path.join(self.tools, 'bin')) + \
                             os.pathsep + os.environ['PATH']
        fails = []
        for bs in sorted(build_set.keys()):
            self.warnings_errors.set_build(arch, bsp, bs)
            start = datetime.datetime.now()
            log.output('- ' * 35)
            log.notice('. Configuring: %s' % (bs))
            try:
                warnings = self.warnings_errors.get_warning_count()
                result = '+ Pass'
                bpath = self._build_dir(arch, bsp, bs)
                good = True
                error_messages = None
                path.mkdir(bpath)
                config_cmd = self._config_command(build_set[bs], arch, bsp)
                cmd = config_cmd
                e = execute.capture_execution(log = self.warnings_errors)
                log.output(wrap(('run: ', cmd), lineend = '\\'))
                if self.options['dry-run']:
                    exit_code = 0
                else:
                    exit_code, proc, output = e.shell(cmd, cwd = path.host(bpath))
                if exit_code != 0:
                    result = '- FAIL'
                    failure = ('configure', arch, bsp, bs, config_cmd)
                    fails += [failure]
                    self.errors['configure'] += 1
                    self.errors['fails'] += [failure]
                    log.notice('- Configure failed: %s' % (bs))
                    log.output('cmd failed: %s' % (cmd))
                    good = False
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
                        error_messages = self.warnings_errors.get_error_messages()
                        result = '- FAIL'
                        failure = ('build', arch, bsp, bs, config_cmd, error_messages)
                        fails += [failure]
                        self.errors['build'] += 1
                        self.errors['fails'] += [failure]
                        log.notice('- FAIL: %s: %s' % (bs, self._error_str()))
                        log.output('cmd failed: %s' % (cmd))
                        good = False
                    files = self._count_files(arch, bsp, bs)
                    log.notice('%s: %s: warnings:%d  exes:%d  objs:%s  libs:%d' % \
                               (result, bs,
                                self.warnings_errors.get_warning_count() - warnings,
                                files['exes'], files['objs'], files['libs']))
                log.notice('  %s' % (self._error_str()))
                self.results.add(good, arch, bsp, config_cmd,
                                 self.warnings_errors.get_warning_count() - warnings,
                                 error_messages)
                if not good and self.options['stop-on-error']:
                    raise error.general('Configuring %s failed' % (bs))
            finally:
                end = datetime.datetime.now()
                if not self.options['no-clean']:
                    log.notice('. Cleaning: %s' % (self._build_dir(arch, bsp, bs)))
                    path.removeall(self._build_dir(arch, bsp, bs))
            log.notice('^ Time %s' % (str(end - start)))
            self.warnings_errors.clear_build()
        bsp_end = datetime.datetime.now()
        log.notice('^ BSP Time %s' % (str(bsp_end - bsp_start)))
        log.output('Failure Report:')
        log.output(self.failures_report(fails))
        os.environ['PATH'] = env_path

    def build_bsp(self, arch, bsp):
        self.build_arch_bsp(arch, bsp)
        self._finished()

    def build_arch(self, arch):
        start = datetime.datetime.now()
        log.output('=' * 70)
        log.notice(']] Architecture: %s' % (arch))
        if not self.config.arch_present(arch):
            raise error.general('Architecture not found: %s' % (arch))
        for bsp in self._bsps(arch):
            self.build_arch_bsp(arch, bsp)
        end = datetime.datetime.now()
        log.notice('^ Architecture Time %s' % (str(end - start)))
        self._finished()

    def build(self):
        for arch in self.config.archs():
            self.build_arch(arch)
        log.notice('^ Profile Time %s' % (str(end - start)))
        self._finished()

    def build_profile(self, profile):
        if not self.config.profile_present(profile):
            raise error.general('Profile not found: %s' % (profile))
        start = datetime.datetime.now()
        log.notice(']] Profile: %s' % (profile))
        for arch in self.config.profile_archs(profile):
            for bsp in self.config.profile_arch_bsps(profile, arch):
                self.build_arch_bsp(arch, bsp)
        end = datetime.datetime.now()
        log.notice('^ Profile Time %s' % (str(end - start)))
        self._finished()

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
        logf = 'bsp-build-%s.txt' % \
               (datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
        config_file = path.join(top, 'share', 'rtems', 'tester',
                                'rtems', 'rtems-bsps.ini')
        if not path.exists(config_file):
            config_file = path.join(top, 'tester', 'rtems', 'rtems-bsps.ini')

        argsp = argparse.ArgumentParser()
        argsp.add_argument('--prefix', help = 'Prefix to build the BSP.',
                           type = str)
        argsp.add_argument('--rtems-tools', help = 'The RTEMS tools directory.',
                           type = str)
        argsp.add_argument('--rtems', help = 'The RTEMS source tree.',
                           type = str)
        argsp.add_argument('--config-report', help = 'Report the configuration.',
                           action = 'store_true')
        argsp.add_argument('--warnings-report', help = 'Report the warnings to a file.',
                           type = str, default = None)
        argsp.add_argument('--build-path', help = 'Path to build in.',
                           type = str)
        argsp.add_argument('--log', help = 'Log file.', type = str)
        argsp.add_argument('--stop-on-error', help = 'Stop on an error.',
                           action = 'store_true')
        argsp.add_argument('--no-clean', help = 'Do not clean the build output.',
                           action = 'store_true')
        argsp.add_argument('--profiles', help = 'Build the listed profiles.',
                           type = str, default = 'tier-1')
        argsp.add_argument('--build', help = 'Build name to build.',
                           type = str, default='all')
        argsp.add_argument('--arch', help = 'Build the specific architecture.',
                           type = str)
        argsp.add_argument('--bsp', help = 'Build the specific BSP.',
                           type = str)
        argsp.add_argument('--jobs', help = 'Number of jobs to run.',
                           type = int, default = host.cpus())
        argsp.add_argument('--dry-run', help = 'Do not run the actual builds.',
                           action = 'store_true')

        opts = argsp.parse_args(args[1:])
        if opts.log is not None:
            logf = opts.log
        log.default = log.log([logf])
        log.notice(title())
        log.output(command_line())
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
        config.load(config_file, opts.build)

        if opts.config_report:
            log.notice('Configuration Report:')
            log.notice(config.report())
            sys.exit(0)

        options = { 'stop-on-error'   : opts.stop_on_error,
                    'no-clean'        : opts.no_clean,
                    'dry-run'         : opts.dry_run,
                    'jobs'            : opts.jobs,
                    'warnings-report' : opts.warnings_report }

        b = build(config, rtems_version(), prefix, tools,
                  path.shell(opts.rtems), build_dir, options)
        if opts.arch is not None:
            if opts.bsp is not None:
                b.build_bsp(opts.arch, opts.bsp)
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
