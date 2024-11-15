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
# RTEMS Time Support
#

import gdb


class time():

    def __init__(self, time_, ticks=False, scaler=1000000000):
        self.ticks = ticks
        self.scaler = scaler
        if time_.type.code == gdb.TYPE_CODE_PTR:
            self.reference = time_
            self.time = time_.dereference()
        else:
            self.time = time_
            self.reference = time_.address
        if self.time.type.strip_typedefs().tag == 'timespec':
            self.timespecs = True
        else:
            self.timespecs = False

    def __str__(self):
        return self.tostring()

    def get(self):
        if self.timespecs:
            return (long(self.time['tv_sec']) * 1000000000) + long(
                self.time['tv_nsec'])
        else:
            return long(self.time) * self.scaler

    def tostring(self):
        if self.ticks:
            return '%d ticks' % (self.time)
        nsecs = self.get()
        secs = nsecs / 1000000000
        nsecs = nsecs % 1000000000
        minutes = secs / 60
        secs = secs % 60
        hours = minutes / 60
        minutes = minutes % 60
        days = hours / 24
        hours = hours % 24
        t = ''
        if days:
            t = '%dd ' % (days)
        if hours or minutes or secs:
            t += '%02d:%02d:%02d ' % (hours, minutes, secs)
        t += '%d nsec' % (nsecs)
        return t
