#
# RTEMS GDB Extensions
#
# main

import gdb
import pretty
import rtems

gdb.pretty_printers = []
gdb.pretty_printers.append(pretty.lookup_function)

# Register commands
# rtems and subcommands
rtems.rtems()
rtems.rtems_object()
rtems.rtems_semaphore()
rtems.rtems_task()
rtems.rtems_message_queue()
rtems.rtems_tod()