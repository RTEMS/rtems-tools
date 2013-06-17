#
# RTEMS Watchdog Support
# Copyright 2010 Chris Johns (chrisj@rtems.org)
#
# $Id$
#

import gdb

import chains
import objects

class state:

    INACTIVE = 0
    BEING_INSERTED = 1
    ACTIVE = 2
    REMOVE_IT = 3
    
    states = {
        0: 'inactive',
        1: 'being-inserted',
        2: 'active',
        3: 'remove-it'
        }
        
    def __init__(self, s):
        self.s = s

    def to_string(self):
        return self.states[self.s]

class control:

    def __init__(self, ctrl):
        self.ctrl = ctrl

    def state(self):
        return state(self.ctrl['state']).to_string()

    def initial(self):
        return self.ctrl['initial']

    def delta_interval(self):
        return self.ctrl['delta_interval']

    def start_time(self):
        return self.ctrl['start_time']

    def stop_time(self):
        return self.ctrl['stop_time']

    def routine(self):
        addr = self.ctrl['routine']
        sym = gdb.lookup_symbol(addr)
        print sym
