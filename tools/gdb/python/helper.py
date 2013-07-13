#
# RTEMS GDB support helper routins.

def tasks_printer_rotuine(wait_queue):
    tasks = wait_queue.tasks()
    print '    Queue: len = %d, state = %s' % (len(tasks),wait_queue.state())
    for t in range(0, len(tasks)):
        print '      ', tasks[t].brief(), ' (%08x)' % (tasks[t].id())