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
# RTEMS heap
#

class block:
    '''Abstract a heap block structure'''

    def __init__(self, blk):
        self.block = blk
        self.prev_size = self.block['prev_size']
        self.size_flag = self.block['size_and_flag']

    def null(self):
        if self.block:
            return False
        return True

    def val(self):
        return str(self.block)

    def next(self):
        if not self.null():
            self.block = self.block['next']

    def prev(self):
        if not self.null():
            self.block = self.block['prev']

class stats:
    '''heap statistics'''

    def __init__(self,stat):
        self.stat = stat

    def inst(self):
        i = self.stat['instance']
        return i

    def avail(self):
        val = self.stat['size']
        return val

    def free(self):
        return self.stat['free_size']

    def show(self):
        print('  Instance:',self.inst())
        print('     Avail:',self.avail())
        print('      Free:',self.free())

    # ToDo : incorporate others

class control:
    '''Abstract a heap control structure'''

    def __init__(self, ctl):
        self.ctl = ctl

    def first(self):
        b = block(self.ctl['first_block'])
        return b

    def last(self):
        b = block(self.ctl['last_block'])
        return b

    def free(self):
        b = block(self.ctl['free_list'])
        return b

    def stat(self):
        st = stats(self.ctl['stats'])
        return st

    def show(self):
        fi = self.first()
        la = self.last()

        print('     First:', fi.val())
        print('      Last:', la.val())

        stats = self.stat()
        print('    stats:')
        stats.show()
