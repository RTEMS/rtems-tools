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
# This code is based on a tool I wrote to parse RPM spec files in the RTEMS
# project. This is now a configuration file format that has moved away from the
# spec file format to support the specific needs of cross-compiling GCC. This
# module parses a configuration file into Python data types that can be used by
# other software modules.
#

from __future__ import print_function

import copy
import functools
import os
import re
import sys

from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import host
from rtemstoolkit import log
from rtemstoolkit import options
from rtemstoolkit import path


def _check_bool(value):
    if value.isdigit():
        if int(value) == 0:
            istrue = False
        else:
            istrue = True
    else:
        istrue = None
    return istrue


class file(object):
    """Parse a config file."""

    def __init__(self, name, opts, macros=None, directives=None, ignores=None):
        self.opts = opts
        if macros is None:
            self.macros = opts.defaults
        else:
            self.macros = macros
        self.init_name = name
        self.directives = ['%include']
        if directives:
            self.directives += directives
        self.ignores = ignores
        log.trace('config: %s' % (name))
        self.disable_macro_reassign = False
        self.configpath = []
        self.wss = re.compile(r'\s+')
        self.tags = re.compile(r':+')
        self.sf = re.compile(r'%\([^\)]+\)')
        for arg in self.opts.args:
            if arg.startswith('--with-') or arg.startswith('--without-'):
                label = arg[2:].lower().replace('-', '_')
                self.macros.define(label)
        self._includes = []
        self.load_depth = 0
        self.lc = 0
        self.name = 'none'

    def __del__(self):
        pass

    def __str__(self):

        def _dict(dd):
            s = ''
            ddl = list(dd.keys())
            ddl.sort()
            for d in ddl:
                s += '  ' + d + ': ' + dd[d] + '\n'
            return s

        s = 'config: %s' % ('.'.join(self.configpath)) + \
            '\n' + str(self.opts) + \
            '\nlines parsed: %d' % (self.lc) + \
            '\nname: ' + self.name + \
            '\nmacros:\n' + str(self.macros)
        return s

    def _name_line_msg(self, msg):
        return '%s:%d: %s' % (path.basename(self.init_name), self.lc, msg)

    def _output(self, text):
        if not self.opts.quiet():
            log.output(text)

    def _error(self, msg):
        err = 'error: %s' % (self._name_line_msg(msg))
        log.stderr(err)
        log.output(err)
        self.in_error = True
        if not self.opts.dry_run():
            log.stderr('warning: switched to dry run due to errors')
            self.opts.set_dry_run()

    def _label(self, name):
        if name.startswith('%{') and name[-1] == '}':
            return name
        return '%{' + name.lower() + '}'

    def _macro_split(self, s):
        '''Split the string (s) up by macros. Only split on the
           outter level. Nested levels will need to split with futher calls.'''
        trace_me = False
        if trace_me:
            print('------------------------------------------------------')
        macros = []
        nesting = []
        has_braces = False
        c = 0
        while c < len(s):
            if trace_me:
                print('ms:', c, '"' + s[c:] + '"', has_braces, len(nesting),
                      nesting)
            #
            # We need to watch for shell type variables or the form '${var}' because
            # they can upset the brace matching.
            #
            if s[c] == '%' or s[c] == '$':
                start = s[c]
                c += 1
                if c == len(s):
                    continue
                #
                # Do we have '%%' or '%(' or '$%' or '$(' or not '${' ?
                #
                if s[c] == '%' or s[c] == '(' or (start == '$'
                                                  and s[c] != '{'):
                    continue
                elif not s[c].isspace():
                    #
                    # If this is a shell macro and we are at the outter
                    # level or is '$var' forget it and move on.
                    #
                    if start == '$' and (s[c] != '{' or len(nesting) == 0):
                        continue
                    if s[c] == '{':
                        this_has_braces = True
                    else:
                        this_has_braces = False
                    nesting.append((c - 1, has_braces))
                    has_braces = this_has_braces
            elif len(nesting) > 0:
                if s[c] == '}' or (s[c].isspace() and not has_braces):
                    #
                    # Can have '%{?test: something %more}' where the
                    # nested %more ends with the '}' which also ends
                    # the outter macro.
                    #
                    if not has_braces:
                        if s[c] == '}':
                            macro_start, has_braces = nesting[len(nesting) - 1]
                            nesting = nesting[:-1]
                            if len(nesting) == 0:
                                macros.append(s[macro_start:c].strip())
                    if len(nesting) > 0:
                        macro_start, has_braces = nesting[len(nesting) - 1]
                        nesting = nesting[:-1]
                        if len(nesting) == 0:
                            macros.append(s[macro_start:c + 1].strip())
            c += 1
        if trace_me:
            print('ms:', macros)
        if trace_me:
            print('-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=')
        return macros

    def _shell(self, line):
        sl = self.sf.findall(line)
        if len(sl):
            e = execute.capture_execution()
            for s in sl:
                if host.is_windows:
                    cmd = '%s -c "%s"' % (self.macros.expand('%{__sh}'),
                                          s[2:-1])
                else:
                    cmd = s[2:-1]
                exit_code, proc, output = e.shell(cmd)
                if exit_code == 0:
                    line = line.replace(s, output)
                else:
                    raise error.general('shell macro failed: %s:%d: %s' %
                                        (s, exit_code, output))
        return line

    def _expand(self, s):
        expand_count = 0
        expanded = True
        while expanded:
            expand_count += 1
            if expand_count > 500:
                raise error.general('macro expand looping: %s' % (s))
            expanded = False
            ms = self._macro_split(s)
            for m in ms:
                mn = m
                #
                # A macro can be '%{macro}' or '%macro'. Turn the later into
                # the former.
                #
                show_warning = True
                if mn[1] != '{':
                    if self.ignores is not None:
                        for r in self.ignores:
                            if r.match(mn) is not None:
                                mn = None
                                break
                        else:
                            mn = self._label(mn[1:])
                            show_warning = False
                    else:
                        mn = self._label(mn[1:])
                        show_warning = False
                elif m.startswith('%{expand'):
                    colon = m.find(':')
                    if colon < 8:
                        log.warning('malformed expand macro, no colon found')
                    else:
                        e = self._expand(m[colon + 1:-1].strip())
                        s = s.replace(m, e)
                        expanded = True
                        mn = None
                elif m.startswith('%{with '):
                    #
                    # Change the ' ' to '_' because the macros have no spaces.
                    #
                    n = self._label('with_' + m[7:-1].strip())
                    if n in self.macros:
                        s = s.replace(m, '1')
                    else:
                        s = s.replace(m, '0')
                    expanded = True
                    mn = None
                elif m.startswith('%{echo'):
                    if not m.endswith('}'):
                        log.warning("malformed conditional macro '%s'" % (m))
                        mn = None
                    else:
                        e = self._expand(m[6:-1].strip())
                        log.output('%s' % (self._name_line_msg(e)))
                        s = ''
                        expanded = True
                        mn = None
                elif m.startswith('%{defined'):
                    n = self._label(m[9:-1].strip())
                    if n in self.macros:
                        s = s.replace(m, '1')
                    else:
                        s = s.replace(m, '0')
                    expanded = True
                    mn = None
                elif m.startswith('%{?') or m.startswith('%{!?'):
                    if m[2] == '!':
                        start = 4
                    else:
                        start = 3
                    colon = m[start:].find(':')
                    if colon < 0:
                        if not m.endswith('}'):
                            log.warning("malformed conditional macro '%s'" %
                                        (m))
                            mn = None
                        else:
                            mn = self._label(m[start:-1])
                    else:
                        mn = self._label(m[start:start + colon])
                    if mn:
                        if m.startswith('%{?'):
                            istrue = False
                            if mn in self.macros:
                                # If defined and 0 then it is false.
                                istrue = _check_bool(self.macros[mn])
                                if istrue is None:
                                    istrue = True
                            if colon >= 0 and istrue:
                                s = s.replace(m, m[start + colon + 1:-1])
                                expanded = True
                                mn = None
                            elif not istrue:
                                mn = '%{nil}'
                        else:
                            isfalse = True
                            if mn in self.macros:
                                istrue = _check_bool(self.macros[mn])
                                if istrue is None or istrue == True:
                                    isfalse = False
                            if colon >= 0 and isfalse:
                                s = s.replace(m, m[start + colon + 1:-1])
                                expanded = True
                                mn = None
                            else:
                                mn = '%{nil}'
                if mn:
                    if mn.lower() in self.macros:
                        s = s.replace(m, self.macros[mn.lower()])
                        expanded = True
                    elif show_warning:
                        self._error("macro '%s' not found" % (mn))
        return self._shell(s)

    def _disable(self, config, ls):
        if len(ls) != 2:
            log.warning('invalid disable statement')
        else:
            if ls[1] == 'select':
                self.macros.lock_read_map()
                log.trace('config: %s: _disable_select: %s' %
                          (self.init_name, ls[1]))
            else:
                log.warning('invalid disable statement: %s' % (ls[1]))

    def _select(self, config, ls):
        if len(ls) != 2:
            log.warning('invalid select statement')
        else:
            r = self.macros.set_read_map(ls[1])
            log.trace('config: %s: _select: %s %s %r' % \
                          (self.init_name, r, ls[1], self.macros.maps()))

    def _define(self, config, ls):
        if len(ls) <= 1:
            log.warning('invalid macro definition')
        else:
            d = self._label(ls[1])
            if self.disable_macro_reassign:
                if (d not in self.macros) or \
                        (d in self.macros and len(self.macros[d]) == 0):
                    if len(ls) == 2:
                        self.macros[d] = '1'
                    else:
                        self.macros[d] = ' '.join([f.strip() for f in ls[2:]])
                else:
                    log.warning("macro '%s' already defined" % (d))
            else:
                if len(ls) == 2:
                    self.macros[d] = '1'
                else:
                    self.macros[d] = ' '.join([f.strip() for f in ls[2:]])

    def _undefine(self, config, ls):
        if len(ls) <= 1:
            log.warning('invalid macro definition')
        else:
            mn = self._label(ls[1])
            if mn in self.macros:
                del self.macros[mn]
            else:
                log.warning("macro '%s' not defined" % (mn))

    def _ifs(self, config, ls, label, iftrue, isvalid, dir, info):
        in_iftrue = True
        data = []
        while True:
            if isvalid and \
                    ((iftrue and in_iftrue) or (not iftrue and not in_iftrue)):
                this_isvalid = True
            else:
                this_isvalid = False
            r = self._parse(config, dir, info, roc=True, isvalid=this_isvalid)
            if r[0] == 'control':
                if r[1] == '%end':
                    self._error(label + ' without %endif')
                    raise error.general('terminating build')
                if r[1] == '%endif':
                    log.trace('config: %s: _ifs: %s %s' %
                              (self.init_name, r[1], this_isvalid))
                    return data
                if r[1] == '%else':
                    in_iftrue = False
            elif r[0] == 'directive':
                if this_isvalid:
                    if r[1] == '%include':
                        self.load(r[2][0])
                        continue
                    dir, info, data = self._process_directive(
                        r, dir, info, data)
            elif r[0] == 'data':
                if this_isvalid:
                    dir, info, data = self._process_data(r, dir, info, data)
            else:
                dir, info, data = self._process_block(r, dir, info, data)

        # @note is a directive extend missing

    def _if(self, config, ls, isvalid, dir, info, invert=False):

        def add(x, y):
            return x + ' ' + str(y)

        istrue = False
        if isvalid:
            if len(ls) == 2:
                s = ls[1]
            else:
                s = (ls[1] + ' ' + ls[2])
            ifls = s.split()
            if len(ifls) == 1:
                #
                # Check if '%if %{x} == %{nil}' has both parts as nothing
                # which means '%if ==' is always True and '%if !=' is always false.
                #
                if ifls[0] == '==':
                    istrue = True
                elif ifls[0] == '!=':
                    istrue = False
                else:
                    istrue = _check_bool(ifls[0])
                    if istrue == None:
                        self._error('invalid if bool value: ' +
                                    functools.reduce(add, ls, ''))
                        istrue = False
            elif len(ifls) == 2:
                if ifls[0] == '!':
                    istrue = _check_bool(ifls[1])
                    if istrue == None:
                        self._error('invalid if bool value: ' +
                                    functools.reduce(add, ls, ''))
                        istrue = False
                    else:
                        istrue = not istrue
                else:
                    #
                    # Check is something is being checked against empty,
                    #   ie '%if %{x} == %{nil}'
                    # The logic is 'something == nothing' is False and
                    # 'something != nothing' is True.
                    #
                    if ifls[1] == '==':
                        istrue = False
                    elif ifls[1] == '!=':
                        istrue = True
                    else:
                        self._error('invalid if bool operator: ' +
                                    functools.reduce(add, ls, ''))
            elif len(ifls) == 3:
                if ifls[1] == '==':
                    if ifls[0] == ifls[2]:
                        istrue = True
                    else:
                        istrue = False
                elif ifls[1] == '!=' or ifls[1] == '=!':
                    if ifls[0] != ifls[2]:
                        istrue = True
                    else:
                        istrue = False
                elif ifls[1] == '>':
                    if ifls[0] > ifls[2]:
                        istrue = True
                    else:
                        istrue = False
                elif ifls[1] == '>=' or ifls[1] == '=>':
                    if ifls[0] >= ifls[2]:
                        istrue = True
                    else:
                        istrue = False
                elif ifls[1] == '<=' or ifls[1] == '=<':
                    if ifls[0] <= ifls[2]:
                        istrue = True
                    else:
                        istrue = False
                elif ifls[1] == '<':
                    if ifls[0] < ifls[2]:
                        istrue = True
                    else:
                        istrue = False
                else:
                    self._error('invalid %if operator: ' +
                                functools.reduce(add, ls, ''))
            else:
                self._error('malformed if: ' + functools.reduce(add, ls, ''))
            if invert:
                istrue = not istrue
            log.trace('config: %s: _if:  %s %s' %
                      (self.init_name, ifls, str(istrue)))
        return self._ifs(config, ls, '%if', istrue, isvalid, dir, info)

    def _ifos(self, config, ls, isvalid, dir, info):
        isos = False
        if isvalid:
            os = self.define('_os')
            for l in ls:
                if l in os:
                    isos = True
                    break
        return self._ifs(config, ls, '%ifos', isos, isvalid, dir, info)

    def _ifarch(self, config, positive, ls, isvalid, dir, info):
        isarch = False
        if isvalid:
            arch = self.define('_arch')
            for l in ls:
                if l in arch:
                    isarch = True
                    break
        if not positive:
            isarch = not isarch
        return self._ifs(config, ls, '%ifarch', isarch, isvalid, dir, info)

    def _parse(self, config, dir, info, roc=False, isvalid=True):
        # roc = return on control

        def _clean(line):
            line = line[0:-1]
            b = line.find('#')
            if b >= 0:
                line = line[1:b]
            return line.strip()

        #
        # Need to add code to count matching '{' and '}' and if they
        # do not match get the next line and add to the string until
        # they match. This closes an opening '{' that is on another
        # line.
        #
        for l in config:
            self.lc += 1
            l = _clean(l)
            if len(l) == 0:
                continue
            log.trace('config: %s: %03d: %s %s' % \
                          (self.init_name, self.lc, str(isvalid), l))
            lo = l
            if isvalid:
                l = self._expand(l)
            if len(l) == 0:
                continue
            if l[0] == '%':
                ls = self.wss.split(l, 2)
                los = self.wss.split(lo, 2)
                if ls[0] == '%disable':
                    if isvalid:
                        self._disable(config, ls)
                elif ls[0] == '%select':
                    if isvalid:
                        self._select(config, ls)
                elif ls[0] == '%error':
                    if isvalid:
                        return ('data',
                                ['%%error %s' % (self._name_line_msg(l[7:]))])
                elif ls[0] == '%warning':
                    if isvalid:
                        return ('data', [
                            '%%warning %s' % (self._name_line_msg(l[9:]))
                        ])
                elif ls[0] == '%define' or ls[0] == '%global':
                    if isvalid:
                        self._define(config, ls)
                elif ls[0] == '%undefine':
                    if isvalid:
                        self._undefine(config, ls)
                elif ls[0] == '%if':
                    d = self._if(config, ls, isvalid, dir, info)
                    if len(d):
                        log.trace('config: %s: %%if: %s' % (self.init_name, d))
                        return ('data', d)
                elif ls[0] == '%ifn':
                    d = self._if(config, ls, isvalid, dir, info, True)
                    if len(d):
                        log.trace('config: %s: %%ifn: %s' %
                                  (self.init_name, d))
                        return ('data', d)
                elif ls[0] == '%ifos':
                    d = self._ifos(config, ls, isvalid, dir, info)
                    if len(d):
                        return ('data', d)
                elif ls[0] == '%ifarch':
                    d = self._ifarch(config, True, ls, isvalid, dir, info)
                    if len(d):
                        return ('data', d)
                elif ls[0] == '%ifnarch':
                    d = self._ifarch(config, False, ls, isvalid, dir, info)
                    if len(d):
                        return ('data', d)
                elif ls[0] == '%endif':
                    if roc:
                        return ('control', '%endif', '%endif')
                    log.warning("unexpected '" + ls[0] + "'")
                elif ls[0] == '%else':
                    if roc:
                        return ('control', '%else', '%else')
                    log.warning("unexpected '" + ls[0] + "'")
                elif ls[0].startswith('%defattr'):
                    return ('data', [l])
                elif ls[0] == '%bcond_with':
                    if isvalid:
                        #
                        # Check if already defined. Would be by the command line or
                        # even a host specific default.
                        #
                        if self._label('with_' + ls[1]) not in self.macros:
                            self._define(config, (ls[0], 'without_' + ls[1]))
                elif ls[0] == '%bcond_without':
                    if isvalid:
                        if self._label('without_' + ls[1]) not in self.macros:
                            self._define(config, (ls[0], 'with_' + ls[1]))
                else:
                    pt = self._parse_token(lo, los, l, ls)
                    if pt is not None:
                        return pt
                    if self.ignores is not None:
                        for r in self.ignores:
                            if r.match(ls[0]) is not None:
                                return ('data', [l])
                    if isvalid:
                        for d in self.directives:
                            if ls[0].strip() == d:
                                return ('directive', ls[0].strip(), ls[1:])
                        log.warning("unknown directive: '" + ls[0] + "'")
                        return ('data', [lo])
            else:
                return ('data', [lo])
        return ('control', '%end', '%end')

    def _parse_token(self, line, line_split, line_expanded,
                     line_split_expanded):
        return None

    def _process_directive(self, results, directive, info, data):
        new_data = []
        if results[1] == '%description':
            new_data = [' '.join(results[2])]
        else:
            directive, into, data = self._directive_filter(
                results, directive, info, data)
        if directive and directive != results[1]:
            self._directive_extend(directive, data)
        directive = results[1]
        data = new_data
        return (directive, info, data)

    def _process_data(self, results, directive, info, data):
        new_data = []
        for l in results[1]:
            if l.startswith('%error'):
                l = self._expand(l)
                raise error.general('config error: %s' % (l[7:]))
            elif l.startswith('%warning'):
                l = self._expand(l)
                log.stderr('warning: %s' % (l[9:]))
                log.warning(l[9:])
            if not directive:
                l = self._expand(l)
                ls = self.tags.split(l, 1)
                log.trace('config: %s: _tag: %s %s' % (self.init_name, l, ls))
                if len(ls) > 1:
                    info = ls[0].lower()
                    if info[-1] == ':':
                        info = info[:-1]
                    info_data = ls[1].strip()
                else:
                    info_data = ls[0].strip()
                if info is not None:
                    self._info_append(info, info_data)
                else:
                    log.warning("invalid format: '%s'" % (info_data[:-1]))
            else:
                log.trace('config: %s: _data: %s %s' %
                          (self.init_name, l, new_data))
                new_data.append(l)
        return (directive, info, data + new_data)

    def _process_block(self, results, directive, info, data):
        raise error.internal('known block type: %s' % (results[0]))

    def _directive_extend(self, dir, data):
        pass

    def _directive_filter(self, results, directive, info, data):
        return directive, into, data

    def _info_append(self, info, data):
        pass

    def load(self, name):

        def common_end(left, right):
            end = ''
            while len(left) and len(right):
                if left[-1] != right[-1]:
                    return end
                end = left[-1] + end
                left = left[:-1]
                right = right[:-1]
            return end

        if self.load_depth == 0:
            self.in_error = False
            self.lc = 0
            self.name = name
            self.conditionals = {}

        self.load_depth += 1

        save_name = self.name
        save_lc = self.lc

        self.name = name
        self.lc = 0

        #
        # Locate the config file. Expand any macros then add the
        # extension. Check if the file exists, therefore directly
        # referenced. If not see if the file contains ':' or the path
        # separator. If it does split the path else use the standard config dir
        # path in the defaults.
        #
        exname = self.expand(name)

        #
        # Macro could add an extension.
        #
        if exname.endswith('.cfg'):
            configname = exname
        else:
            configname = '%s.cfg' % (exname)
            name = '%s.cfg' % (name)

        if ':' in configname:
            cfgname = path.basename(configname)
        else:
            cfgname = common_end(configname, name)

        if not path.exists(configname):
            if ':' in configname:
                configdirs = path.dirname(configname).split(':')
            else:
                configdirs = self.define('_configdir').split(':')
            for cp in configdirs:
                configname = path.join(path.abspath(cp), cfgname)
                if path.exists(configname):
                    break
                configname = None
            if configname is None:
                raise error.general('no config file found: %s' % (cfgname))

        try:
            log.trace('config: %s: _open: %s' %
                      (self.init_name, path.host(configname)))
            config = open(path.host(configname), 'r')
        except IOError as err:
            raise error.general('error opening config file: %s' %
                                (path.host(configname)) + ': ' + str(err))
        self.configpath += [configname]

        self._includes += [configname]

        try:
            dir = None
            info = None
            data = []
            while True:
                r = self._parse(config, dir, info)
                if r[0] == 'control':
                    if r[1] == '%end':
                        break
                    log.warning("unexpected '%s'" % (r[1]))
                elif r[0] == 'directive':
                    if r[1] == '%include':
                        self.load(r[2][0])
                        continue
                    dir, info, data = self._process_directive(
                        r, dir, info, data)
                elif r[0] == 'data':
                    dir, info, data = self._process_data(r, dir, info, data)
                else:
                    self._error("%d: invalid parse state: '%s" %
                                (self.lc, r[0]))
            if dir is not None:
                self._directive_extend(dir, data)
        except:
            config.close()
            raise

        config.close()

        self.name = save_name
        self.lc = save_lc

        self.load_depth -= 1

    def defined(self, name):
        return self.macros.has_key(name)

    def define(self, name):
        if name in self.macros:
            d = self.macros[name]
        else:
            n = self._label(name)
            if n in self.macros:
                d = self.macros[n]
            else:
                raise error.general('%d: macro "%s" not found' %
                                    (self.lc, name))
        return self._expand(d)

    def set_define(self, name, value):
        self.macros[name] = value

    def expand(self, line):
        if type(line) == list:
            el = []
            for l in line:
                el += [self._expand(l)]
            return el
        return self._expand(line)

    def macro(self, name):
        if name in self.macros:
            return self.macros[name]
        raise error.general('macro "%s" not found' % (name))

    def directive(self, name):
        pass

    def abspath(self, rpath):
        return path.abspath(self.define(rpath))

    def includes(self):
        return self._includes

    def file_name(self):
        return self.init_name
