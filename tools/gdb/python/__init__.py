
if __name__ == "__main__":
    import sys
    import os.path
    sys.path.append(os.path.dirname(__file__))
    import supercore
    import chains
    import rtems
    import classic
    import objects
    import threads

    import supercore_printer
    import classic_printer

    # Needed to reload code inside gdb source command
    reload(supercore)
    reload(chains)
    reload(rtems)
    reload(classic)
    reload(objects)
    reload(threads)
    reload(supercore_printer)
    reload(classic_printer)
    print 'RTEMS GDB Support loaded'
