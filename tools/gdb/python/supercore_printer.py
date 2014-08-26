# RTEMS Tools Project (http://www.rtems.org/)
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
# RTEMS Supercore pretty printers for GDB
#

import objects
import itertools
import threads

class id:
    """Print an object given the ID. Print using the struct display hint and an
    iterator."""

    class iterator:
        """Use an iterator for each field expanded from the id so GDB output
        is formatted correctly."""

        def __init__(self, id):
            self.id = id
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            self.count += 1
            if self.count == 1:
                return int(self.id.value())
            elif self.count == 2:
                return self.id.node()
            elif self.count == 3:
                return self.id.api()
            elif self.count == 4:
                return self.id._class()
            elif self.count == 5:
                return self.id.index()
            raise StopIteration

    def __init__(self, id):
        self.id = objects.ident(id)

    def to_string(self):
        return ''

    @staticmethod
    def key(i):
        if i == 0:
            return 'id'
        elif i == 1:
            return 'node'
        elif i == 2:
            return 'api'
        elif i == 3:
            return 'class'
        elif i == 4:
            return 'index'
        return 'bad'

    def children(self):
        counter = itertools.imap (self.key, itertools.count())
        return itertools.izip (counter, self.iterator(self.id))

    def display_hint (self):
        return 'struct'

class name:
    """Pretty printer for an object's name. It has to guess the type as no
    information is available to help determine it."""

    def __init__(self, nameval):
        self.name = objects.name(nameval)

    def to_string(self):
        return str(self.name)

class control:

    class iterator:
        """Use an iterator for each field expanded from the id so GDB output
        is formatted correctly."""

        def __init__(self, object):
            self.object = object
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            self.count += 1
            if self.count == 1:
                return self.object.node()
            elif self.count == 2:
                return self.object.id()
            elif self.count == 3:
                return self.object.name()
            raise StopIteration

    def to_string(self):
        return ''

    def __init__(self, object):
        self.object = objects.control(object)

    @staticmethod
    def key(i):
        if i == 0:
            return 'Node'
        elif i == 1:
            return 'id'
        elif i == 2:
            return 'name'
        return 'bad'

    def children(self):
        counter = itertools.imap (self.key, itertools.count())
        return itertools.izip (counter, self.iterator(self.object))

    def display_hint (self):
        return 'struct'


class state:

    def __init__(self, state):
        self.state = threads.state(state)
    def to_string(self):
        return self.state.to_string()

class chains:

    def __init__(self,chain):
        self.chain = chains.control(chain)

    def to_string(self):
        return "First:"+str(self.chain.first())+"\n Last:"+str(self.chain.last())

class node:
    def __init__(self, node):
        self.node = chains.node(node)

    def to_string(self):
        return "Node: "+str(self.node)+" Next: "+str(self.node.next())+" Prev: "+str(self.node.previous())
