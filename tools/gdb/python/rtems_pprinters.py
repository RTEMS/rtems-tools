# SPDX-License-Identifier: BSD-2-Clause

# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2024-2024 Suraj Kumar 
# All rights reserved.

# This package is part of the RTEMS Tools Project.

# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.

# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# This script contains the definitions to all pretty-printers written for RTEMS
# kernel structures. 

# To add a pretty-printer for a structure: 
#   1. Define a class for it and write the printer code 
#   2. Add the structure name along with the class name in the "printers"
#      dictionary 

import gdb.printing

class BasePrettyPrinter:

    def __init__(self, val):
        self.val = val

    def to_string(self):
        raise NotImplementedError("Subclasses should implement this!")

    def display_hint(self):
        return None


# A pretty printer for a singular node in the Chain_Control structure inside
# RTEMS
class ChainNodePrinter(BasePrettyPrinter):
    """Pretty print a Chain_Node."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        next_node = self.val['next']
        previous_node = self.val['previous']
        address = self.val.address
        return (f"Chain_Node(\n"
                f"    address={address},\n"
                f"    next={next_node},\n"
                f"    previous={previous_node}\n"
                f")")


# A pretty-printer for the Chain_Control structure inside RTEMS
class ChainControlPrinter(BasePrettyPrinter):
    """Pretty print a Chain_Control."""

    def __init__(self, val):
        self.val = val

    def walk_chain(self, start_node):
        """Walk through the chain and return a string representation of all nodes."""
        nodes = []
        current_node = start_node
        visited = set()
        permanent_null = gdb.Value(0).cast(gdb.lookup_type('void').pointer())

        while current_node and current_node.address != permanent_null:
            node_address = current_node.cast(
                gdb.lookup_type('Chain_Node').pointer())
            if node_address in visited:
                break
            visited.add(node_address)
            nodes.append(str(ChainNodePrinter(current_node).to_string()))
            next_node_address = current_node['next']
            if next_node_address == permanent_null:
                break
            current_node = next_node_address.cast(
                gdb.lookup_type('Chain_Node').pointer()).dereference()
        return "\n".join(nodes)

    def is_empty(self):
        """Check if the chain is empty."""
        head_node = self.val['Head']['Node']
        tail_node = self.val['Tail']['Node']
        permanent_null = gdb.Value(0).cast(gdb.lookup_type('void').pointer())
        return (head_node['next'] == permanent_null
                and tail_node['previous'] == permanent_null)

    def to_string(self):
        head_node = self.val['Head']['Node']
        head_fill = self.val['Head']['fill']
        tail_node = self.val['Tail']['Node']
        tail_fill = self.val['Tail']['fill']

        empty_status = "empty" if self.is_empty() else "not empty"

        chain_nodes = self.walk_chain(head_node)

        return (f"Chain_Control(\n"
                f"    Head(\n"
                f"        Node(\n"
                f"            address={head_node.address},\n"
                f"            next={head_node['next']},\n"
                f"            previous={head_node['previous']}\n"
                f"        ),\n"
                f"        fill={head_fill}\n"
                f"    ),\n"
                f"    Tail(\n"
                f"        Node(\n"
                f"            address={tail_node.address},\n"
                f"            next={tail_node['next']},\n"
                f"            previous={tail_node['previous']}\n"
                f"        ),\n"
                f"        fill={tail_fill}\n"
                f"    ),\n"
                f"Status: Chain is {empty_status}\n"
                f"Nodes:\n"
                f"{chain_nodes}\n"
                f")")


# A pretty-printer for the POSIX Threads Objects structure
class POSIXThreadsObjectsPrinter(BasePrettyPrinter):
    """Pretty print a _POSIX_Threads_Objects."""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        thread_id = self.val['thread_id']
        stack = self.val['stack']
        priority = self.val['priority']
        chain = self.val['chain']

        return (f"_POSIX_Threads_Objects(\n"
                f"    thread_id={thread_id},\n"
                f"    stack={stack},\n"
                f"    priority={priority},\n"
                f"    chain={chain}\n"
                f")")


printers = {
    'Chain_Node': ChainNodePrinter,
    'Chain_Control': ChainControlPrinter,
    '_POSIX_Threads_Objects': POSIXThreadsObjectsPrinter
}


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("rtems_printers")

    for structure, class_name in printers.items():
        struct_regexp = "^" + structure + "$"
        pp.add_printer(structure, struct_regexp, class_name)

    return pp
