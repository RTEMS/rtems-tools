#
# RTEMS Classic API Support
# Copyright 2010 Chris Johns (chrisj@rtems.org)
#
# $Id$
#

import gdb
import itertools
import re

import objects
import threads
import watchdog
import supercore

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
        'barrier' : ['scope',
                     'priority',
                     'barrier'],
        'message_queue' : ['priority',
                           'scope']
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
        s = '0x%08x,' % (self.attr)
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

    def __init__(self, id):
        self.id = id;
        self.object = objects.information.object(self.id).dereference()
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'], 'semaphore')

    def show(self, from_tty):
        print '     Name:', self.object_control.name()
        print '     Attr:', self.attr.to_string()
        if self.attr.test('semaphore-type', 'bin-sema') or \
                self.attr.test('semaphore-type', 'simple-bin-sema'):
            core_mutex = self.object['Core_control']['mutex']
            locked = core_mutex['lock'] == 0
            if locked:
                s = 'locked'
            else:
                s = 'unlocked'
            print '     Lock:', s
            print '  Nesting:', core_mutex['nest_count']
            print '  Blocked:', core_mutex['blocked_count']
            print '   Holder:',
            holder = core_mutex['holder']
            if holder and locked:
                holder = threads.control(holder.dereference())
                print holder.brief()
            elif holder == 0 and locked:
                print 'locked but no holder'
            else:
                print 'unlocked'
            wait_queue = threads.queue(core_mutex['Wait_queue'])
            tasks = wait_queue.tasks()
            print '    Queue: len = %d, state = %s' % (len(tasks),
                                                       wait_queue.state())
            for t in range(0, len(tasks)):
                print '      ', tasks[t].brief(), ' (%08x)' % (tasks[t].id())
        else:
            print 'semaphore'

class task:
    "Print a classic tasks."

    def __init__(self, id):
        self.id = id;
        self.task = \
            threads.control(objects.information.object(self.id).dereference())

    def show(self, from_tty):
        print '     Name:', self.task.name()
        print '    State:', self.task.current_state()
        print '  Current:', self.task.current_priority()
        print '     Real:', self.task.real_priority()
        print ' Suspends:', self.task.suspends()
        print ' Post Ext:', self.task.post_task_switch_ext()
        print '  Preempt:', self.task.preemptible()
        print ' T Budget:', self.task.cpu_time_budget()
        wait_info = self.task.wait_info()

class message_queue:
    "Print a classic messege queue"

    def __init__(self,id):
        self.id = id
        self.object = objects.information.object(self.id).dereference()
        self.object_control = objects.control(self.object['Object'])
        self.attr = attribute(self.object['attribute_set'], \
            'message_queue')
        self.wait_queue = threads.queue( \
            self.object['message_queue']['Wait_queue'])

        self.core_control = supercore.CORE_message_queue(self.object['message_queue'])

    def show(self, from_tty):
        print '     Name:', self.object_control.name()
        print '     Attr:', self.attr.to_string()

        self.core_control.show()

class timer:
    '''Print a classic timer'''

    def __init__(self, id):
        self.id = id
        self.object = objects.information.object(self.id).dereference()
        self.object_control = objects.control(self.object['Object'])
        self.watchdog = watchdog.control(self.object['Ticker'])

    def show(self, from_tty):
        print '     Name:', self.object_control.name()
        self.watchdog.show()