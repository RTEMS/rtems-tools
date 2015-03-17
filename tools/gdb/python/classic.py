# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2014 Chris Johns (chrisj@rtems.org)
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
# RTEMS Classic API Support
#

import gdb
import itertools
import re
#ToDo This shouldn't be here
import helper

import heaps
import mutex
import objects
import supercore
import threads
import watchdog

class attribute:
    """The Classic API attribute."""

    groups = {
        'none' : [],
        'all' : ['scope',
                 'priority',
                 'fpu',
                 'semaphore-type',
                 'semaphore-pri',
                 'semaphore-pri-ceiling',
                 'barrier',
                 'task'],
        'task' : ['scope',
                  'priority',
                  'fpu',
                  'task'],
        'semaphore' : ['scope',
                       'priority',
                       'semaphore-type',
                       'semaphore-pri',
                       'semaphore-pri-ceiling'],
        'barrier' : ['barrier'],
        'message_queue' : ['priority',
                           'scope'],
        'partition' : ['scope'],
        'region' : ['priority']
        }

    masks = {
        'scope' : 0x00000002,
        'priority' : 0x00000004,
        'fpu' : 0x00000001,
        'semaphore-type' : 0x00000030,
        'semaphore-pri' : 0x00000040,
        'semaphore-pri-ceiling' : 0x00000080,
        'barrier' : 0x00000010,
        'task' : 0x00008000
        }

    fields = {
        'scope' : [(0x00000000, 'local'),
                   (0x00000002, 'global')],
        'priority' : [(0x00000000, 'fifo'),
                      (0x00000004, 'pri')],
        'fpu' : [(0x00000000, 'no-fpu'),
                 (0x00000001, 'fpu')],
        'semaphore-type' : [(0x00000000, 'count-sema'),
                            (0x00000010, 'bin-sema'),
                            (0x00000020, 'simple-bin-sema')],
        'semaphore-pri' : [(0x00000000, 'no-inherit-pri'),
                           (0x00000040, 'inherit-pri')],
        'semaphore-pri-ceiling' : [(0x00000000, 'no-pri-ceiling'),
                                   (0x00000080, 'pri-ceiling')],
        'barrier' : [(0x00000010, 'barrier-auto-release'),
                     (0x00000000, 'barrier-manual-release')],
        'task' : [(0x00000000, 'app-task'),
                  (0x00008000, 'sys-task')]
        }

    def __init__(self, attr, attrtype = 'none'):
        if attrtype not in self.groups:
            raise 'invalid attribute type'
        self.attrtype = attrtype
        self.attr = attr

    #ToDo: Move this out
    def to_string(self):
        s = '(0x%08x) ' % (self.attr)
        if self.attrtype != 'none':
            for m in self.groups[self.attrtype]:
                v = self.attr & self.masks[m]
                for f in self.fields[m]:
                    if f[0] == v:
                        s += f[1] + ','
                        break
        return s[:-1]

    def test(self, mask, value):
        if self.attrtype != 'none' and \
                mask in self.groups[self.attrtype]:
            v = self.masks[mask] & self.attr
            for f in self.fields[mask]:
                if v == f[0] and value == f[1]:
                    return True
        return False


class semaphore:
    "Print a classic semaphore."

    def __init__(self, obj):
        if obj.type.code == gdb.TYPE_CODE_PTR:
            self.reference = obj
            self.object = obj.dereference()
        else:
            self.object = obj
            self.reference = obj.address
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'], 'semaphore')

    def show(self, from_tty):
        if self.object_control.id() != 0:
            print '     Name:', self.object_control.name()
            print '       Id: 0x%08x (@ 0x%08x)' % (self.object_control.id(),
                                                   self.reference)
            print '     Attr:', self.attr.to_string()
            if self.attr.test('semaphore-type', 'bin-sema') or \
               self.attr.test('semaphore-type', 'simple-bin-sema'):
                core_mutex = mutex.control(self.object['Core_control']['mutex'])
                print '  Nesting:', core_mutex.nest_count()
                print '   Holder:',
                holder = core_mutex.holder()
                if holder:
                    print '%s (id 0x%08x)' % (holder.brief(), holder.id())
                else:
                    print 'no holder (unlocked)'
                wait_queue = core_mutex.wait_queue()
                tasks = wait_queue.tasks()
                print '    Queue: len = %d, state = %s' % (len(tasks),
                                                           wait_queue.state())
                if len(tasks) > 0:
                    print '    Tasks:'
                    print '    Name (c:current, r:real), (id)'
                    for t in range(0, len(tasks)):
                        print '      ', tasks[t].brief(), ' (%08x)' % (tasks[t].id())
                return True
        return False

class task:
    "Print a classic task"

    def __init__(self, obj):
        self.reference = obj
        self.object = obj.dereference()
        self.task = threads.control(self.reference)
        self.wait_info = self.task.wait_info()
        self.regs = self.task.registers()
        #self.regs = sparc.register(self.object['Registers'])

    def show(self, from_tty):
        if self.task.id() != 0:
            cpu = self.task.executing()
            if cpu == -1:
                cpu = 'not executing'
            print '         Id:', '0x%08x (@ 0x%08x)' % (self.task.id(),
                                                         self.task.reference)
            print '       Name:', self.task.name()
            print ' Active CPU:', cpu
            print '      State:', self.task.current_state()
            print '    Current:', self.task.current_priority()
            print '       Real:', self.task.real_priority()
            print '    Preempt:', self.task.preemptible()
            print '   T Budget:', self.task.cpu_time_budget()
            print '       Time:', self.task.cpu_time_used()
            print '  Resources:', self.task.resource_count()
            print '  Regsters:'
            for name in self.regs.names():
                val = self.regs.get(name)
                print '    %20s: %08x (%d)' % (name, val, val)
            return True
        return False

class message_queue:
    "Print classic messege queue"

    def __init__(self, obj):
        self.reference = obj
        self.object = obj.dereference()
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'], \
            'message_queue')
        self.wait_queue = threads.queue( \
            self.object['message_queue']['Wait_queue'])

        self.core_control = supercore.message_queue(self.object['message_queue'])

    def show(self, from_tty):
        print '     Name:', self.object_control.name()
        print '     Attr:', self.attr.to_string()

        self.core_control.show()

class timer:
    '''Print a classic timer'''

    def __init__(self, obj):
        self.reference = obj
        self.object = obj.dereference()
        self.object_control = objects.control(self.object['Object'])
        self.watchdog = watchdog.control(self.object['Ticker'])

    def show(self, from_tty):
        print '     Name:', self.object_control.name()
        self.watchdog.show()

class partition:
    ''' Print a rtems partition '''

    def __init__(self, obj):
        self.reference = obj
        self.object = obj.dereference()
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'], 'partition')
        self.starting_addr = self.object['starting_address']
        self.length = self.object['length']
        self.buffer_size = self.object['buffer_size']
        self.used_blocks = self.object['number_of_used_blocks']

    def show(self, from_tty):
        # ToDo: the printing still somewhat crude.
        print '     Name:', self.object_control.name()
        print '     Attr:', self.attr.to_string()
        print '   Length:', self.length
        print '   B Size:', self.buffer_size
        print ' U Blocks:', self.used_blocks

class region:
    "prints a classic region"

    def __init__(self, obj):
        self.reference = obj
        self.object = obj.dereference()
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'], 'region')
        self.wait_queue = threads.queue(self.object['Wait_queue'])
        self.heap = heaps.control(self.object['Memory'])

    def show(self, from_tty):
        print '     Name:', self.object_control.name()
        print '     Attr:', self.attr.to_string()
        helper.tasks_printer_routine(self.wait_queue)
        print '   Memory:'
        self.heap.show()

class barrier:
    '''classic barrier abstraction'''

    def __init__(self, obj):
        self.reference = obj
        self.object = obj.dereference()
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'],'barrier')
        self.core_b_control = supercore.barrier_control(self.object['Barrier'])

    def show(self,from_tty):
        print '     Name:',self.object_control.name()
        print '     Attr:',self.attr.to_string()

        if self.attr.test('barrier','barrier-auto-release'):
            max_count = self.core_b_control.max_count()
            print 'Aut Count:', max_count

        print '  Waiting:',self.core_b_control.waiting_threads()
        helper.tasks_printer_routine(self.core_b_control.tasks())
