#
# RTEMS Supercore Objects
#

import threads

# ToDo: Move this to helper.
def tasks_printer_rotuine(wait_queue):
    tasks = wait_queue.tasks()
    print '    Queue: len = %d, state = %s' % (len(tasks),wait_queue.state())
    for t in range(0, len(tasks)):
        print '      ', tasks[t].brief(), ' (%08x)' % (tasks[t].id())

class CORE_message_queue:
    '''Manage a Supercore message_queue'''

    def __init__(self, message_queue):
        self.queue = message_queue
        self.wait_queue = threads.queue(self.queue['Wait_queue'])
        # ToDo: self.attribute =''
        # self.buffer

    def show(self):
        tasks_printer_rotuine(self.wait_queue)
