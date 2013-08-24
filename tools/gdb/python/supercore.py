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
        print ' Time Of Day'

        if not self.is_set():
            print ' Application has not set a TOD'

        print '      Now:', self.now()
        print '   Uptime:', self.timer()


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
