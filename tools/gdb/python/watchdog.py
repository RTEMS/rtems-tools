# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2014 Chris Johns (chrisj@rtems.org)
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
# RTEMS Watchdog Support
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
        print(self.to_string())
