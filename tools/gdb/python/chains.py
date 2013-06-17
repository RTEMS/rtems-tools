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
        return self.node_val['next'] == 0

    def next(self):
        if not self.null():
            self.node_val = self.node_val['next'].dereference()

    def previous(self):
        if not self.null():
            self.node_val = self.node_val['previous'].dereference()

    def cast(self, typename):
        if not self.null():
            nodetype = gdb.lookup_type(typename)
            return self.node_val.cast(nodetype)
        return None

class control:
    """Manage the Chain_Control."""

    def __init__(self, ctrl):
        self.ctrl = ctrl

    def first(self):
        return node(self.ctrl['first'].dereference())

    def last(self):
        return node(self.ctrl['first'])
