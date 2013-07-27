#
# RTEMS Classic pretty printers for GDB
#
import classic
import gdb

class attribute:

    def __init__(self, attribute):
        ''' ToDo: Verify - usage of all '''
        self.attr = classic.attribute(attribute,'all')

    def to_string(self):
        return gdb.Value(self.attr.to_string())

class semaphore:
    """Print a Semaphore_Control object. Print using the struct display hint
    and an iterator. """

    class iterator:
        """Use an iterator for each field expanded from the id so GDB output
        is formatted correctly."""

        def __init__(self, semaphore):
            self.semaphore = semaphore
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            self.count += 1
            if self.count == 1:
                return self.semaphore['Object']
            elif self.count == 2:
                attr = attribute(self.semaphore['attribute_set'],
                                 'semaphore')
                return attr.to_string()
            elif self.count == 3:
                return self.semaphore['Core_control']
            raise StopIteration

    def __init__(self, semaphore):
        self.semaphore = semaphore

    def to_string(self):
        return ''

    @staticmethod
    def key(i):
        if i == 0:
            return 'Object'
        elif i == 1:
            return 'attribute_set'
        elif i == 2:
            return 'Core_control'
        return 'bad'

    def children(self):
        counter = itertools.imap (self.key, itertools.count())
        return itertools.izip (counter, self.iterator(self.semaphore))

    def display_hint (self):
        return 'struct'
