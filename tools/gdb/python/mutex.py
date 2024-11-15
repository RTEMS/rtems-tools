# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2015 Chris Johns (chrisj@rtems.org)
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
# RTEMS Mutex Support
#

import gdb

import objects
import percpu
import threads


class attributes:
    CORE_MUTEX_NESTING_ACQUIRES = 0
    CORE_MUTEX_NESTING_IS_ERROR = 1
    CORE_MUTEX_NESTING_BLOCKS = 2

    CORE_MUTEX_DISCIPLINES_FIFO = 0
    CORE_MUTEX_DISCIPLINES_PRIORITY = 1
    CORE_MUTEX_DISCIPLINES_PRIORITY_INHERIT = 2
    CORE_MUTEX_DISCIPLINES_PRIORITY_CEILING = 3

    def __init__(self, attr):
        self.attr = attr

    def lock_nesting_behaviour(self):
        return self.attr['lock_nesting_behaviour']

    def only_owner_release(self):
        return self.attr['only_owner_release']

    def discipline(self):
        return self.attr['discipline']

    def priority_ceiling(self):
        return self.attr['priority_ceiling']

    def brief(self):
        s = ''
        if self.lock_nesting_behaviour() == CORE_MUTEX_NESTING_ACQUIRES:
            s += 'lck-nest-acquire'
        elif self.lock_nesting_behaviour() == CORE_MUTEX_NESTING_IS_ERROR:
            s += 'lck-nest-acquire'
        elif self.lock_nesting_behaviour() == CORE_MUTEX_NESTING_BLOCKS:
            s += 'lck-nest-blocks'
        else:
            s += 'lck-nest-bad'
        if self.only_owner_release():
            s += ',owner-release'
        else:
            s += ',any-release'
        if self.discipline() == CORE_MUTEX_DISCIPLINES_FIFO:
            s += ',fifo'
        elif self.discipline() == CORE_MUTEX_DISCIPLINES_PRIORITY:
            s += ',pri'
        elif self.discipline() == CORE_MUTEX_DISCIPLINES_INHERIT:
            s += ',inherit'
        elif self.discipline() == CORE_MUTEX_DISCIPLINES_PRIORITY_CEILING:
            s += ',pri-celling'
        else:
            s += ',dis-bad'
        return s


class control():
    '''
    CORE_mutex_Control has the following fields:
      Wait_queue         Thread_queue_Control
      Attributes         CORE_mutex_attributes
      nest_count         uint32_t
      *holder            Thread_control
      queue              CORE_mutex_order_list               X

    where 'X' means the field is condition and may no exist.
    '''

    def __init__(self, ctrl):
        if ctrl.type.code == gdb.TYPE_CODE_PTR:
            self.reference = ctrl
            self.ctrl = ctrl.dereference()
        else:
            self.ctrl = ctrl
            self.reference = ctrl.address
        self.attr = attributes(ctrl['Attributes'])

    def wait_queue(self):
        return threads.queue(self.ctrl['Wait_queue'])

    def attributes(self):
        return self.attr

    def nest_count(self):
        return self.ctrl['nest_count']

    def holder(self):
        h = self.ctrl['holder']
        if h:
            return threads.control(h)
        return None

    def brief(self):
        return "nests:%d, %s" % (self.ctrl['nest_count'], self.attr.brief())
