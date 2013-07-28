#
# RTEMS heap
#

class block:
    '''Abstract a heap block structure'''

    def __init__(self, blk):
        self.block = blk
        self.prev_size = self.block['prev_size']
        self.size_flag = self.block['size_and_flag']

    def null(self):
        if self.block:
            return False
        return True

    def val(self):
        return str(self.block)

    def next(self):
        if not self.null():
            self.block = self.block['next']

    def prev(self):
        if not self.null():
            self.block = self.block['prev']

class stats:
    '''heap statistics'''

    def __init__(self,stat):
        self.stat = stat

    def inst(self):
        i = self.stat['instance']
        return i

    def avail(self):
        val = self.stat['size']
        return val

    def free(self):
        return self.stat['free_size']

    def show(self):
        print '  Instance:',self.inst()
        print '     Avail:',self.avail()
        print '      Free:',self.free()

    # ToDo : incorporate others

class control:
    '''Abstract a heap control structure'''

    def __init__(self, ctl):
        self.ctl = ctl

    def first(self):
        b = block(self.ctl['first_block'])
        return b

    def last(self):
        b = block(self.ctl['last_block'])
        return b

    def free(self):
        b = block(self.ctl['free_list'])
        return b

    def stat(self):
        st = stats(self.ctl['stats'])
        return st

    def show(self):
        fi = self.first()
        la = self.last()

        print '     First:', fi.val()
        print '      Last:', la.val()

        stats = self.stat()
        print '    stats:'
        stats.show()