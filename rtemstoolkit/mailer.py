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
# Manage emailing results or reports.
#

import os
import smtplib
import socket

import error
import options
import path

def append_options(opts):
    opts['--mail'] = 'Send email report or results.'
    opts['--smtp-host'] = 'SMTP host to send via.'
    opts['--mail-to'] = 'Email address to send the email too.'
    opts['--mail-from'] = 'Email address the report is from.'

class mail:
    def __init__(self, opts):
        self.opts = opts

    def from_address(self):

        def _clean(l):
            if '#' in l:
                l = l[:l.index('#')]
            if '\r' in l:
                l = l[:l.index('r')]
            if '\n' in l:
                l = l[:l.index('\n')]
            return l.strip()

        addr = self.opts.get_arg('--mail-from')
        if addr is not None:
            return addr[1]
        mailrc = None
        if 'MAILRC' in os.environ:
            mailrc = os.environ['MAILRC']
        if mailrc is None and 'HOME' in os.environ:
            mailrc = path.join(os.environ['HOME'], '.mailrc')
        if mailrc is not None and path.exists(mailrc):
            # set from="Joe Blow <joe@blow.org>"
            try:
                mrc = open(mailrc, 'r')
                lines = mrc.readlines()
                mrc.close()
            except IOError, err:
                raise error.general('error reading: %s' % (mailrc))
            for l in lines:
                l = _clean(l)
                if 'from' in l:
                    fa = l[l.index('from') + len('from'):]
                    if '=' in fa:
                        addr = fa[fa.index('=') + 1:].replace('"', ' ').strip()
            if addr is not None:
                return addr
        addr = self.opts.defaults.get_value('%{_sbgit_mail}')
        return addr

    def smtp_host(self):
        host = self.opts.get_arg('--smtp-host')
        if host is not None:
            return host[1]
        host = self.opts.defaults.get_value('%{_mail_smtp_host}')
        if host is not None:
            return host
        return 'localhost'

    def send(self, to_addr, subject, body):
        from_addr = self.from_address()
        msg = "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n" % \
            (from_addr, to_addr, subject) + body
        try:
            s = smtplib.SMTP(self.smtp_host())
            s.sendmail(from_addr, [to_addr], msg)
        except smtplib.SMTPException, se:
            raise error.general('sending mail: %s' % (str(se)))
        except socket.error, se:
            raise error.general('sending mail: %s' % (str(se)))

if __name__ == '__main__':
    import sys
    optargs = {}
    append_options(optargs)
    opts = options.load(sys.argv, optargs = optargs, defaults = 'defaults.mc')
    m = mail(opts)
    print 'From: %s' % (m.from_address())
    print 'SMTP Host: %s' % (m.smtp_host())
    m.send(m.from_address(), 'Test mailer.py', 'This is a test')
