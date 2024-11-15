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
# RTEMS Threads Support
#

import gdb

import chains
import objects
import percpu
import rbtrees
import time


def task_chain(chain):
    tasks = []
    if not chain.empty():
        node = chain.first()
        while not node.null():
            t = control(node.cast('Thread_Control'))
            tasks.append(t)
            node = node(node.next())
    return tasks


def task_tree(tree):
    tasks = []
    node = tree.first(rbtrees.rbt_left)
    while not node.null():
        tasks.append(control(node.cast('Thread_Control')))
        node = node.next(rbtrees.rbt_left)
    return tasks


class state():

    ALL_SET = 0x000fffff
    READY = 0x00000000
    DORMANT = 0x00000001
    SUSPENDED = 0x00000002
    TRANSIENT = 0x00000004
    DELAYING = 0x00000008
    WAITING_FOR_TIME = 0x00000010
    WAITING_FOR_BUFFER = 0x00000020
    WAITING_FOR_SEGMENT = 0x00000040
    WAITING_FOR_MESSAGE = 0x00000080
    WAITING_FOR_EVENT = 0x00000100
    WAITING_FOR_SEMAPHORE = 0x00000200
    WAITING_FOR_MUTEX = 0x00000400
    WAITING_FOR_CONDITION_VARIABLE = 0x00000800
    WAITING_FOR_JOIN_AT_EXIT = 0x00001000
    WAITING_FOR_RPC_REPLY = 0x00002000
    WAITING_FOR_PERIOD = 0x00004000
    WAITING_FOR_SIGNAL = 0x00008000
    WAITING_FOR_BARRIER = 0x00010000
    WAITING_FOR_RWLOCK = 0x00020000
    INTERRUPTIBLE_BY_SIGNAL = 0x10000000
    LOCALLY_BLOCKED = \
        WAITING_FOR_BUFFER             | \
        WAITING_FOR_SEGMENT            | \
        WAITING_FOR_MESSAGE            | \
        WAITING_FOR_SEMAPHORE          | \
        WAITING_FOR_MUTEX              | \
        WAITING_FOR_CONDITION_VARIABLE | \
        WAITING_FOR_JOIN_AT_EXIT       | \
        WAITING_FOR_SIGNAL             | \
        WAITING_FOR_BARRIER            | \
        WAITING_FOR_RWLOCK
    WAITING_ON_THREAD_QUEUE = \
        LOCALLY_BLOCKED | WAITING_FOR_RPC_REPLY
    BLOCKED = \
        DELAYING                | \
        WAITING_FOR_TIME        | \
        WAITING_FOR_PERIOD      | \
        WAITING_FOR_EVENT       | \
        WAITING_ON_THREAD_QUEUE | \
        INTERRUPTIBLE_BY_SIGNAL

    masks = {
        ALL_SET: 'all-set',
        READY: 'ready',
        DORMANT: 'dormant',
        SUSPENDED: 'suspended',
        TRANSIENT: 'transient',
        DELAYING: 'delaying',
        WAITING_FOR_TIME: 'waiting-for-time',
        WAITING_FOR_BUFFER: 'waiting-for-buffer',
        WAITING_FOR_SEGMENT: 'waiting-for-segment',
        WAITING_FOR_MESSAGE: 'waiting-for-message',
        WAITING_FOR_EVENT: 'waiting-for-event',
        WAITING_FOR_SEMAPHORE: 'waiting-for-semaphore',
        WAITING_FOR_MUTEX: 'waiting-for-mutex',
        WAITING_FOR_CONDITION_VARIABLE: 'waiting-for-condition-variable',
        WAITING_FOR_JOIN_AT_EXIT: 'waiting-for-join-at-exit',
        WAITING_FOR_RPC_REPLY: 'waiting-for-rpc-reply',
        WAITING_FOR_PERIOD: 'waiting-for-period',
        WAITING_FOR_SIGNAL: 'waiting-for-signal',
        WAITING_FOR_BARRIER: 'waiting-for-barrier',
        WAITING_FOR_RWLOCK: 'waiting-for-rwlock'
    }

    def __init__(self, s):
        self.s = s

    def to_string(self):
        if (self.s & self.LOCALLY_BLOCKED) == self.LOCALLY_BLOCKED:
            return 'locally-blocked'
        if (self.s &
                self.WAITING_ON_THREAD_QUEUE) == self.WAITING_ON_THREAD_QUEUE:
            return 'waiting-on-thread-queue'
        if (self.s & self.BLOCKED) == self.BLOCKED:
            return 'blocked'
        s = ','
        for m in self.masks:
            if (self.s & m) == m:
                s = self.masks[m] + ','
        return s[:-1]


class cpu_usage():

    def __init__(self, time_):
        self.time = time.time(time_)

    def __str__(self):
        return self.time.tostring()

    def get(self):
        return self.time.get()


class wait_info():

    def __init__(self, info):
        self.info = info

    def id(self):
        return self.info['id']

    def count(self):
        return self.info['count']

    def return_arg(self):
        return self.info['return_argument']

    def option(self):
        return self.info['option']

    def block2n(self):
        return task_chain(chains.control(self.info['Block2n']))

    def queue(self):
        return task_chain(chains.control(self.info['queue']))


class registers():

    def __init__(self, regs):
        self.regs = regs

    def names(self):
        return [field.name for field in self.regs.type.fields()]

    def get(self, reg):
        t = str(self.regs[reg].type)
        if t in ['double']:
            return float(self.regs[reg])
        return int(self.regs[reg])

    def format(self, reg):
        t = self.regs[reg].type
        if t in ['uint32_t', 'unsigned', 'unsigned long']:
            return '%08x (%d)' % (val)


class control():
    '''
    Thread_Control has the following fields:
      Object              Objects_Control
      RBNode              RBTree_Node
      current_state       States_Control
      current_priority    Priority_Control
      real_priority       Priority_Control
      resource_count      uint32_t
      Wait                Thread_Wait_information
      Timer               Watchdog_Control
      receive_packet      MP_packet_Prefix*                   X
      lock_mutex          Chain_Control                       X
      Resource_node       Resource_Node                       X
      is_global           bool                                X
      is_preemptible      bool
      Scheduler           Thread_Scheduler_control
      rtems_ada_self      void*                               X
      cpu_time_budget     uint32_t
      budget_algorithm    Thread_CPU_budget_algorithms
      budget_callout      Thread_CPU_budget_algorithm_callout
      cpu_time_used       Thread_CPU_usage_t
      Start               Thread_Start_information
      Post_switch_actions Thread_Action_control
      Registers           Context_Control
      fp_context          Context_Control_fp*                 X
      libc_reent          struct _reent*
      API_Extensions      void*[THREAD_API_LAST + 1]
      task_variables      rtems_task_variable_t*              X
      Key_Chain           Chain_Control
      Life                Thread_Life_control
      extensions          void*[RTEMS_ZERO_LENGTH_ARRAY]

    where 'X' means the field is condition and may no exist.
    '''

    def __init__(self, ctrl):
        if ctrl.type.code == gdb.TYPE_CODE_PTR:
            self.reference = ctrl
            self.ctrl = ctrl.dereference()
        else:
            self.ctrl = ctrl
            self.reference = ctrl.address
        self.object = objects.control(ctrl['Object'])
        self._executing = percpu.thread_active(self.reference)
        self._heir = percpu.thread_heir(self.reference)

    def id(self):
        return self.object.id()

    def name(self):
        val = self.object.name()
        if val == "":
            val = '*'
        return val

    def executing(self):
        return self._executing

    def heir(self):
        return self._heir

    def current_state(self):
        return state(self.ctrl['current_state']).to_string()

    def current_priority(self):
        return self.ctrl['current_priority']

    def real_priority(self):
        return self.ctrl['real_priority']

    def resource_count(self):
        return self.ctrl['resource_count']

    def cpu_time_budget(self):
        return self.ctrl['cpu_time_budget']

    def cpu_time_used(self):
        return cpu_usage(self.ctrl['cpu_time_used'])

    def preemptible(self):
        return self.ctrl['is_preemptible']

    def cpu_time_budget(self):
        return self.ctrl['cpu_time_budget']

    def wait_info(self):
        return wait_info(self.ctrl['Wait'])

    def registers(self):
        return registers(self.ctrl['Registers'])

    def is_idle(self):
        return (self.id() & 0xff000000) == 0x90000000

    def brief(self):
        return "'%s' (c:%d, r:%d)" % \
            (self.name(), self.current_priority(), self.real_priority())


class queue():
    """Manage the Thread_queue_Control."""

    def __init__(self, que):
        if que.type.code == gdb.TYPE_CODE_PTR:
            self.reference = que
            self.que = que.dereference()
        else:
            self.que = que
            self.reference = que.address

    def fifo(self):
        return str(self.que['discipline']) == 'THREAD_QUEUE_DISCIPLINE_FIFO'

    def priority(self):
        return str(
            self.que['discipline']) == 'THREAD_QUEUE_DISCIPLINE_PRIORITY'

    def state(self):
        return state(self.que['state']).to_string()

    def tasks(self):
        if self.fifo():
            t = task_chain(chains.control(self.que['Queues']['Fifo']))
        else:
            t = task_tree(rbtrees.control(self.que['Queues']['Priority']))
        return t
