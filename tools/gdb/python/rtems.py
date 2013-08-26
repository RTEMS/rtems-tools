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
import chains
import watchdog
import supercore
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
        self.__doc__ = 'Display the RTEMS object given a numeric ID \
                                            (Or a reference to rtems_object).'
        super(rtems_object, self).__init__('rtems object',
                                           gdb.COMMAND_DATA,
                                           gdb.COMPLETE_SYMBOL)

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

class rtems_index(gdb.Command):
    '''Print object by index'''

    api = 'classic'
    _class = ''

    def __init__(self,command):
        super(rtems_index, self).__init__( command,
                                           gdb.COMMAND_DATA,
                                           gdb.COMPLETE_NONE)

    def instance(self,obj):
        '''Returns a n instance of corresponding object, the child should extend this'''
        return obj

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

            instance = self.instance(obj)
            instance.show(from_tty)
        objects.information.invalidate()


class rtems_semaphore(rtems_index):
    '''semaphore subcommand'''
    _class = 'semaphores'

    def __init__(self):
        self.__doc__ = 'Display RTEMS semaphore(s) by index(es)'
        super(rtems_semaphore, self).__init__('rtems semaphore')

    def instance(self,obj):
        return classic.semaphore(obj)

class rtems_task(rtems_index):
    '''tasks subcommand for rtems'''

    _class = 'tasks'

    def __init__(self):
        self.__doc__ = 'Display RTEMS task(s) by index(es)'
        super(rtems_task,self).__init__('rtems task')

    def instance(self,obj):
        return classic.task(obj)


class rtems_message_queue(rtems_index):
    '''Message Queue subcommand'''

    _class = 'message_queues'

    def __init__(self):
        self.__doc__ = 'Display RTEMS message_queue(s) by index(es)'
        super(rtems_message_queue,self).__init__('rtems mqueue')

    def instance(self,obj):
        return classic.message_queue(obj)

class rtems_timer(rtems_index):
    '''Index subcommand'''

    _class = 'timers'

    def __init__(self):
        self.__doc__ = 'Display RTEMS timer(s) by index(es)'
        super(rtems_timer, self).__init__('rtems timer')

    def instance(self,obj):
        return classic.timer(obj)

class rtems_partition(rtems_index):
    '''Partition subcommand'''

    _class = 'partitions'

    def __init__(self):
        self.__doc__ = 'Display RTEMS partition(s) by index(es)'
        super(rtems_partition, self).__init__('rtems partition')

    def instance(self, obj):
        return classic.partition(obj)

class rtems_region(rtems_index):
    '''Region subcomamnd'''

    _class = 'regions'

    def __init__(self):
        self.__doc__ = 'Display RTEMS region(s) by index(es)'
        super(rtems_region , self).__init__('rtems regions')

    def instance(self, obj):
        return classic.region(obj)

class rtems_barrier(rtems_index):
    '''Barrier subcommand'''

    _class = 'barriers'

    def __init__(self):
        self.__doc__ = 'Display RTEMS barrier(s) by index(es)'
        super(rtems_barrier , self).__init__('rtems barrier')

    def instance(self, obj):
        return classic.barrier(obj)

class rtems_tod(gdb.Command):
    '''Print rtems time of day'''

    api = 'internal'
    _class = 'time'

    def __init__(self):
        self.__doc__ = 'Display RTEMS time of day'
        super(rtems_tod, self).__init__ \
                    ('rtems tod', gdb.COMMAND_STATUS,gdb.COMPLETE_NONE)

    def invoke(self, arg, from_tty):

        if arg:
            print "warning: commad takes no arguments!"

        obj = objects.information.object_return(self.api,self._class)
        instance = supercore.time_of_day(obj)
        instance.show()
        objects.information.invalidate()

class rtems_watchdog_chain(gdb.Command):
    '''Print watchdog ticks chain'''

    api = 'internal'
    _class = ''

    def __init__(self,command):
        super(rtems_watchdog_chain, self).__init__ \
                                (command, gdb.COMMAND_DATA, gdb.COMPLETE_NONE)

    def invoke(self, arg, from_tty):
        obj = objects.information.object_return(self.api, self._class)

        inst = chains.control(obj)

        if inst.empty():
            print '     error: empty chain'
            return

        nd = inst.first()
        i = 0
        while not nd.null():
            wd = watchdog.control(nd.cast('Watchdog_Control'))
            print ' #'+str(i)
            print wd.to_string()
            nd.next()
            i += 1

class rtems_wdt(rtems_watchdog_chain):

    _class = 'wdticks'

    def __init__(self):
        self.__doc__ = 'Display watchdog ticks chain'
        super(rtems_wdt, self).__init__('rtems wdticks')

class rtems_wsec(rtems_watchdog_chain):

    _class = 'wdseconds'

    def __init__(self):
        self.__doc__ = 'Display watchdog seconds chain'
        super(rtems_wsec, self).__init__('rtems wdseconds')

