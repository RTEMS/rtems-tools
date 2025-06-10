#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2016 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-testing'.
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
# Log output to stdout and/or a file.
#

from __future__ import print_function

import os
import sys
import threading

from rtemstoolkit import error

#
# A global log.
#
default = None

#
# A global capture handler.
#
capture = None

#
# Global parameters.
#
tracing = False
quiet = False

#
# Global output lock to keep output together when working with threads
#
lock = threading.Lock()


def safe_blocking_write(file_handle, out):
    '''This is a bit of a hack to force the file handle back to
        blocking. It seems for some cases stdio is being set to
        non-blocking in some execute commands.
    '''
    writing = True
    while writing:
        try:
            file_handle.write(out)
            writing = False
        except BlockingIOError as e:
            import fcntl
            flags = fcntl.fcntl(file_handle, fcntl.F_GETFL)
            if flags & os.O_NONBLOCK != 0:
                fcntl.fcntl(file_handle, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)


def info(args):
    s = [' Command Line: %s' % (' '.join(args))]
    if hasattr(os, 'uname'):
        s += [' Host: %s' % (' '.join(os.uname()))]
    else:
        h = ' Host: win32-mingw'
        if 'HOSTTYPE' in os.environ:
            h += ' ' + os.environ['HOSTTYPE']
        else:
            h += ' i686'
        s += [h]
    s += [' Python: %s' % (sys.version.replace('\n', ''))]
    return s


def set_default_once(log):
    if default is None:
        default = log


def _output(text=os.linesep, log=None):
    """Output the text to a log if provided else send it to stdout."""
    if text is None:
        text = os.linesep
    if type(text) is list:
        text = os.linesep.join(text) + os.linesep
    if isinstance(text, bytes):
        text = text.decode('utf-8', 'ignore')
    if log:
        log.output(text)
    elif default is not None:
        default.output(text)
    else:
        lock.acquire()
        for l in text.replace(chr(13), '').splitlines():
            safe_blocking_write(sys.stdout, l)
        lock.release()
    if capture is not None:
        lock.acquire()
        capture(text)
        lock.release()


def stderr(text=os.linesep, log=None):
    lock.acquire()
    for l in text.replace(chr(13), '').splitlines():
        safe_blocking_write(sys.stderr, l)
    lock.release()


def output(text=os.linesep, log=None):
    if not quiet:
        _output(text, log)


def notice(text=os.linesep, log=None, stdout_only=False):
    if not quiet and \
            (default is not None and not default.has_stdout() or stdout_only):
        lock.acquire()
        for l in text.replace(chr(13), '').splitlines():
            safe_blocking_write(sys.stdout, l)
        lock.release()
    if not stdout_only:
        _output(text, log)


def trace(text=os.linesep, log=None):
    if tracing:
        _output(text, log)


def warning(text=os.linesep, log=None):
    for l in text.replace(chr(13), '').splitlines():
        _output('warning: %s' % (l), log)


def flush(log=None):
    if log:
        log.flush()
    elif default is not None:
        default.flush()


class log:
    """Log output to stdout or a file."""

    def __init__(self, streams=None, tail_size=100):
        self.lock = threading.Lock()
        self.tail = []
        self.tail_size = tail_size
        self.fhs = [None, None]
        if streams:
            for s in streams:
                if s == 'stdout':
                    self.fhs[0] = sys.stdout
                elif s == 'stderr':
                    self.fhs[1] = sys.stderr
                else:
                    try:
                        self.fhs.append(open(s, 'w'))
                    except IOError as ioe:
                        raise error.general("creating log file '" + s + \
                                            "': " + str(ioe))

    def __del__(self):
        for f in range(2, len(self.fhs)):
            self.fhs[f].close()

    def __str__(self):
        t = ''
        for tl in self.tail:
            t += tl + os.linesep
        return t[:-len(os.linesep)]

    def _tail(self, text):
        if type(text) is not list:
            text = text.splitlines()

        self.tail += text
        if len(self.tail) > self.tail_size:
            self.tail = self.tail[-self.tail_size:]

    def has_stdout(self):
        return self.fhs[0] is not None

    def has_stderr(self):
        return self.fhs[1] is not None

    def output(self, text):
        """Output the text message to all the logs."""
        # Reformat the text to have local line types.
        text = text.replace(chr(13), '').splitlines()
        self._tail(text)
        out = os.linesep.join(text) + os.linesep
        if isinstance(out, bytes):
            out = out.decode('utf-8', 'ignore')
        self.lock.acquire()
        try:
            for f in range(0, len(self.fhs)):
                if self.fhs[f] is not None:
                    safe_blocking_write(self.fhs[f], out)
            self.flush()
        except:
            raise
        finally:
            self.lock.release()

    def flush(self):
        """Flush the output."""
        for f in range(0, len(self.fhs)):
            if self.fhs[f] is not None:
                self.fhs[f].flush()
