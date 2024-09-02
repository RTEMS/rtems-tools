#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013-2017 Chris Johns (chrisj@rtems.org)
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
# RTEMS Testing Consoles
#

import errno
import os
import sys
import telnetlib
import time

from rtemstoolkit import error
from rtemstoolkit import host
from rtemstoolkit import path

class tty:

    def __init__(self, dev):
        self.dev = dev
        self.timeout = 5
        self.conn = None
        ds = dev.split(':')
        self.host = ds[0]
        if len(ds) == 1:
            self.port = 23
        else:
            try:
                self.port = int(ds[1])
            except:
                raise error.general('invalid port: %s' % (dev))
        self.conn = telnetlib.Telnet()
        self._reopen()

    def __del__(self):
        if self.conn:
            try:
                self.conn.close()
            except:
                pass

    def __str__(self):
        s = 'host: %s port: %d' % ((self.host, self.port))
        return s

    def _reopen(self):
        if self.conn:
            self.conn.close()
            time.sleep(1)
        try:
            self.conn.open(self.host, self.port, self.timeout)
        except IOError as ioe:
            if self.conn:
                self.conn.close()
            raise error.general('opening telnet: %s:%d: %s' % (self.host,
                                                               self.port,
                                                               ioe))
        except:
            if self.conn:
                self.conn.close()
            raise error.general('opening telnet: %s:%d: unknown' % (self.host,
                                                                    self.port))

    def off(self):
        self.is_on = False

    def on(self):
        self.is_on = True

    def set(self, flags):
        pass

    def read(self):
        reopen = False
        try:
            data = self.conn.read_very_eager()
        except IOError as ioe:
            if ioe.errno == errno.ECONNREFUSED:
                reopen = True
            data = ''
        except EOFError as ose:
            reopen = True
            data = ''
        if reopen:
            self._reopen()
        return data


if __name__ == "__main__":
    if len(sys.argv) == 2:
        import time
        t = tty(sys.argv[1])
        print(t)
        while True:
            time.sleep(0.05)
            c = t.read()
            sys.stdout.write(c)
