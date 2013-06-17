
if __name__ == "__main__":
    import sys
    import os.path
    sys.path.append(os.path.dirname(__file__))
    import chains
    import rtems
    import classic
    import objects
    import threads
    reload(chains)
    reload(rtems)
    reload(classic)
    reload(objects)
    reload(threads)
    print 'RTEMS GDB Support loaded'
