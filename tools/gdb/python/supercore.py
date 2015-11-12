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
# RTEMS Supercore Objects
#

import threads
import helper

class time_of_day:
    '''Manage time of day object'''

    def __init__(self, tod):
        self.tod = tod

    def now(self):
        return self.tod['now']

    def timer(self):
        return self.tod['uptime']

    def is_set(self):
        return bool(self.tod['is_set'])

    def show(self):
        print(' Time Of Day')

        if not self.is_set():
            print(' Application has not set a TOD')

        print('      Now:', self.now())
        print('   Uptime:', self.timer())


class message_queue:
    '''Manage a Supercore message_queue'''

    def __init__(self, message_queue):
        self.queue = message_queue
        self.wait_queue = threads.queue(self.queue['Wait_queue'])
        # ToDo: self.attribute =''
        # self.buffer

    def show(self):
        helper.tasks_printer_routine(self.wait_queue)

class barrier_attributes:
    '''supercore bbarrier attribute'''

    def __init__(self,attr):
        self.attr = attr

    def max_count(self):
        c = self.attr['maximum_count']
        return c

    def discipline(self):
        d = self.attr['discipline']
        return d

class barrier_control:
    '''Manage a Supercore barrier'''

    def __init__(self, barrier):
        self.barrier = barrier
        self.wait_queue = threads.queue(self.barrier['Wait_queue'])
        self.attr = barrier_attributes(self.barrier['Attributes'])

    def waiting_threads(self):
        wt = self.barrier['number_of_waiting_threads']
        return wt

    def max_count(self):
        return self.attr.max_count()

    def discipline(self):
        return self.attr.discipline()

    def tasks(self):
        return self.wait_queue
