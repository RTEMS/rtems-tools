#
# RTEMS Supercore Objects
#

import threads
import helper

class CORE_message_queue:
    '''Manage a Supercore message_queue'''

    def __init__(self, message_queue):
        self.queue = message_queue
        self.wait_queue = threads.queue(self.queue['Wait_queue'])
        # ToDo: self.attribute =''
        # self.buffer

    def show(self):
        helper.tasks_printer_routine(self.wait_queue)
