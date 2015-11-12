#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2013-2014 Chris Johns (chrisj@rtems.org)
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

import os
import sys
import termios

from rtemstoolkit import error
from rtemstoolkit import options
from rtemstoolkit import path

def save():
    if not options.host_windows:
        try:
            sin = termios.tcgetattr(sys.stdin)
            sout = termios.tcgetattr(sys.stdout)
            serr = termios.tcgetattr(sys.stderr)
            return sin, sout, serr
        except:
            pass
    return None

def restore(attributes):
    if attributes is not None:
        termios.tcsetattr(sys.stdin, termios.TCSANOW, attributes[0])
        termios.tcsetattr(sys.stdout, termios.TCSANOW, attributes[1])
        termios.tcsetattr(sys.stderr, termios.TCSANOW, attributes[2])

class tty:

    def __init__(self, dev):
        if options.host_windows:
            raise error.general('termios not support on host')
        self.dev = dev
        self.default_attr = None
        self.fd = None
        self.if_on = False
        if options.host_windows:
            raise error.general('TTY consoles not supported on Windows.')
        if not path.exists(dev):
            raise error.general('dev not found: %s' % (dev))
        try:
            self.fd = open(dev, 'rw')
        except IOError as ioe:
            raise error.general('opening tty dev: %s: %s' % (dev, ioe))
        except:
            raise error.general('opening tty dev: %s: unknown' % (dev))
        try:
            self.default_attr = termios.tcgetattr(self.fd)
        except:
            raise error.general('cannot get termios attrs: %s' % (dev))
        self.attr = self.default_attr

    def __del__(self):
        if self.fd and self.default_attr:
            try:
                self.fd.close()
            except:
                pass

    def __str__(self):
        def _input(attr):
            s = ''
            if attr & termios.IGNBRK:
                s += ' IGNBRK'
            if attr & termios.BRKINT:
                s += ' BRKINT'
            if attr & termios.IGNPAR:
                s += ' IGNPAR'
            if attr & termios.PARMRK:
                s += ' PARMRK'
            if attr & termios.INPCK:
                s += ' INPCK'
            if attr & termios.ISTRIP:
                s += ' ISTRIP'
            if attr & termios.INLCR:
                s += ' INLCR'
            if attr & termios.IGNCR:
                s += ' IGNCR'
            if attr & termios.IXON:
                s += ' IXON'
            if attr & termios.IXOFF:
                s += ' IXOFF'
            if attr & termios.IXANY:
                s += ' IXANY'
            if attr & termios.IMAXBEL:
                s += ' IMAXBEL'
            return s

        def _output(attr):
            s = ''
            if attr & termios.OPOST:
                s += ' OPOST'
            if attr & termios.ONLCR:
                s += ' ONLCR'
            if attr & termios.OCRNL:
                s += ' OCRNL'
            if attr & termios.TABDLY:
                s += ' TABDLY'
            if attr & termios.TAB0:
                s += ' TAB0'
            if attr & termios.TAB3:
                s += ' TAB3'
            if attr & termios.ONOCR:
                s += ' ONOCR'
            if attr & termios.ONLRET:
                s += ' ONLRET'
            return s

        def _control(attr):
            s = ''
            if (attr & termios.CSIZE) == termios.CS5:
                s += ' CS5'
            if (attr & termios.CSIZE) == termios.CS6:
                s += ' CS6'
            if (attr & termios.CSIZE) == termios.CS7:
                s += ' CS7'
            if (attr & termios.CSIZE) == termios.CS8:
                s += ' CS8'
            if attr & termios.CSTOPB:
                s += ' CSTOPB'
            if attr & termios.CREAD:
                s += ' CREAD'
            if attr & termios.PARENB:
                s += ' PARENB'
            if attr & termios.PARODD:
                s += ' PARODD'
            if attr & termios.HUPCL:
                s += ' HUPCL'
            if attr & termios.CLOCAL:
                s += ' CLOCAL'
            if attr & termios.CRTSCTS:
                s += ' CRTSCTS'
            return s

        def _local(attr):
            s = ''
            if attr & termios.ECHOKE:
                s += ' ECHOKE'
            if attr & termios.ECHOE:
                s += ' ECHOE'
            if attr & termios.ECHO:
                s += ' ECHO'
            if attr & termios.ECHONL:
                s += ' ECHONL'
            if attr & termios.ECHOPRT:
                s += ' ECHOPRT'
            if attr & termios.ECHOCTL:
                s += ' ECHOCTL'
            if attr & termios.ISIG:
                s += ' ISIG'
            if attr & termios.ICANON:
                s += ' ICANON'
            if attr & termios.IEXTEN:
                s += ' IEXTEN'
            if attr & termios.TOSTOP:
                s += ' TOSTOP'
            if attr & termios.FLUSHO:
                s += ' FLUSHO'
            if attr & termios.PENDIN:
                s += ' PENDIN'
            if attr & termios.NOFLSH:
                s += ' NOFLSH'
            return s

        def _baudrate(attr):
            if attr == termios.B0:
                s = 'B0'
            elif attr == termios.B50:
                s = 'B50'
            elif attr == termios.B75:
                s = 'B75'
            elif attr == termios.B110:
                s = 'B110'
            elif attr == termios.B134:
                s = 'B134'
            elif attr == termios.B150:
                s = 'B150'
            elif attr == termios.B200:
                s = 'B200'
            elif attr == termios.B300:
                s = 'B300'
            elif attr == termios.B600:
                s = 'B600'
            elif attr == termios.B1800:
                s = 'B1800'
            elif attr == termios.B1200:
                s = 'B1200'
            elif attr == termios.B2400:
                s = 'B2400'
            elif attr == termios.B4800:
                s = 'B4800'
            elif attr == termios.B9600:
                s = 'B9600'
            elif attr == termios.B19200:
                s = 'B19200'
            elif attr == termios.B38400:
                s = 'B38400'
            elif attr == termios.B57600:
                s = 'B57600'
            elif attr == termios.B115200:
                s = 'B115200'
            elif attr == termios.B230400:
                s = 'B230400'
            elif attr == termios.B460800:
                s = 'B460800'
            else:
                s = 'unknown'
            return s

        if self.attr is None:
            return 'None'
        s = 'iflag: %s' % (_input(self.attr[0]))
        s += os.linesep + 'oflag: %s' % (_output(self.attr[1]))
        s += os.linesep + 'cflag: %s' % (_control(self.attr[2]))
        s += os.linesep + 'lflag: %s' % (_local(self.attr[3]))
        s += os.linesep + 'ispeed: %s' % (_baudrate(self.attr[4]))
        s += os.linesep + 'ospeed: %s' % (_baudrate(self.attr[5]))
        return s

    def _update(self):
        self.off()
        try:
            termios.tcflush(self.fd, termios.TCIOFLUSH)
            #attr = self.attr
            #attr[0] = termios.IGNPAR;
            #attr[1] = 0
            #attr[2] = termios.CRTSCTS | termios.CS8 | termios.CREAD;
            #attr[3] = 0
            #attr[6][termios.VMIN] = 1
            #attr[6][termios.VTIME] = 2
            #termios.tcsetattr(self.fd, termios.TCSANOW, attr)
            termios.tcsetattr(self.fd, termios.TCSANOW, self.attr)
            termios.tcflush(self.fd, termios.TCIOFLUSH)
        except:
            raise
        if self.is_on:
            self.on()

    def _baudrate_mask(self, flag):
        if flag == 'B0':
            mask = termios.B0
            self.attr[5] = termios.B0
        elif flag == 'B50':
            mask = termios.B50
        elif flag == 'B75':
            mask = termios.B75
        elif flag == 'B110':
            mask = termios.B110
        elif flag == 'B134':
            mask = termios.B134
        elif flag == 'B150':
            mask = termios.B150
        elif flag == 'B200':
            mask = termios.B200
        elif flag == 'B300':
            mask = termios.B300
        elif flag == 'B600':
            mask = termios.B600
        elif flag == 'B1800':
            mask = termios.B1800
        elif flag == 'B1200':
            mask = termios.B1200
        elif flag == 'B2400':
            mask = termios.B2400
        elif flag == 'B4800':
            mask = termios.B4800
        elif flag == 'B9600':
            mask = termios.B9600
        elif flag == 'B19200':
            mask = termios.B19200
        elif flag == 'B38400':
            mask = termios.B38400
        elif flag == 'B57600':
            mask = termios.B57600
        elif flag == 'B115200':
            mask = termios.B115200
        elif flag == 'B230400':
            mask = termios.B230400
        elif flag == 'B460800':
            mask = termios.B460800
        else:
            mask = None
        return mask

    def _input_mask(self, flag):
        if flag == 'IGNBRK':
            mask = termios.IGNBRK
        elif flag == 'BRKINT':
            mask = termios.BRKINT
        elif flag == 'IGNPAR':
            mask = termios.IGNPAR
        elif flag == 'PARMRK':
            mask = termios.PARMRK
        elif flag == 'INPCK':
            mask = termios.INPCK
        elif flag == 'ISTRIP':
            mask = termios.ISTRIP
        elif flag == 'INLCR':
            mask = termios.INLCR
        elif flag == 'IGNCR':
            mask = termios.IGNCR
        elif flag == 'IXON':
            mask = termios.IXON
        elif flag == 'IXOFF':
            mask = termios.IXOFF
        elif flag == 'IXANY':
            mask = termios.IXANY
        elif flag == 'IMAXBEL':
            mask = termios.IMAXBEL
        else:
            mask = None
        return mask

    def _output_mask(self, flag):
        if flag == 'OPOST':
            mask = termios.OPOST
        elif flag == 'ONLCR':
            mask = termios.ONLCR
        elif flag == 'OCRNL':
            mask = termios.OCRNL
        elif flag == 'TABDLY':
            mask = termios.TABDLY
        elif flag == 'TAB0':
            mask = termios.TAB0
        elif flag == 'TAB3':
            mask = termios.TAB3
        elif flag == 'ONOCR':
            mask = termios.ONOCR
        elif flag == 'ONLRET':
            mask = termios.ONLRET
        else:
            mask = None
        return mask

    def _control_mask(self, flag):
        if flag == 'CSTOPB':
            mask = termios.CSTOPB
        elif flag == 'CREAD':
            mask = termios.CREAD
        elif flag == 'PARENB':
            mask = termios.PARENB
        elif flag == 'PARODD':
            mask = termios.PARODD
        elif flag == 'HUPCL':
            mask = termios.HUPCL
        elif flag == 'CLOCAL':
            mask = termios.CLOCAL
        elif flag == 'CRTSCTS':
            mask = termios.CRTSCTS
        else:
            mask = None
        return mask

    def _local_mask(self, flag):
        if flag == 'ECHOKE':
            mask = termios.ECHOKE
        elif flag == 'ECHOE':
            mask = termios.ECHOE
        elif flag == 'ECHO':
            mask = termios.ECHO
        elif flag == 'ECHONL':
            mask = termios.ECHONL
        elif flag == 'ECHOPRT':
            mask = termios.ECHOPRT
        elif flag == 'ECHOCTL':
            mask = termios.ECHOCTL
        elif flag == 'ISIG':
            mask = termios.ISIG
        elif flag == 'ICANON':
            mask = termios.ICANON
        elif flag == 'IEXTEN':
            mask = termios.IEXTEN
        elif flag == 'TOSTOP':
            mask = termios.TOSTOP
        elif flag == 'FLUSHO':
            mask = termios.FLUSHO
        elif flag == 'PENDIN':
            mask = termios.PENDIN
        elif flag == 'NOFLSH':
            mask = termios.NOFLSH
        else:
            mask = None
        return mask

    def _set(self, index, mask, state):
        if state:
            self.attr[index] = self.attr[index] | mask
        else:
            self.attr[index] = self.attr[index] & ~mask

    def off(self):
        if self.fd:
            try:
                termios.tcflow(self.fd, termios.TCOOFF)
            except:
                pass
            try:
                termios.tcflow(self.fd, termios.TCIOFF)
            except:
                pass
            self.is_on = False

    def on(self):
        if self.fd:
            try:
                termios.tcflow(self.fd, termios.TCOON)
            except:
                pass
            try:
                termios.tcflow(self.fd, termios.TCION)
            except:
                pass
            self.is_on = True

    def baudrate(self, flag):
        mask = self._baudrate_mask(flag)
        if mask:
            self.attr[4] = mask
            self.attr[5] = mask
        else:
            raise error.general('invalid setting: %s' % (flag))
        self._update()

    def input(self, flag, on):
        mask = self._input_mask(flag)
        if mask is None:
            raise error.general('invalid input flag: %s' % (flag))
        self._set(0, mask, on)
        self._update()

    def output(self, flag, on):
        mask = self._output_mask(flag)
        if mask is None:
            raise error.general('invalid output flag: %s' % (flag))
        self._set(1, mask, on)
        self._update()

    def control(self, flag, on):
        mask = self._control_mask(flag)
        if mask is None:
            raise error.general('invalid control flag: %s' % (flag))
        self._set(2, mask, on)
        self._update()

    def local(self, flag, on):
        mask = self._local_mask(flag)
        if mask is None:
            raise error.general('invalid local flag: %s' % (flag))
        self._set(3, mask, on)
        self._update()

    def vmin(self, _vmin):
        self.attr[6][termios.VMIN] = _vmin

    def vtime(self, _vtime):
        self.attr[6][termios.VTIME] = _vtime

    def set(self, flags):
        for f in flags.split(','):
            if len(f) < 2:
                raise error.general('invalid flag: %s' % (f))
            if f[0] == '~':
                on = False
                flag = f[1:]
            else:
                on = True
                flag = f
            if f.startswith('VMIN'):
                vs = f.split('=')
                if len(vs) != 2:
                    raise error.general('invalid vmin flag: %s' % (f))
                try:
                    _vmin = int(vs[1])
                except:
                    raise error.general('invalid vmin flag: %s' % (f))
                self.vmin(_vmin)
                continue
            if f.startswith('VTIME'):
                vs = f.split('=')
                if len(vs) != 2:
                    raise error.general('invalid vtime flag: %s' % (f))
                try:
                    _vtime = int(vs[1])
                except:
                    raise error.general('invalid vtime flag: %s' % (f))
                self.vtime(_vtime)
                continue
            mask = self._baudrate_mask(flag)
            if mask:
                if not on:
                    raise error.general('baudrates are not flags: %s' % (f))
                self.attr[4] = mask
                self.attr[5] = mask
                continue
            mask = self._input_mask(flag)
            if mask:
                self._set(0, mask, on)
                continue
            mask = self._output_mask(flag)
            if mask:
                self._set(1, mask, on)
                continue
            mask = self._control_mask(flag)
            if mask:
                self._set(2, mask, on)
                continue
            mask = self._local_mask(flag)
            if mask:
                self._set(3, mask, on)
                continue
            raise error.general('unknown tty flag: %s' % (f))
        self._update()

if __name__ == "__main__":
    if len(sys.argv) == 2:
        t = tty(sys.argv[1])
        t.baudrate('B115200')
        t.input('BRKINT', False)
        t.input('IGNBRK', True)
        t.input('IGNCR', True)
        t.local('ICANON', False)
        t.local('ISIG', False)
        t.local('IEXTEN', False)
        t.local('ECHO', False)
        t.control('CLOCAL', True)
        t.control('CRTSCTS', False)
        t.vmin(1)
        t.vtime(2)
        print(t)
        t.set('B115200,~BRKINT,IGNBRK,IGNCR,~ICANON,~ISIG,~IEXTEN,~ECHO,CLOCAL,~CRTSCTS')
        print(t)
        t.on()
        while True:
            c = t.fd.read(1)
            sys.stdout.write(c)
