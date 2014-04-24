#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2014 Chris Johns (chrisj@rtems.org)
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
# All paths in defaults must be Unix format. Do not store any Windows format
# paths in the defaults.
#
# Every entry must describe the type of checking a host must pass.
#
# Records:
#  key: type, attribute, value
#   type     : none, dir, exe, triplet
#   attribute: none, required, optional
#   value    : 'single line', '''multi line'''
#

#
# The Xilinx Zync ZC706 board connected via OpenOCD and a JTAG pod. The console
# is connected to a tty device.
#
[global]
bsp:                          none,    none,   'beagleboardxm'
jobs:                         none,    none,   '1'

[beagleboardxm]
beagleboardxm:            none,    none,   '%{_rtscripts}/gdb.cfg'
beagleboardxm_arch:       none,    none,   'arm'
bsp_tty_dev:              none,    none,   '/dev/cuaU3'
gdb_script:               none,    none,   'beagleboardxm_gdb_script'
beagleboardxm_gdb_script: none,    none,   '''target remote kaka:3333
                                              mon beagleboard_xm_mlo /home/chris/development/rtems/bb/uboot/u-boot/MLO
                                              load
                                              b _Terminate
                                              continue'''
