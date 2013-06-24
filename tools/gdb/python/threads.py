#
# RTEMS Threads Support
# Copyright 2010 Chris Johns (chrisj@rtems.org)
#
# $Id$
#

import gdb

import chains
import objects

def task_chain(chain):
    tasks = []
    node = chain.first()
    while not node.null():
        tasks.append(control(node.cast('Thread_Control')))
        node.next()
    return tasks

class state:

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
        ALL_SET : 'all-set',
        READY : 'ready',
        DORMANT : 'dormant',
        SUSPENDED : 'suspended',
        TRANSIENT : 'transient',
        DELAYING : 'delaying',
        WAITING_FOR_TIME : 'waiting-for-time',
        WAITING_FOR_BUFFER : 'waiting-for-buffer',
        WAITING_FOR_SEGMENT : 'waiting-for-segment',
        WAITING_FOR_MESSAGE : 'waiting-for-message',
        WAITING_FOR_EVENT : 'waiting-for-event',
        WAITING_FOR_SEMAPHORE : 'waiting-for-semaphore',
        WAITING_FOR_MUTEX : 'waiting-for-mutex',
        WAITING_FOR_CONDITION_VARIABLE : 'waiting-for-condition-variable',
        WAITING_FOR_JOIN_AT_EXIT : 'waiting-for-join-at-exit',
        WAITING_FOR_RPC_REPLY : 'waiting-for-rpc-reply',
        WAITING_FOR_PERIOD : 'waiting-for-period',
        WAITING_FOR_SIGNAL : 'waiting-for-signal',
        WAITING_FOR_BARRIER : 'waiting-for-barrier',
        WAITING_FOR_RWLOCK : 'waiting-for-rwlock'
        }

    def __init__(self, s):
        self.s = s

    def to_string(self):
        if (self.s & self.LOCALLY_BLOCKED) == self.LOCALLY_BLOCKED:
            return 'locally-blocked'
        if (self.s & self.WAITING_ON_THREAD_QUEUE) == self.WAITING_ON_THREAD_QUEUE:
            return 'waiting-on-thread-queue'
        if (self.s & self.BLOCKED) == self.BLOCKED:
            return 'blocked'
        s = ','
        for m in self.masks:
            if (self.s & m) == m:
                s = self.masks[m] + ','
        return s[:-1]

class wait_info:

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

class control:

    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.object = objects.control(ctrl['Object'])

    def id(self):
        return self.object.id()

    def name(self):
        return self.object.name()

    def current_state(self):
        return state(self.ctrl['current_state']).to_string()

    def current_priority(self):
        return self.ctrl['current_priority']

    def real_priority(self):
        return self.ctrl['real_priority']

    def suspends(self):
        return self.ctrl['suspend_count']

    def post_task_switch_ext(self):
        return self.ctrl['do_post_task_switch_extension']

    def preemptible(self):
        return self.ctrl['is_preemptible']

    def cpu_time_budget(self):
        return self.ctrl['cpu_time_budget']

    def wait_info(self):
        return wait_info(self.ctrl['Wait'])

    def brief(self):
        return "'%s' (c:%d, r:%d)" % \
            (self.name(), self.current_priority(), self.real_priority())

class queue:
    """Manage the Thread_queue_Control."""

    priority_headers = 4

    def __init__(self, que):
        self.que = que

    def fifo(self):
        return str(self.que['discipline']) == 'THREAD_QUEUE_DISCIPLINE_FIFO'

    def priority(self):
        return str(self.que['discipline']) == 'THREAD_QUEUE_DISCIPLINE_PRIORITY'

    def state(self):
        return state(self.que['state']).to_string()

    def tasks(self):
        if self.fifo():
            t = task_chain(chains.control(self.que['Queues']['Fifo']))
        else:
            t = []
            for ph in range(0, self.priority_headers):
                t.extend(task_chain(chains.control( \
                    self.que['Queues']['Priority'][ph])))
        return t

    def to_string(self):
        if self.fifo():
            s = 'fifo'
        else:
            s = 'priority'
        return

class state_printer:

    def __init__(self, s):
        self.s = state(s)

    def to_string(self):
        return self.s.to_string()
