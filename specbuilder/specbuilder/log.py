#
# $Id$
#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-testing'.
#
# RTEMS Tools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTEMS Tools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RTEMS Tools.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Log output to stdout and/or a file.
#

import os
import sys

import error

#
# A global log.
#
default = None

def set_default_once(log):
    if default is None:
        default = log

def output(text = os.linesep, log = None):
    """Output the text to a log if provided else send it to stdout."""
    if text is None:
        text = os.linesep
    if type(text) is list:
        _text = ''
        for l in text:
            _text += l + os.linesep
        text = _text
    if log:
        log.output(text)
    elif default is not None:
        default.output(text)
    else:
        for l in text.replace(chr(13), '').splitlines():
            print l

def flush(log = None):
    if log:
        log.flush()
    elif default is not None:
        default.flush()

class log:
    """Log output to stdout or a file."""
    def __init__(self, streams = None):
        self.fhs = [None, None]
        if streams:
            for s in streams:
                if s == 'stdout':
                    self.fhs[0] = sys.stdout
                elif s == 'stderr':
                    self.fhs[1] = sys.stderr
                else:
                    try:
                        self.fhs.append(file(s, 'w'))
                    except IOError, ioe:
                         raise error.general("creating log file '" + s + \
                                             "': " + str(ioe))

    def __del__(self):
        for f in range(2, len(self.fhs)):
            self.fhs[f].close()

    def has_stdout(self):
        return self.fhs[0] is not None

    def has_stderr(self):
        return self.fhs[1] is not None

    def output(self, text):
        """Output the text message to all the logs."""
        # Reformat the text to have local line types.
        out = ''
        for l in text.replace(chr(13), '').splitlines():
            out += l + os.linesep 
        for f in range(0, len(self.fhs)):
            if self.fhs[f] is not None:
                self.fhs[f].write(out)

    def flush(self):
        """Flush the output."""
        for f in range(0, len(self.fhs)):
            if self.fhs[f] is not None:
                self.fhs[f].flush()
        
if __name__ == "__main__":
    l = log(['stdout', 'log.txt'])
    for i in range(0, 10):
        l.output('hello world: %d\n' % (i))
    l.output('hello world CRLF\r\n')
    l.output('hello world NONE')
    l.flush()
    del l
