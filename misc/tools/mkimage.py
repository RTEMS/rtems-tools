#!/usr/bin/env python2

# A quickly bashed together replacement for u-boot's mkimage written in python
#
# Copyright 2010 Craig Barker 
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, 
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
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


from optparse import OptionParser
from struct import *
import sys
import os.path
import time
import binascii

MAGIC = 0x27051956
IMG_NAME_LENGTH = 32

archs = {'invalid':0, 'alpha':1, 'arm':2, 'x86':3, 'ia64':4, 'm68k':12,
         'microblaze':14, 'mips':5, 'mips64':6, 'nios':13, 'nios2':15,
         'powerpc':7, 'ppc':7, 's390':8, 'sh':9, 'sparc':10,
         'sparc64':11, 'blackfin':16, 'arv32':17, 'st200':18 }

oss   = {'invalid':0, 'openbsd':1, 'netbsd':2, 'freebsd':3, '4_4bsd':4,
         'linux':5, 'svr4':6, 'esix':7, 'solaris':8, 'irix':9,
         'sco':10, 'dell':11, 'ncr':12, 'lynos':13, 'vxworks':14,
         'psos':15, 'qnx':16, 'u-boot':17, 'rtems':18, 'artos':19,
         'unity':20, 'integrity':21 }

types = {'invalid':0, 'standalone':1, 'kernel':2, 'ramdisk':3, 'multi':4,
         'firmware':5,'script':6, 'filesystem':7, 'flat_dt':8 }

comps = {'none':0, 'bzip2':2, 'gzip':1, 'lzma':3 }

usage = "usage: %prog [options] image"
parser = OptionParser(usage=usage)
parser.add_option("-A","--arch", dest="arch", default="powerpc", 
                  help="set architecture to 'arch'", metavar="ARCH")
parser.add_option("-O","--os", dest="os", default="linux",
                  help="set operating system to 'os'", metavar="OS")
parser.add_option("-T","--type", dest="type", default="kernel",
                  help="set image type to 'type'", metavar="TYPE")
parser.add_option("-C","--comp", dest="comp", default="gzip",
                  help="set compression type 'comp'", metavar="COMP")
parser.add_option("-a","--addr", dest="addr", default="0",
                  help="set load address to 'addr' (hex)", metavar="ADDR")
parser.add_option("-e","--ep", dest="ep", default="0",
                  help="set entry point to 'ep' (hex)", metavar="EP")
parser.add_option("-n","--name", dest="name", default="",
                  help="set image name to 'name'", metavar="NAME")
parser.add_option("-d","--datafile", dest="datafile",
                  help="use image data from 'datafile'", metavar="DATAFILE")
parser.add_option("-x","--xip", action="store_true", dest="xip", default=False,
                  help="set XIP (execute in place)")

(options, args) = parser.parse_args()

if len(args) != 1: parser.print_help()

if options.arch not in archs: 
        print "Invalid architecture specified, aborting"
        sys.exit(2)

if options.os not in oss:
        print "Invalid operating system specified, aborting"
        sys.exit(2) 

if options.comp not in comps:
        print "Invalid compression specified, aborting"
        sys.exit(2) 

if options.type not in types:
        print "Invalid image type specified, aborting"
        sys.exit(2) 

try:
        inputsize = os.path.getsize(options.datafile)
        inputfile = open(options.datafile, 'rb')

except IOError:
        print "Invalid datafile specified, aborting"
        sys.exit(2)

try:
        outputfile = open(args[0],'wb')

except IOError:
        print "Error opening output file for writing, aborting"
        sys.exit(1)

struct = Struct("!IIIIIIIBBBB"+str(IMG_NAME_LENGTH)+"s")

outputfile.seek(struct.size);

inputcrc = 0;

while True:
        inputblock = inputfile.read(4096)
        if not inputblock: break
        inputcrc = binascii.crc32(inputblock, inputcrc)
        outputfile.write(inputblock)

inputcrc = inputcrc & 0xffffffff

structdata = struct.pack(MAGIC, 0, int(time.time()), inputsize, 
                int(options.addr,16), int(options.ep,16), inputcrc, 
                oss[options.os], archs[options.arch], types[options.type], 
                comps[options.comp], options.name)

headercrc = binascii.crc32(structdata) & 0xFFFFFFFF

structdata =  struct.pack(MAGIC, headercrc, int(time.time()), inputsize,
                int(options.addr,16), int(options.ep,16), inputcrc,
                oss[options.os], archs[options.arch], types[options.type],
                comps[options.comp], options.name)

outputfile.seek(0)
outputfile.write(structdata)
outputfile.close()
inputfile.close()
