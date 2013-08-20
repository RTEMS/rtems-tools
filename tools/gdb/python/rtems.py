#
# RTEMS Pretty Printers
# Copyright 2010 Chris Johns (chrisj@rtems.org)
#
# $Id$
#

import gdb
import re

import objects
import threads
import classic


class rtems(gdb.Command):
    """Prefix command for RTEMS."""

    def __init__(self):
        super(rtems, self).__init__('rtems',
                                    gdb.COMMAND_STATUS,
                                    gdb.COMPLETE_NONE,
                                    True)

class rtems_object(gdb.Command):
    """Object sub-command for RTEMS"""

    objects = {
        'classic/semaphores': lambda obj: classic.semaphore(obj),
        'classic/tasks': lambda obj: classic.task(obj),
        'classic/message_queues': lambda obj: classic.message_queue(obj),
        'classic/timers' : lambda obj: classic.timer(obj),
        'classic/partitions' : lambda obj: classic.partition(obj),
        'classic/regions' : lambda obj: classic.region(obj),
        'classic/barriers' : lambda obj: classic.barrier(obj)
        }

    def __init__(self):
        self.__doc__ = 'Display the RTEMS object given a numeric ID (Or a reference to rtems_object).'
        super(rtems_object, self).__init__('rtems object',
                                           gdb.COMMAND_STATUS)

    def invoke(self, arg, from_tty):
        for num in arg.split():
            try:
                val = gdb.parse_and_eval(num)
                num = int(val)
            except:
                print 'error: "%s" is not a number' % (num)
                return
            id = objects.ident(num)
            if not id.valid():
                print 'Invalid object id'
                return

            print 'API:%s Class:%s Node:%d Index:%d Id:%08X' % \
                (id.api(), id._class(), id.node(), id.index(), id.value())
            objectname = id.api() + '/' + id._class()

            obj = objects.information.object(id).dereference()
            if objectname in self.objects:
                object = self.objects[objectname](obj)
                object.show(from_tty)
        objects.information.invalidate()

class rtems_semaphore(gdb.Command):
    '''Semaphore subcommand for rtems'''

    api = 'classic'
    _class = 'semaphores'

    def __init__(self):
        self.__doc__ = 'Display the RTEMS semaphores by index'
        super(rtems_semaphore, self).__init__('rtems semaphore',
                                           gdb.COMMAND_STATUS)

    def invoke(self, arg, from_tty):
        for val in arg.split():
            try:
                index = int(val)
            except ValueError:
                print "error: %s is not an index" % (val)
                return
            try:
                obj = objects.information.object_return( self.api,
                                                         self._class,
                                                         index ).dereference()
            except IndexError:
                print "error: index %s is invalid" % (index)
                return

            instance = classic.semaphore(obj)
            instance.show(from_tty)
        objects.information.invalidate()

class rtems_task(gdb.Command):
    '''tasks subcommand for rtems'''

    api = 'classic'
    _class = 'tasks'

    def __init__(self):
        self.__doc__ = 'Display the RTEMS tasks by index(s)'
        super(rtems_task,self).__init__('rtems task', gdb.COMMAND_STATUS)

    def invoke(self, arg, from_tty):
        for val in arg.split():
            try:
                index = int(val)
            except ValueError:
                raise gdb.GdbError( "Value is not an integer")

            try:
                obj = objects.information.object_return(self.api,
                                                        self._class,
                                                        index).dereference()
            except IndexError:
                print "error: index %s is invalid" % (index)
                return

            instance = classic.task(obj)
            instance.show(from_tty)
        objects.information.invalidate()

class rtems_message_queue(gdb.Command):
    '''Message Queue subcommand'''

    api = 'classic'
    _class = 'message_queues'

    def __init__(self):
        self.__doc__ = 'Display the RTEMS message_queue by index(s)'
        super(rtems_message_queue,self).__init__('rtems mqueue', gdb.COMMAND_STATUS)

    def invoke(self, arg, from_tty):
        for val in arg.split():
            try:
                index = int(val)
            except ValueError:
                print "error: %s is not an index" % (val)
                return

            try:
                obj = objects.information.object_return(self.api,
                                                        self._class,
                                                        index).dereference()
            except IndexError:
                print "error: index %s is invalid" % (index)
                return

            instance = classic.message_queue(obj)
            instance.show(from_tty)
        objects.information.invalidate()