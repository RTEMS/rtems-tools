#
# RTEMS GDB support helper routins.

import gdb

def tasks_printer_routine(wait_queue):
    tasks = wait_queue.tasks()
    print '    Queue: len = %d, state = %s' % (len(tasks),wait_queue.state())
    for t in range(0, len(tasks)):
        print '      ', tasks[t].brief(), ' (%08x)' % (tasks[t].id())

def type_from_value(val):
    type = val.type;
    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()
    # Get the unqualified type
    return type.unqualified ()
