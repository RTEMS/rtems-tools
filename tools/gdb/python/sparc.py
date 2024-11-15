#
# RTEMS gdb extensions
# sparc archetecture specific abstractions

from helper import test_bit


class register:
    '''SPARC Registers'''

    class psr:
        '''status register'''

        sv_table = {0: 'user', 1: 'superviser'}

        def __init__(self, psr):
            self.psr = psr

        def current_window(self):
            return int(self.psr & 0xf)

        def traps(self):
            return test_bit(self.psr, 5)

        def prev_superviser(self):
            return int(test_bit(self.psr, 6))

        def superviser(self):
            return int(test_bit(self.psr, 7))

        def interrupt_level(self):
            # bits 8 to 11
            return (self.spr & 0x780) >> 7

        def floating_point_status(self):
            return test_bit(self.psr, 12)

        def coproc_status(self):
            return test_bit(self.psr, 13)

        def carry(self):
            return test_bit(self.psr, 20)

        def overflow(self):
            return test_bit(self.psr, 21)

        def zero(self):
            return test_bit(self.psr, 22)

        def icc(self):
            n = test_bit(self.psr, 23)
            z = test_bit(self.psr, 22)
            v = test_bit(self.psr, 21)
            c = test_bit(self.psr, 20)
            return (n, z, v, c)

        def to_string(self):
            val = "     Status Register"
            val += "\n         R Window : " + str(self.current_window())
            val += "\n    Traps Enabled : " + str(self.traps())
            val += "\n   Flaoting Point : " + str(self.floating_point_status())
            val += "\n      Coprocessor : " + str(self.coproc_status())
            val += "\n   Processor Mode : " + self.sv_table[self.superviser()]
            val += "\n       Prev. Mode : " + self.sv_table[self.superviser()]
            val += "\n            Carry : " + str(int(self.carry()))
            val += "\n         Overflow : " + str(int(self.overflow()))
            val += "\n             Zero : " + str(int(self.zero()))

            return val

    def __init__(self, reg):
        self.reg = reg

    def global_regs(self):
        val = [self.reg['g0_g1']]

        for i in range(2, 7):
            val.append(int(self.reg['g' + str(i)]))
        return val

    def local_regs(self):
        val = []

        for i in range(0, 8):
            val.append(self.reg['l' + str(i)])
        return val

    def in_regs(self):
        val = []

        for i in range(0, 8):
            if i == 6:
                val.append(self.reg['i6_fp'])
            else:
                val.append(self.reg['i' + str(i)])
        return val

    def out_regs(self):
        val = []

        for i in range(0, 8):
            if i == 6:
                val.append(self.reg['o6_sp'])
            else:
                val.append(self.reg['o' + str(i)])
        return val

    def status(self):
        return self.psr(self.reg['psr'])

    def show(self):
        print('         Global Regs:', )
        print(' [', )
        for i in self.global_regs():
            print(str(i) + ',', )
        print('\b\b ]')

        print('          Local Regs:', )
        print(' [', )
        for i in self.local_regs():
            print(str(i) + ',', )
        print('\b\b ]')

        print('             In Regs:', )
        print(' [', )
        for i in self.in_regs():
            print(str(i) + ',', )
        print('\b\b ]')

        print('            Out Regs:', )
        print(' [', )
        for i in self.out_regs():
            print(str(i) + ',', )
        print('\b\b ]')

        sr = self.status()
        print(sr.to_string())
