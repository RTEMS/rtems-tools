#
# RTEMS Chains Support
# Copyright 2010 Chris Johns (chrisj@rtems.org)
#
# $Id$
#

import gdb

class node:
    """Manage the Chain_Node."""

    def __init__(self, node_val):
        self.node_val = node_val

    def null(self):
        if not self.node_val:
            return True
        return False

    def next(self):
        if not self.null():
            self.node_val = self.node_val['next']

    def previous(self):
        if not self.null():
            self.node_val = self.node_val['previous']

    def cast(self, typename):
        if not self.null():
            nodetype = gdb.lookup_type(typename)
            return self.node_val.cast(nodetype)
        return None

    def to_string(self):
        return self.node_val['next'] + "Prev: "+self.node_val['previous']

class control:
    """Manage the Chain_Control."""

    def __init__(self, ctrl):
        self.ctrl = ctrl

    def first(self):
        t = node(self.ctrl['Head']['Node'])
        return t

    def last(self):
        return node(self.ctrl['Tail']['Node'])

