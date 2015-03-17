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
# RTEMS Red/Black Tree Support
#

import gdb

rbt_left = 0
rbt_right = 1

def opp_dir(dir_):
    if dir_ == rbt_left:
        return rbt_right
    return rbt_left


class node:
    """Manage the RBTree_Node_struct."""

    def __init__(self, node_ptr):
        self.node_ptr = node_ptr
        self.node_val = None
        if node_ptr != 0:
            print '}}}}'
            self.node_val = node_ptr.dereference()

    def __str__(self):
        return self.to_string()

    def null(self):
        if not self.node_val:
            return True
        return False

    def pointer(self):
        return self.node_ptr

    def parent(self):
        if not self.null():
            return self.node_val['parent']
        return None

    def child(self):
        if not self.null():
            return self.node_val['child']
        return None

    def left(self):
        if not self.null():
            return self.node_val['child'][rbt_left]
        return None

    def right(self):
        if not self.null():
            return self.node_val['child'][rbt_right]
        return None

    def next(self, dir_):
        next_ = None
        if not self.null():
            current = self.child(color, dir_)
            if current is not None:
                while current is not None:
                    cn = node(current)
                    current = cn.child(opp_dir(dir_))
                    next_ = current
            else:
                # RBTree_Node *parent = node->parent;
                # if ( parent->parent && node == parent->child[ opp_dir ] ) {
                #
                #  node->parent
                if self.parent():
                    # pn = *(node->parent) or pn = *parent
                    pn = node(self.parent())
                    if pn.parent() and (self.pointer() == pn.child(opp_dir(dir_))):
                        next_ = self.parent()
                    else:
                        nn = self
                        while pn.parent():
                            if nn.pointer() != pn.child(dir_):
                                break
                            nn = pn
                            pn = node(pn.parent())
                        if pn.parent():
                            next_ = pn.pointer()
        return next_

    def predecessor(self):
        return self.next(rbt_left)

    def successor(self):
        return self.next(rbt_right)

    def color(self):
        if not self.null():
            return self.node_val['color']
        return None

    def cast(self, typename):
        if not self.null():
            nodetype = gdb.lookup_type(typename)
            return self.node_val.cast(nodetype)
        return None

    def off_tree(self):
        if parent.null():
            return True
        return False

    def to_string(self):
        if self.null():
            return 'NULL'
        print ']]] here'
        return 'Parent:%s Child:%s Color:%s' % (self.node_val['parent'],
                                                self.node_val['child'],
                                                self.node_val['color'])

class control:
    """Manage the RBTree_Control."""

    def __init__(self, ctrl):
        self.ctrl = ctrl

    def root(self):
        """Return the root node of the RBT."""
        return node(self.ctrl['root'])

    def first(self, dir_):
        """ Return the min or max nodes of this RBT."""
        return node(self.ctrl['first'][dir_])

    def empty(self):
        if self.root().null():
            return True
        return False
