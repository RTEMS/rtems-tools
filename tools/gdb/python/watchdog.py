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

    # ToDo: fix this.1
    def state(self):
        return state(1).to_string()
        #return state(int(self.ctrl['state'])).to_string()

    def initial(self):
        return self.ctrl['initial']

    def delta_interval(self):
        return self.ctrl['delta_interval']

    def start_time(self):
        return self.ctrl['start_time']

    def stop_time(self):
        return self.ctrl['stop_time']

    # ToDo: Better printing of watchdog.
    def routine(self):
        addr = self.ctrl['routine']
        return str(addr)

    def show(self):
        print "     State:", self.state()
        print "     Intial Interval:", self.initial()
        print "     Delta Interval:", self.delta_interval()
        print "     Start time:", self.start_time()
        print "     Stop time:", self.stop_time()
        print "     WD Routine:", self.routine()