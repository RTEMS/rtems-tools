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
# RTEMS pretty printers
#

import re
import helper
import objects

import supercore_printer
import classic_printer

pretty_printer = {
    '^rtems_id$': supercore_printer.id,
    '^Objects_Id$': supercore_printer.id,
    '^Objects_Name$': supercore_printer.name,
    '^Objects_Control$': supercore_printer.control,
    '^States_Control$': supercore_printer.state,
    '^rtems_attribute$': classic_printer.attribute,
    '^Semaphore_Control$': classic_printer.semaphore
}


def build_pretty_printer():
    pp_dict = {}

    for name in pretty_printer:
        pp_dict[re.compile(name)] = pretty_printer[name]

    return pp_dict


def lookup_function(val):
    "Look-up and return a pretty-printer that can print val."

    global nesting

    typename = str(helper.type_from_value(val))

    for function in pp_dict:
        if function.search(typename):
            nesting += 1
            result = pp_dict[function](val)
            nesting -= 1
            if nesting == 0:
                objects.information.invalidate()
            return result

    # Cannot find a pretty printer.  Return None.
    return None


# ToDo: properly document.
nesting = 0

pp_dict = build_pretty_printer()
