#
# RTEMS pretty printers
#
import re
import helper
import objects

import supercore_printer
import classic_printer

pretty_printer = {

    '^rtems_id$'            : supercore_printer.id,
    '^Objects_Id$'          : supercore_printer.id,
    '^Objects_Name$'        : supercore_printer.name,
    '^Objects_Control$'     : supercore_printer.control,
    '^States_Control$'      : supercore_printer.state,
    '^rtems_attribute$'     : classic_printer.attribute,
    '^Semaphore_Control$'   : classic_printer.semaphore
}


def build_pretty_printer ():
    pp_dict = {}

    for name in pretty_printer:
        pp_dict[re.compile(name)] = pretty_printer[name]

    return pp_dict

def lookup_function (val):
    "Look-up and return a pretty-printer that can print val."

    global nesting

    typename = str(helper.type_from_value(val))

    for function in pp_dict:
        if function.search (typename):
            nesting += 1
            result = pp_dict[function] (val)
            nesting -= 1
            if nesting == 0:
                objects.information.invalidate()
            return result

    # Cannot find a pretty printer.  Return None.
    return None

# ToDo: properly document.
nesting = 0

pp_dict = build_pretty_printer()
