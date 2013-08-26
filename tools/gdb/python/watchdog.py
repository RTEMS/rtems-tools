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

    def to_string(self):
        val = ""
        val += "     State:" + str(self.state())
        val += "\n     Intial Interval:" + str(self.initial())
        val += "\n     Delta Interval:"+ str(self.delta_interval())
        val += "\n     Start time:" + str(self.start_time())
        val += "\n     Stop time:" + str(self.stop_time())
        val += "\n     WD Routine:" + str(self.routine())
        return val

    def show(self):
        print self.to_string()