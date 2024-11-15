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
# RTEMS Chains Support
#

import gdb


class node:
    """Manage the Chain_Node."""

    def __init__(self, node_val):
        if node_val:
            if node_val.type.code == gdb.TYPE_CODE_PTR:
                self.reference = node_val
                self.node_val = node_val.dereference()
            else:
                self.node_val = node_val
                self.reference = node_val.address
        else:
            self.node_val = node_val

    def __str__(self):
        return self.to_string()

    def null(self):
        if not self.node_val:
            return True
        return False

    def next(self):
        if not self.null():
            return self.node_val['next']

    def previous(self):
        if not self.null():
            return self.node_val['previous']

    def cast(self, typename):
        if not self.null():
            nodetype = gdb.lookup_type(typename)
            return self.node_val.cast(nodetype)
        return None

    def to_string(self):
        if self.null():
            return 'NULL'
        return 'Node:%s Next:%s Prev:%s' % (
            self.reference, self.node_val['next'], self.node_val['previous'])


class control:
    """Manage the Chain_Control."""

    def __init__(self, ctrl):
        if ctrl.type.code == gdb.TYPE_CODE_PTR:
            self.reference = ctrl
            self.ctrl = ctrl.dereference()
        else:
            self.ctrl = ctrl
            self.reference = ctrl.address

    def first(self):
        t = node(self.ctrl['Head']['Node'])
        return t

    def tail(self):
        return node(self.ctrl['Tail']['Node'])

    def empty(self):
        if self.tail().previous() == self.reference:
            return True
        return False
