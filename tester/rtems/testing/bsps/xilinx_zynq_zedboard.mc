#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2017 Chris Johns (chrisj@rtems.org)
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
# The Xilinx Zynq Zedboard and Microzed board connected via TFTP. The console
# is connected to a telnet tty device.
#
[global]
bsp:                       none,    none,   'xilinx_zynq_zedboard'
jobs:                      none,    none,   '1'

[xilinx_zynq_zedboard]
xilinx_zynq_zedboard:      none,    none,   '%{_rtscripts}/tftp.cfg'
xilinx_zynq_zedboard_arch: none,    none,   'arm'
test_restarts:             none,    none,   '3'
target_reset_command:      none,    none,   'wemo-reset CSEng1 3'
target_reset_regex:        none,    none,   '^U-Boot SPL .*'
bsp_tty_dev:               none,    none,   'tuke:30005'
