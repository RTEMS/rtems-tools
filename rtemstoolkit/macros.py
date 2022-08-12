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
# Macro tables.
#

from __future__ import print_function

import copy
import inspect
import re
import os
import string

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import path

#
# Macro tables
#
class macros:

    class macro_iterator:
        def __init__(self, keys):
            self.keys = keys
            self.index = 0

        def __iter__(self):
            return self

        def __next__(self):
            if self.index < len(self.keys):
                key = self.keys[self.index]
                self.index += 1
                return key
            raise StopIteration

        def iterkeys(self):
            return self.keys

    def _unicode_to_str(self, us):
        try:
            if type(us) == unicode:
                return us.encode('ascii', 'replace')
        except:
            pass
        try:
            if type(us) == bytes:
                return us.encode('ascii', 'replace')
        except:
            pass
        return us

    def __init__(self, name = None, original = None, rtdir = '.', show_minimal = False):
        self.files = []
        self.str_show_minimal = show_minimal
        self.macro_filter = re.compile(r'%{[^}]+}')
        if original is None:
            self.macros = {}
            self.read_maps = []
            self.read_map_locked = False
            self.write_map = 'global'
            self.rtpath = path.abspath(path.dirname(inspect.getfile(macros)))
            if path.dirname(self.rtpath).endswith('/share/rtems'):
                self.prefix = path.dirname(self.rtpath)[:-len('/share/rtems')]
            else:
                self.prefix = '.'
            self.macros['global'] = {}
            self.macros['global']['nil'] = ('none', 'none', '')
            self.macros['global']['_cwd'] = ('dir',
                                             'required',
                                             path.abspath(os.getcwd()))
            self.macros['global']['_prefix'] = ('dir', 'required', self.prefix)
            self.macros['global']['_rtdir'] = ('dir',
                                               'required',
                                               path.abspath(self.expand(rtdir)))
            self.macros['global']['_rttop'] = ('dir', 'required', self.prefix)
        else:
            self.macros = {}
            for m in original.macros:
                if m not in self.macros:
                    self.macros[m] = {}
                for k in original.macros[m]:
                    self.macros[m][k] = copy.copy(original.macros[m][k])
            self.read_maps = sorted(copy.copy(original.read_maps))
            self.read_map_locked = copy.copy(original.read_map_locked)
            self.write_map = copy.copy(original.write_map)
        if name is not None:
            self.load(name)

    def __copy__(self):
        return macros(original = self)

    def __str__(self):
        text_len = 80
        text = ''
        for f in self.files:
            text += '> %s%s' % (f, os.linesep)
        max_key_size = 0
        for map in self.macros:
            size = len(max(self.macros[map].keys(), key = lambda x: len(x)))
            if size > max_key_size:
                max_key_size = size
        maps = sorted(self.macros)
        maps.remove('global')
        for map in ['global'] + maps:
            rm = '-'
            for rmap in self.read_maps:
                if rmap[4:] == '___%s' % (map):
                    if self.read_map_locked:
                        rm = 'R[%s]' % (rmap[:4])
                    else:
                        rm = 'r[%s]' % (rmap[:4])
                    break
            if map == self.write_map:
                wm = 'w'
            else:
                wm = '-'
            text += '[%s] %s,%s%s' % (map, wm, rm, os.linesep)
            for k in sorted(self.macros[map].keys()):
                d = self.macros[map][k]
                if self.str_show_minimal:
                    text += " %s: %s" % (k, ' ' * (max_key_size - len(k)))
                else:
                    text += " %s:%s '%s'%s '%s'%s" % \
                            (k, ' ' * (max_key_size - len(k)),
                             d[0], ' ' * (8 - len(d[0])),
                             d[1], ' ' * (10 - len(d[1])))
                if len(d[2]) == 0:
                    text += "''%s" % (os.linesep)
                else:
                    if '\n' in d[2]:
                        text += "'''"
                    else:
                        text += "'"
                ds = d[2].split('\n')
                lc = 0
                indent = False
                for l in ds:
                    lc += 1
                    while len(l):
                        if indent:
                            if self.str_show_minimal:
                                text += ' %s  ' % (' ' * max_key_size)
                            else:
                                text += ' %s  %10s %12s' % (' ' * max_key_size, ' ', ' ')
                        indent = True
                        text += l[0:text_len]
                        l = l[text_len:]
                        if len(l):
                            text += ' \\'
                        elif lc == len(ds):
                            if len(ds) > 1:
                                text += "'''"
                            else:
                                text += "'"
                        text += '%s' % (os.linesep)
        return text

    def __iter__(self):
        return macros.macro_iterator(self.keys())

    def __getitem__(self, key):
        macro = self.get(key)
        if macro is None or macro[1] == 'undefine':
            raise IndexError('key: %s' % (key))
        return macro[2]

    def __setitem__(self, key, value):
        key = self._unicode_to_str(key)
        if type(key) is not str:
            raise TypeError('bad key type (want str): %s' % (type(key)))
        if type(value) is not tuple:
            value = self._unicode_to_str(value)
        if type(value) is str:
            value = ('none', 'none', value)
        if type(value) is not tuple:
            raise TypeError('bad value type (want tuple): %s' % (type(value)))
        if len(value) != 3:
            raise TypeError('bad value tuple (len not 3): %d' % (len(value)))
        value = (self._unicode_to_str(value[0]),
                 self._unicode_to_str(value[1]),
                 self._unicode_to_str(value[2]))
        if type(value[0]) is not str:
            raise TypeError('bad value tuple type field: %s' % (type(value[0])))
        if type(value[1]) is not str:
            raise TypeError('bad value tuple attrib field: %s' % (type(value[1])))
        if type(value[2]) is not str:
            raise TypeError('bad value tuple value field: %s' % (type(value[2])))
        if value[0] not in ['none', 'triplet', 'dir', 'file', 'exe']:
            raise TypeError('bad value tuple (type field): %s' % (value[0]))
        if value[1] not in ['none', 'optional', 'required',
                            'override', 'undefine', 'convert']:
            raise TypeError('bad value tuple (attrib field): %s' % (value[1]))
        if value[1] == 'convert':
            value = self.expand(value)
        self.macros[self.write_map][self.key_filter(key)] = value

    def __delitem__(self, key):
        self.undefine(key)

    def __contains__(self, key):
        return self.has_key(key)

    def __len__(self):
        return len(list(self.keys()))

    def keys(self):
        keys = list(self.macros['global'].keys())
        for rm in self.get_read_maps():
            for mk in self.macros[rm]:
                if self.macros[rm][mk][1] == 'undefine':
                    if mk in keys:
                        keys.remove(mk)
                else:
                    keys.append(mk)
        return sorted(set(keys))

    def has_key(self, key):
        key = self._unicode_to_str(key)
        if type(key) is not str:
            raise TypeError('bad key type (want str): %s' % (type(key)))
        if self.key_filter(key) not in list(self.keys()):
            return False
        return True

    def maps(self):
        return self.macros.keys()

    def get_read_maps(self):
        return [rm[7:] for rm in self.read_maps]

    def key_filter(self, key):
        if key.startswith('%{') and key[-1] == '}':
            key = key[2:-1]
        return key.lower()

    def parse(self, lines):

        def _clean(l):
            if '#' in l:
                l = l[:l.index('#')]
            if '\r' in l:
                l = l[:l.index('r')]
            if '\n' in l:
                l = l[:l.index('\n')]
            return l.strip()

        trace_me = False
        if trace_me:
            print('[[[[]]]] parsing macros')
        orig_macros = copy.copy(self.macros)
        map = 'global'
        lc = 0
        state = 'key'
        token = ''
        macro = []
        for l in lines:
            lc += 1
            #print 'l:%s' % (l[:-1])
            if len(l) == 0:
                continue
            l_remaining = l
            for c in l:
                if trace_me:
                    print(']]]]]]]] c:%s(%d) s:%s t:"%s" m:%r M:%s' % \
                        (c, ord(c), state, token, macro, map))
                l_remaining = l_remaining[1:]
                if c == '#' and not state.startswith('value'):
                    break
                if c == '\n' or c == '\r':
                    if not (state == 'key' and len(token) == 0) and \
                            not state.startswith('value-multiline'):
                        self.macros = orig_macros
                        raise error.general('malformed macro line:%d: %s' % (lc, l))
                if state == 'key':
                    if c not in string.whitespace:
                        if c == '[':
                            state = 'map'
                        elif c == '%':
                            state = 'directive'
                        elif c == ':':
                            macro += [token]
                            token = ''
                            state = 'attribs'
                        elif c == '#':
                            break
                        else:
                            token += c
                elif state == 'map':
                    if c == ']':
                        if token not in self.macros:
                            self.macros[token] = {}
                        map = token
                        token = ''
                        state = 'key'
                    elif c in string.printable and c not in string.whitespace:
                        token += c
                    else:
                        self.macros = orig_macros
                        raise error.general('invalid macro map:%d: %s' % (lc, l))
                elif state == 'directive':
                    if c in string.whitespace:
                        if token == 'include':
                            self.load(_clean(l_remaining))
                            token = ''
                            state = 'key'
                            break
                    elif c in string.printable and c not in string.whitespace:
                        token += c
                    else:
                        self.macros = orig_macros
                        raise error.general('invalid macro directive:%d: %s' % (lc, l))
                elif state == 'include':
                    if c is string.whitespace:
                        if token == 'include':
                            state = 'include'
                    elif c in string.printable and c not in string.whitespace:
                        token += c
                    else:
                        self.macros = orig_macros
                        raise error.general('invalid macro directive:%d: %s' % (lc, l))
                elif state == 'attribs':
                    if c not in string.whitespace:
                        if c == ',':
                            macro += [token]
                            token = ''
                            if len(macro) == 3:
                                state = 'value-start'
                        else:
                            token += c
                elif state == 'value-start':
                    if c == "'":
                        state = 'value-line-start'
                elif state == 'value-line-start':
                    if c == "'":
                        state = 'value-multiline-start'
                    else:
                        state = 'value-line'
                        token += c
                elif state == 'value-multiline-start':
                    if c == "'":
                        state = 'value-multiline'
                    else:
                        macro += [token]
                        state = 'macro'
                elif state == 'value-line':
                    if c == "'":
                        macro += [token]
                        state = 'macro'
                    else:
                        token += c
                elif state == 'value-multiline':
                    if c == "'":
                        state = 'value-multiline-end'
                    else:
                        token += c
                elif state == 'value-multiline-end':
                    if c == "'":
                        state = 'value-multiline-end-end'
                    else:
                        state = 'value-multiline'
                        token += "'" + c
                elif state == 'value-multiline-end-end':
                    if c == "'":
                        macro += [token]
                        state = 'macro'
                    else:
                        state = 'value-multiline'
                        token += "''" + c
                else:
                    self.macros = orig_macros
                    raise error.internal('bad state: %s' % (state))
                if state == 'macro':
                    self.macros[map][macro[0].lower()] = (macro[1], macro[2], macro[3])
                    macro = []
                    token = ''
                    state = 'key'

    def load(self, name):
        names = self.expand(name).split(':')
        for n in names:
            log.trace('opening: %s' % (n))
            if path.exists(n):
                try:
                    mc = open(path.host(n), 'r')
                    macros = self.parse(mc)
                    mc.close()
                    self.files += [n]
                    return
                except IOError as err:
                    pass
        raise error.general('opening macro file: %s' % \
                                (path.host(self.expand(name))))

    def get(self, key):
        key = self._unicode_to_str(key)
        if type(key) is not str:
            raise TypeError('bad key type: %s' % (type(key)))
        key = self.key_filter(key)
        for rm in self.get_read_maps():
            if key in self.macros[rm]:
                return self.macros[rm][key]
        if key in self.macros['global']:
            return self.macros['global'][key]
        return None

    def get_type(self, key):
        m = self.get(key)
        if m is None:
            return None
        return m[0]

    def get_attribute(self, key):
        m = self.get(key)
        if m is None:
            return None
        return m[1]

    def get_value(self, key):
        m = self.get(key)
        if m is None:
            return None
        return m[2]

    def overridden(self, key):
        return self.get_attribute(key) == 'override'

    def define(self, key, value = '1'):
        if type(key) is not str:
            raise TypeError('bad key type: %s' % (type(key)))
        self.__setitem__(key, ('none', 'none', value))

    def undefine(self, key):
        if type(key) is not str:
            raise TypeError('bad key type: %s' % (type(key)))
        key = self.key_filter(key)
        for map in self.macros:
            if key in self.macros[map]:
                del self.macros[map][key]

    def expand(self, _str):
        """Simple basic expander of config file macros."""
        start_str = _str
        expanded = True
        count = 0
        while expanded:
            count += 1
            if count > 1000:
                raise error.general('expansion looped over 1000 times "%s"' %
                                    (start_str))
            expanded = False
            for m in self.macro_filter.findall(_str):
                name = m[2:-1]
                macro = self.get(name)
                if macro is None:
                    raise error.general('cannot expand default macro: %s in "%s"' %
                                        (m, _str))
                _str = _str.replace(m, macro[2])
                expanded = True
        return _str

    def find(self, regex):
        what = re.compile(regex)
        keys = []
        for key in self.keys():
            if what.match(key):
                keys += [key]
        return keys

    def set_read_map(self, _map):
        if not self.read_map_locked:
            if _map in self.macros:
                if _map not in self.get_read_maps():
                    rm = '%04d___%s' % (len(self.read_maps), _map)
                    self.read_maps = sorted(self.read_maps + [rm])
                return True
        return False

    def unset_read_map(self, _map):
        if not self.read_map_locked:
            if _map in self.get_read_maps():
                for i in range(0, len(self.read_maps)):
                    if '%04d___%s' % (i, _map) == self.read_maps[i]:
                        self.read_maps.pop(i)
                return True
        return False

    def set_write_map(self, _map, add = False):
        if _map in self.macros:
            self.write_map = _map
            return True
        elif add:
            self.write_map = _map
            self.macros[_map] = {}
            return True
        return False

    def unset_write_map(self):
        self.write_map = 'global'
        return True

    def lock_read_map(self):
        self.read_map_locked = True

    def unlock_read_map(self):
        self.read_map_locked = False

if __name__ == "__main__":
    import copy
    import sys
    print(inspect.getfile(macros))
    m = macros()
    d = copy.copy(m)
    m['test1'] = 'something'
    if d.has_key('test1'):
        print('error: copy failed.')
        sys.exit(1)
    m.parse("[test]\n" \
            "test1: none, undefine, ''\n" \
            "name:  none, override, 'pink'\n")
    print('set test:', m.set_read_map('test'))
    if m['name'] != 'pink':
        print('error: override failed. name is %s' % (m['name']))
        sys.exit(1)
    if m.has_key('test1'):
        print('error: map undefine failed.')
        sys.exit(1)
    print('unset test:', m.unset_read_map('test'))
    print(m)
    print(m.keys())
