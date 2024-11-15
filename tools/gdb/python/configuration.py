# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2014 Chris Johns (chrisj@rtems.org)
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
# RTEMS Configuration Table
#

import gdb


def _table():
    return gdb.parse_and_eval('Configuration')


def fields():
    return [field.name for field in _table().type.fields()]


def mp():
    return '_Configuration_MP_table' in fields()


def smp():
    if 'smp_enabled' in fields():
        return int(_table()['smp_enabled']) != 0
    return False


def maximum_processors():
    if smp():
        return int(_table()['maximum_processors'])
    return 1


def work_space_size():
    return long(_table()['work_space_size'])


def stack_space_size():
    return long(_table()['stack_space_size'])


def maximum_extensions():
    return long(_table()['maximum_extensions'])


def maximum_keys():
    return long(_table()['maximum_keys'])


def maximum_key_value_pairs():
    return long(_table()['maximum_key_value_pairs'])


def microseconds_per_tick():
    return long(_table()['microseconds_per_tick'])


def nanoseconds_per_tick():
    return long(_table()['nanoseconds_per_tick'])


def ticks_per_timeslice():
    return long(_table()['ticks_per_timeslice'])


def idle_task():
    return long(_table()['idle_task'])


def idle_task_stack_size():
    return long(_table()['idle_task_stack_size'])


def interrupt_stack_size():
    return long(_table()['interrupt_stack_size'])


def stack_allocate_init_hook():
    return long(_table()['stack_allocate_init_hook'])


def stack_allocate_hook():
    return long(_table()['stack_allocate_hook'])


def stack_free_hook():
    return long(_table()['stack_free_hook'])


def do_zero_of_workspace():
    return int(_table()['do_zero_of_workspace']) != 0


def unified_work_area():
    return int(_table()['unified_work_area']) != 0


def stack_allocator_avoids_work_space():
    return long(_table()['stack_allocator_avoids_work_space'])


def number_of_initial_extensions():
    return int(_table()['number_of_initial_extensions'])


def user_extension_table():
    return _table()['User_extension_table']
