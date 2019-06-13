#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2019 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#
# This code builds a bootloader image for an SD card for a range of
# boards on a range of hosts.
#

from __future__ import print_function

import argparse
import copy
import datetime
import os
import sys
import tempfile

from rtemstoolkit import check
from rtemstoolkit import configuration
from rtemstoolkit import error
from rtemstoolkit import execute
from rtemstoolkit import host
from rtemstoolkit import log
from rtemstoolkit import macros
from rtemstoolkit import path
from rtemstoolkit import version

def _check_exes(exes):
    ok = True
    first = True
    for exe in exes:
        log.output('check exe: %s' % (exe))
        if not check.check_exe(None, exe):
            if first:
                log.notice('Host executable(s) not found:')
                first = False
            log.notice(' %s' % (exe))
            ok = False
    return ok

def _command(cmd, cwd):
    e = execute.capture_execution()
    cwd = path.abspath(cwd)
    log.output('>> cwd: %s' % (cwd))
    log.output('> %s' % (cmd))
    exit_code, proc, output = e.shell(cmd, cwd = path.host(cwd))
    output_split = output.split(os.linesep)
    if len(output_split) >= 1 and len(output_split[0]) > 0:
        log.output(['> ' + l for l in output_split])
    log.output('> exit: %d' % (exit_code))
    if exit_code != 0:
        err = 'executing failure: (exit:%d) %s' % (exit_code, cmd)
        raise error.general(err)
    return output

siunits = { 'g': 1024 * 1024 * 1024,
            'm': 1024 * 1024,
            'k': 1024 }

def si_units(self, units):
    if units not in siunits:
        raise error.general('invalid SI unit: %s' % (units))
    return siunits[units]

def _si_parse_size(size):
    orig = size
    units = 1
    suffix = ''
    if size[-1].isalpha():
        suffix = size[-1]
        if suffix not in list(siunits.keys()):
            err = 'invalid SI unit (k, m, g): %s: %s' % (suffix, size)
            raise error.general(err)
        units = siunits[suffix]
        size = size[:-1]
    if not size.isdigit():
        raise error.general('invalid size: %s' % (orig))
    size = int(size)
    return size, suffix, size * units

def _si_size(size):
    si_s, si_u, size = _si_parse_size(size)
    return size

def _si_size_units(size):
    si_s, si_u, size = _si_parse_size(size)
    return si_s, si_u

def _si_from_size(size):
    if isinstance(size, str):
        value = int(size)
        if str(value) != size:
            return size
        size = int(size)
    for u in siunits:
        value = int(size / siunits[u])
        if value != 0:
            return '%d%s' % (value, u)
    return str(size)

class bootloader(object):

    mandatory_configs = [
        'image_size',
        'part_type',
        'part_label',
        'fs_format',
        'fs_size',
        'fs_alignment',
        'tool_prefix',
        'bootloaders'
    ]

    def __init__(self, command_path, build = None, bootloader = None):
        #
        # Check if there is a defaults.mc file under the command path. If so
        # this is the tester being run from within the git repo. If not found
        # assume the tools have been installed and the defaults is in the
        # install prefix.
        #
        boot_ini = 'tools/config/rtems-boot.ini'
        if path.exists(path.join(command_path, boot_ini)):
            rtdir = command_path
        else:
            rtdir = '%{_prefix}/share/rtems'
        boot_ini = '%s/%s' % (rtdir, boot_ini)
        self.build = None
        self.macros = macros.macros(rtdir = rtdir, show_minimal = True)
        self.config = configuration.configuration(raw = False)
        self.load_config(bootloader, self.macros.expand(boot_ini))
        self.clean = True

    def __getitem__(self, key):
        if self.macros.has_key(key) and self.macros[key] != 'None':
            r = self.macros.expand('%%{%s}' % (key))
            if r == '1':
                return True
            elif r == '0':
                return False
            else:
                return r
        return None

    def __setitem__(self, key, value):
        if value is None:
            value = 'None'
        elif isinstance(value, bool):
            value = '1' if value else '0'
        elif isinstance(value, int):
            value = str(value)
        self.macros[key] = value

    def get_mandatory_configs(self):
        return self.mandatory_configs

    def check_mandatory_configs(self):
        for c in self.get_mandatory_configs():
            if not self.macros.has_key(c):
                raise error.general('boot config missing: %s' % (c))

    def load_config(self, bootloader, config):
        self.config.load(config)
        #
        # Check the config file has the basic data and structure.
        #
        bootloaders = self.config.comma_list('default', 'bootloaders')
        for bl in bootloaders:
            if not self.config.has_section(bl):
                raise error.general('boot config: missing bootloader section: %s' % (bl))
            for b in self.config.comma_list(bl, 'boards'):
                if not self.config.has_section(b):
                    raise error.general('boot config: missing board section: %s' % (b))
        #
        # Is the bootloader valid?
        #
        if bootloader is not None and bootloader not in bootloaders:
            raise error.general('boot config: unknown bootloader: %s' % (bootloader))
        self.macros['bootloader'] = str(bootloader)
        self.macros['version_str'] = version.string()
        self.macros['version'] = str(version.version())
        self.macros['revision'] = str(version.revision())
        if version.released():
            self.macros['released'] = '1'
        #
        # Map the config to macros. The [default] section is global. The
        # remaining sections are added as macro maps so the specalised
        # bootloaders can enable reading from a macro map to provide specific
        # values for a specific config such as a board.
        #
        for s in self.config.get_sections():
            if s != 'default':
                self.macros.set_write_map(s, add = True)
            for i in self.config.get_items(s):
                self.macros[i[0]] = i[1]
            self.macros.unset_write_map()
        self.macros.set_read_map('global')
        if bootloader is not None:
            self.macros.set_read_map(bootloader)

    def list_boards(self):
        boards = { }
        for bl in self.config.comma_list('default', 'bootloaders'):
            boards[bl] = self.config.comma_list(bl, 'boards')
        return boards

    def section_macro_map(self, section, nesting_level = 0):
        nesting_level += 1
        if nesting_level >= 100:
            err = 'boot config: too many map levels (looping?): %s' % (section)
            raise error.general(err)
        if section not in self.macros.maps():
            raise error.general('boot config: maps section not found: %s' % (section))
        self.macros.set_read_map(section)
        for s in self.config.comma_list(section, 'uses', err = False):
            self.section_macro_map(s, nesting_level)

    def boards(self):
         return self.config.comma_list(self['bootloader'], 'boards')

    def log(self):
        log.output('Configuration:')
        log.output('    Bootloader: {0}'.format(self.macros['bootloader']))
        log.output('         Image: {0}'.format(self.macros['output']))
        log.output('    Image Size: {0}'.format(self.macros['image_size']))
        log.output('     Part Type: {0}'.format(self.macros['part_type']))
        log.output('     FS Format: {0}'.format(self.macros['fs_format']))
        log.output('       FS Size: {0}'.format(self.macros['fs_size']))
        log.output('      FS Align: {0}'.format(self.macros['fs_alignment']))
        log.output('        Kernel: {0}'.format(self.macros['kernel']))
        log.output('           FDT: {0}'.format(self.macros['fdt']))
        log.output('      Net DHCP: {0}'.format(self.macros['net_dhcp']))
        log.output('        Net IP: {0}'.format(self.macros['net_ip']))
        log.output(' Net Server IP: {0}'.format(self.macros['net_server_ip']))
        log.output('      Net File: {0}'.format(self.macros['net_exe']))
        log.output('       Net FDT: {0}'.format(self.macros['net_fdt']))
        log.output('         Files: {0}'.format(len(self.files())))
        log.output('Macros:')
        macro_str = str(self.macros).split(os.linesep)
        log.output(os.linesep.join([' ' + l for l in macro_str]))

    def get_exes(self):
        return []

    def check_exes(self):
        return _check_exes(self.get_exes())

    def files(self):
        return [f.strip() for f in self.comma_split(self['files']) if len(f) > 0]

    def install_files(self, image, mountpoint):
        pass

    def install_configuration(self, image, mountpoint):
        pass

    def kernel_image(self):
        return self['kernel_image'].replace('@KERNEL@', path.basename(self['kernel']))

    def fdt_image(self):
        return self['fdt_image'].replace('@FDT@', path.basename(self['fdt']))

    def filter_text(self, lines):
        out = []
        for line in lines:
            if '@KERNEL@' in line:
                line = line.replace('@KERNEL@', path.basename(self['kernel']))
            if '@KERNEL_IMAGE@' in line:
                line = line.replace('@KERNEL_IMAGE@', self.kernel_image())
            if '@FDT@' in line:
                line = line.replace('@FDT@', path.basename(self['fdt']))
            if '@FDT_IMAGE@' in line:
                line = line.replace('@FDT_IMAGE@', self.fdt_image())
            if '@NET_SERVER_IP@' in line and self['net_server_ip']is not None:
                line = line.replace('@NET_SERVER_IP@', self['net_server_ip'])
            if '@NET_IP@' in line and self['net_ip'] is not None:
                line = line.replace('@NET_IP@', self['net_ip'])
            if '@NET_BOOTEXE@' in line and self['net_exe'] is not None:
                line = line.replace('@NET_BOOTEXE@', self['net_exe'])
            if '@NET_BOOTFDT@' in line and self['net_fdt'] is not None:
                line = line.replace('@NET_BOOTFDT@', self['net_fdt'])
            out += [line]
        return out

    def comma_split(self, value):
        if value is not None:
            return [s.strip() for s in value.split(',')]
        return []

class uboot_bootloader(bootloader):

    def __init__(self, command_path, build, convert_kernel, paths, board):
        self.uboot = { 'paths': paths, 'board': board }
        self.convert_kernel = convert_kernel
        super(uboot_bootloader, self).__init__(command_path, build, 'u-boot')
        if self.board() not in self.boards():
            raise error.general('board not found: %s' %(self.board()))
        log.output('Board: %s' % (self.board()))
        self.section_macro_map(self.board())
        self.macros.set_read_map(self['bootloader'] + '-templates')
        self.macros.lock_read_map()
        self._check_frist_second_stages()

    def _check_frist_second_stages(self):
        if not self.convert_kernel:
            if self['first_stage'] is not None and \
               not path.exists(self['first_stage']):
                err = 'u-boot: first stage loader not found: %s' % \
                      (self['first_stage'])
                raise error.general(err)
            if self['second_stage'] is not None and \
               not path.exists(self['second_stage']):
                err = 'u-boot: second stage loader not found: %s' % \
                      (self['second_stage'])
                raise error.general(err)

    def load_config(self, bootloader, config):
        super(uboot_bootloader, self).load_config(bootloader, config)
        if not self.convert_kernel:
            paths_count = len(self.uboot['paths'])
            if paths_count == 1:
                self.macros['ubootdir'] = path.abspath(self.uboot['paths'][0])
            elif paths_count == 2:
                    self.macros['first_stage'] = self.uboot['paths'][0]
                    self.macros['second_stage'] = self.uboot['paths'][1]
            else:
                raise error.general('u-boot: invalid number of paths')
        self.macros['mkimage'] = 'mkimage'

    def get_mandatory_configs(self):
        cfgs = super(uboot_bootloader, self).get_mandatory_configs()
        return cfgs + ['objcopy',
                       'arch',
                       'vendor',
                       'board',
                       'config_name',
                       'first_stage',
                       'second_stage',
                       'kernel_converter']

    def board(self):
        return self.uboot['board']

    def get_exes(self):
        exes = super(uboot_bootloader, self).get_exes()
        if self['executables'] is not None:
            exes += self.comma_split(self['executables'])
        return exes

    def install_files(self, image, mountpoint):
        if self['kernel'] is not None:
            kernel_image = self.kernel_convert(image, self['kernel'])
            if self.convert_kernel:
                path.copy(kernel_image, self['output'])
            else:
                image.install(kernel_image, mountpoint)
        if self['fdt'] is not None:
            fdt_image = self.fdt_convert(image, self['fdt'])
            image.install(fdt_image, mountpoint)

    def install_configuration(self, image, mountpoint):
        uenv_txt = self['uenv_txt']
        if uenv_txt is not None:
            log.output('Uenv txt: %s' % (uenv_txt))
            image.install(path.abspath(uenv_txt), mountpoint)
        else:
            template = None
            if self['net_dhcp'] or self['net_ip'] is not None:
                if self['net_dhcp']:
                    template = 'uenv_net_dhcp'
                else:
                    template = 'uenv_net_static'
                if self['net_server_ip']:
                    template += '_sip'
                if self['net_fdt']:
                    template += '_net_fdt'
            else:
                if self['kernel'] is not None:
                    template = 'uenv_exe'
                elif self['fdt'] is not None:
                    template = 'uenv'
                if self['fdt'] is not None:
                    template += '_fdt'
            if template is not None:
                log.notice('Uenv template: %s' % (template))
                uenv_start = self.comma_split(self['uenv_start'])
                uenv_body = self.comma_split(self[template])
                uenv_end = self.comma_split(self['uenv_end'])
                uenv = uenv_start + uenv_body + uenv_end
                image.install_text(self.filter_text(uenv),
                                   path.join(mountpoint, self['boot_config']))

    def kernel_convert(self, image, kernel):
        dst = path.join(path.abspath(self['build']), path.basename(kernel))
        self['kernel_build'] = dst
        log.output('Copy (into build): %s -> %s' % (kernel, dst))
        image.clean_path(dst)
        path.copy(kernel, dst)
        cmds = self.filter_text(self.comma_split(self['kernel_converter']))
        for cmd in cmds:
            _command(cmd, self['build'])
        return self['kernel_image'].replace('@KERNEL@', dst)

    def fdt_convert(self, image, fdt):
        dst = path.join(path.abspath(self['build']), path.basename(fdt))
        self['fdt_build'] = dst
        log.output('Copy (into build): %s -> %s' % (fdt, dst))
        image.clean_path(dst)
        path.copy(fdt, dst)
        return self['fdt_image'].replace('@FDT@', dst)

class image(object):

    def __init__(self, bootloader):
        self.loader = bootloader
        self.detach_images = []
        self.unmount_paths = []
        self.remove_paths = []

    def build(self):
        #
        # Cleanup if any goes wrong.
        #
        try:
            #
            # Ge the absolute paths to fixed locations.
            #
            output = path.abspath(self.loader['output'])
            build = path.abspath(self.loader['build'])
            mountpoint = path.join(build, 'mnt')

            #
            # Create any paths we need. They are removed when this object is
            # deleted.
            #
            self.create_path(build)

            #
            # If only coverting a kernel no need to create an image.
            #
            if not self.loader.convert_kernel:
                self.create_path(mountpoint)

                #
                # Create the blank image file. This is attached as a device,
                # partitioned, formatted and the files written to it.
                #
                log.notice('Create image: %s size %s' % (self.loader['output'],
                                                         self.loader['image_size']))
                self.image_create(output, self.loader['image_size'])

                #
                # Attach the image so it is a device.
                #
                log.notice('Attach image to device: %s' % (self.loader['output']))
                device = self.image_attach(output)

                #
                # Partition the image. The device may change.
                #
                log.notice('Partition device: %s as %s' % (device,
                                                           self.loader['part_type']))
                device = self.partition(output,
                                        device,
                                        self.loader['part_type'],
                                        self.loader['part_label'],
                                        self.loader['fs_format'],
                                        self.loader['fs_size'],
                                        self.loader['fs_alignment'])
                part = self.device_partition(device, 1)

                #
                # Format the first partition.
                #
                log.notice('Format: %s as %s' % (part, self.loader['fs_format']))
                self.format_partition(part, self.loader['fs_format'])

                #
                # Mount the file system.
                #
                log.notice('Mount: %s' % (part))
                self.mount(self.loader['fs_format'], part, mountpoint)

                #
                # Install the first stage and second stage boot loaders.
                #
                self.install(self.loader['first_stage'], mountpoint)
                self.install(self.loader['second_stage'], mountpoint)

            #
            # Install the bootload files.
            #
            self.loader.install_files(self, mountpoint)

            if not self.loader.convert_kernel:
                #
                # Install the bootloader configuration.
                #
                self.loader.install_configuration(self, mountpoint)

                #
                # Install any user files if present.
                #
                for f in self.loader.files():
                    self.install(f, mountpoint)

            #
            # Done.
            #
            log.notice('Finished')
        finally:
            self.cleanup()

    def install(self, src, dst):
        src_base = path.basename(src)
        log.notice('Install: %s' % (src_base))
        asrc = path.abspath(src)
        adst =  path.join(path.abspath(dst), src_base)
        log.output('Copy: %s -> %s' % (asrc, adst))
        path.copy(asrc, adst)

    def install_text(self, text, dst):
        dst_base = path.basename(dst)
        log.notice('Install: %s' % (dst_base))
        adst =  path.abspath(dst)
        log.output('Copy: text[%d] -> %s' % (len(text), adst))
        log.output([' ] ' + l for l in text])
        with open(adst, "w") as o:
            o.write(os.linesep.join(text))

    def image_create(self, path_, size):
        self.host_image_create(path_, size, path.exists(path_))

    def image_attach(self, path_):
        device = self.host_image_attach(path_)
        self.detach_images += [device]
        return device

    def image_detach(self, device):
        if device in self.detach_images:
            self.detach_images.remove(device)
            self.host_image_detach(device)

    def partition(self, image_, device, ptype, plabel, pformat, psize, palign):
        return self.host_partition(image_, device,
                                   ptype, plabel, pformat, psize, palign)

    def format_partition(self, device, pformat):
        self.host_format_partition(device, pformat)

    def device_partition(self, device, pindex):
        return self.host_device_partition(device, pindex)

    def mount(self, pformat, device, path_):
        if path_ not in self.unmount_paths:
            self.host_mount(pformat, device, path_)
            self.unmount_paths += [path_]

    def unmount(self, path_):
        if path_ in self.unmount_paths:
            self.host_unmount(path_)
            self.unmount_paths.remove(path_)

    def cleanup(self):
        log.notice('Cleaning up')
        for m in self.unmount_paths:
            log.output('unmount: %s' % (m))
            self.unmount(m)
        for i in self.detach_images:
            log.output('detach: %s' % (i))
            self.image_detach(i)
        if self.loader.clean:
            for r in self.remove_paths:
                if path.exists(r):
                    log.output('remove: %s' % (r))
                    path.removeall(r)

    def get_exes(self):
        return ['dd']

    def check_exes(self):
        return _check_exes(self.get_exes())

    def clean_path(self, name):
        self.remove_paths += [name]

    def create_path(self, where, recreate = True, cleanup = True):
        if path.exists(where):
            log.output('remove: %s' % (where))
            path.removeall(where)
        try:
            log.output('make: %s' % (where))
            path.mkdir(where)
        except:
            raise error.general('cannot create build path: %s' % (where))
        if not path.isreadable(where):
            raise error.general('build path is not readable: %s' % (where))
        if not path.iswritable(where):
            raise error.general('build path is not writeable: %s' % (where))
        if cleanup:
            self.remove_paths += [where]

    def command(self, cmd):
        return _command(cmd, self.loader['build'])

    def host_image_create(self, path_, size, exists):
        img_size, img_units = _si_size_units(size)
        self.command('dd if=/dev/zero of=%s bs=1%s count=%d' % (path_,
                                                                img_units,
                                                                img_size))

    def host_image_attach(self, path_):
        raise error.general('no platform support: host_image_attach')

    def host_image_detach(self, device):
        raise error.general('no platform support: host_image_detach')

    def host_partition(self, image_, device, ptype, plabel, pformat, psize, palign):
        raise error.general('no platform support: host_partition')

    def host_format_partition(self, device, pformat):
        raise error.general('no platform support: host_format_partition')

    def host_device_partition(self, device, pindex):
        raise error.general('no platform support: host_device_partition')

    def host_mount(self, pformat, device, path_):
        raise error.general('no platform support: host_mount')

    def host_unmount(self, path_):
        raise error.general('no platform support: host_unmount')

class freebsd_image(image):
    def __init__(self, loader):
        super(freebsd_image, self).__init__(loader)

    def get_exes(self):
        exes = super(freebsd_image, self).get_exes()
        return exes + ['mdconfig',
                       'gpart',
                       'newfs_msdos',
                       'mount',
                       'umount']

    def host_image_attach(self, path_):
        return self.command('sudo mdconfig -f %s' % (path_))

    def host_image_detach(self, device):
        self.command('sudo mdconfig -d -u %s' % (device))

    def host_partition(self, image_, device, ptype, plabel, pformat, psize, palign):
        types = { 'MBR': 'MBR' }
        formats = { 'fat16': 'fat16',
                    'fat32': 'fat32' }
        if ptype not in types:
            err = 'unknown type of partitioning: %s' % (ptype)
            raise error.general(err)
        if pformat not in formats:
            raise error.general('unknown format: %s' % (pformat))
        self.command('sudo gpart create -s %s %s' % (types[ptype], device))
        self.command('sudo gpart add -s %s -t %s -a %s %s' % (_si_from_size(psize),
                                                              formats[pformat],
                                                              palign,
                                                              device))
        self.command('sudo gpart set -a active -i 1 %s' % (device))
        return device

    def host_format_partition(self, device, pformat):
        formats = { 'fat16': ('newfs_msdos', '16'),
                    'fat32': ('newfs_msdos', '32') }
        if pformat not in formats:
            raise error.general('unknown format: %s' % (pformat))
        self.command('sudo %s -F %s %s' % (formats[pformat][0],
                                           formats[pformat][1],
                                           device))

    def host_device_partition(self, device, pindex):
        return '/dev/%ss%d' % (device, pindex)

    def host_mount(self, pformat, device, path_):
        formats = { 'fat16': 'msdos',
                    'fat32': 'msdos' }
        if pformat not in formats:
            raise error.general('unknown format: %s' % (pformat))
        self.command('sudo mount -t %s %s %s' % (formats[pformat],
                                                 device,
                                                 path_))

    def host_unmount(self, path_):
        self.command('sudo umount %s' % (path_))

class linux_image(image):
    def __init__(self, loader):
        super(linux_image, self).__init__(loader)

    def get_exes(self):
        exes = super(linux_image, self).get_exes()
        return exes + ['losetup',
                       'fdisk',
                       'mkfs.fat',
                       'mount',
                       'umount']

    def host_image_create(self, path_, size, exists):
        img_size, img_units = _si_size_units(size)
        self.command('dd if=/dev/zero of=%s bs=%s count=%d' % (path_,
                                                               _si_units(img_units),
                                                               img_size))
    def host_image_attach(self, path_):
        return self.command('sudo losetup --partscan --find --show %s' % (path_))

    def host_image_detach(self, device):
        self.command('sudo losetup --detach %s' % (device))

    def host_partition(self, image_, device, ptype, pformat, psize, palign):
        types = { 'MBR': 'MBR' }
        formats = { 'fat16': '6',
                    'fat32': 'b' }
        if ptype not in types:
            err = 'unknown type of partitioning: %s' % (ptype)
            raise error.general(err)
        if pformat not in formats:
            raise error.general('unknown format: %s' % (pformat))
        #
        # Datch the loop back device, we use fdisk on the image to avoid any
        # kernel errors related to re-reading the updated partition data.
        #
        self.host_image_detach(device)
        #
        # This awkward exchange is needed to script fdisk, hmmm.
        #
        with tempfile.NamedTemporaryFile() as tmp:
            s = os.linesep.join(['o',   # create a new empty part table
                                 'n',   # add a new partition
                                 'p',   # primary
                                 '1',   # partition 1
                                 '%d' % (_si_size(palign) / 512),
                                 '%d' % (_si_size(psize) / 512),
                                 't',   # change a partition type
                                 '%s' % (formats[pformat]), # hex code
                                 'a',   # toggle a bootable flag
                                 'p',   # print
                                 'w',   # write table to disk and exit
                                 ''])
            log.output('fdisk script:')
            log.output(s)
            tmp.write(s.encode())
            tmp.seek(0)
            self.command('cat %s | fdisk -t %s %s' % (tmp.name,
                                                      types[ptype],
                                                      image_))
        return self.host_image_attach(image_)

    def host_format_partition(self, device, pformat):
        formats = { 'fat16': ('mkfs.fat', '16'),
                    'fat32': ('mkfs.fat', '32') }
        if pformat not in formats:
            raise error.general('unknown format: %s' % (pformat))
        self.command('sudo %s -F %s %s' % (formats[pformat][0],
                                           formats[pformat][1],
                                           device))

    def host_device_partition(self, device, pindex):
        return '%sp%d' % (device, pindex)

    def host_mount(self, pformat, device, path_):
        options = { 'fat16': '-o uid=%d' % (os.getuid()),
                    'fat32': '-o uid=%d' % (os.getuid()) }
        if pformat in options:
            opts = options[pformat]
        else:
            opts = ''
        self.command('sudo mount %s %s %s' % (opts, device, path_))

    def host_unmount(self, path_):
        self.command('sudo umount %s' % (path_))

class darwin_image(image):
    def __init__(self, loader):
        super(darwin_image, self).__init__(loader)
        if not self.loader['output'].endswith('.img'):
            log.notice('Output file does not end with `.img`. ' + \
                       'Needed on MacOS due to a bug (Id: 51283993)')
            raise error.general('output file does not end with `.img`')

    def get_exes(self):
        exes = super(darwin_image, self).get_exes()
        return exes + ['hdiutil',
                       'diskutil',
                       'fdisk']

    def host_image_attach(self, path_):
        output = self.command('sudo hdiutil attach %s -nomount -nobrowse' % (path_))
        if len(output.split(os.linesep)) != 1 or not output.startswith('/dev/'):
            raise error.general('invalid hdiutil attach outputl; see log')
        return output.strip()

    def host_image_detach(self, device):
        self.command('sudo hdiutil detach %s' % (device))

    def host_partition(self, image_, device, ptype, plabel, pformat, psize, palign):
        types = { 'MBR': 'MBR' }
        formats = { 'fat16': 'MS-DOS FAT16',
                    'fat32': 'MS-DOS FAT32' }
        if ptype not in types:
            err = 'unknown type of partitioning: %s' % (ptype)
            raise error.general(err)
        if pformat not in formats:
            raise error.general('unknown format: %s' % (pformat))
        #
        # Align the parition by adding free space before. Sign.
        #
        cmd  = "sudo diskutil partitionDisk %s 2 %s " % (device, types[ptype])
        cmd += "'Free Space' '%%noformat%%' %s " % (palign)
        cmd += "'%s' %s %s" % (formats[pformat], plabel, psize)
        self.command(cmd)
        #
        # MacOS mounts the filesystem once the partitioning has finished,
        # unmount it as we have no control over the mountpoint.
        #
        self.command('sudo diskutil unmountDisk %s' % (device))
        #
        # This awkward exchange is needed to set the active bit.
        #
        with tempfile.NamedTemporaryFile() as tmp:
            s = os.linesep.join(['f 1', # flag toggle on partition 1
                                 'w',   # write
                                 'p',   # print
                                 'q',   # quit
                                 ''])
            tmp.write(s.encode())
            tmp.seek(0)
            self.command('cat %s | sudo fdisk -y -e %s' % (tmp.name, device))
        return device

    def host_format_partition(self, device, pformat):
        log.output(' * No format stage; done when partitioning')

    def host_device_partition(self, device, pindex):
        return '%ss%d' % (device, pindex)

    def host_mount(self, pformat, device, path_):
        self.command('sudo diskutil mount -mountPoint %s %s' % (path_, device))

    def host_unmount(self, path_):
        self.command('sudo diskutil unmount %s' % (path_))

builders = {
    'freebsd': freebsd_image,
    'linux'  : linux_image,
    'darwin' : darwin_image
}

def load_log(logfile):
    log.default = log.log(streams = [logfile])

def log_default():
    return 'rtems-log-boot-image.txt'

class valid_dir(argparse.Action):
    def __call__(self, parser, namespace, values, option_string = None):
        if type(values) is not list:
            values = [values]
        for value in values:
            if not path.isdir(value):
                raise argparse.ArgumentError(self,
                                             'is not a valid directory: %s' % (value))
            if not path.isreadable(value):
                raise argparse.ArgumentError(self, 'is not readable: %s' % (value))
            if not path.iswritable(value):
                raise argparse.ArgumentError(self, 'is not writeable: %s' % (value))
            setattr(namespace, self.dest, value)

class valid_file(argparse.Action):
    def __call__(self, parser, namespace, value, option_string = None):
        current = getattr(namespace, self.dest)
        if not isinstance(current, list) and current is not None:
            raise argparse.ArgumentError(self,
                                         ' already provided: %s, have %s' % (value,
                                                                             current))
        if not path.isfile(value):
            raise argparse.ArgumentError(self, 'is not a valid file: %s' % (value))
        if not path.isreadable(value):
            raise argparse.ArgumentError(self, 'is not readable: %s' % (value))
        if current is not None:
            value = current + [value]
        setattr(namespace, self.dest, value)

class valid_file_if_exists(argparse.Action):
    def __call__(self, parser, namespace, value, option_string = None):
        current = getattr(namespace, self.dest)
        if not isinstance(current, list) and current is not None:
            raise argparse.ArgumentError(self,
                                         ' already provided: %s, have %s' % (value,
                                                                             current))
        if path.exists(value):
            if not path.isfile(value):
                raise argparse.ArgumentError(self, 'is not a valid file: %s' % (value))
            if not path.isreadable(value):
                raise argparse.ArgumentError(self, 'is not readable: %s' % (value))
        if current is not None:
            value = current + [value]
        setattr(namespace, self.dest, value)

class valid_paths(argparse.Action):
    def __call__(self, parser, namespace, value, option_string = None):
        current = getattr(namespace, self.dest)
        if current is None:
            current = []
        if isinstance(value, list):
            values = value
        else:
            values = [values]
        for value in values:
            if not path.isfile(value) and not path.isdir(value):
                err = 'is not a valid file or directory: %s' % (value)
                raise argparse.ArgumentError(self, err)
            if not path.isreadable(value):
                raise argparse.ArgumentError(self, 'is not readable: %s' % (value))
            current += [value]
        setattr(namespace, self.dest, current)

class valid_format(argparse.Action):
    def __call__(self, parser, namespace, value, option_string = None):
        current = getattr(namespace, self.dest)
        if not isinstance(current, list) and current is not None:
            raise argparse.ArgumentError(self,
                                         ' already provided: %s, have %s' % (value,
                                                                             current))
        if value not in ['fat16', 'fat32']:
            raise argparse.ArgumentError(self, ' invalid format: %s' % (value))
        setattr(namespace, self.dest, value)

class valid_si(argparse.Action):
    def __call__(self, parser, namespace, value, option_string = None):
        current = getattr(namespace, self.dest)
        units = len(value)
        if value[-1].isalpha():
            if value[-1] not in ['k', 'm', 'g']:
                raise argparse.ArgumentError(self,
                                             'invalid SI (k, m, g): %s' % (value[-1]))
            units = -1
        if not value[:units].isdigit():
            raise argparse.ArgumentError(self, 'invalid SI size: %s' % (value))
        setattr(namespace, self.dest, value)

class valid_ip(argparse.Action):
    def __call__(self, parser, namespace, value, option_string = None):
        current = getattr(namespace, self.dest)
        if current is not None:
            raise argparse.ArgumentError(self,
                                         ' already provided: %s, have %s' % (value,
                                                                             current))
        setattr(namespace, self.dest, value)

def run(args = sys.argv, command_path = None):
    ec = 0
    notice = None
    builder = None
    try:
        description  = 'Provide one path to a u-boot build or provide two '
        description += 'paths to the built the first and second stage loaders, '
        description += 'for example a first stage loader is \'MLO\' and a second '
        description += '\'u-boot.img\'. If converting a kernel only provide the '
        description += 'executable\'s path.'

        argsp = argparse.ArgumentParser(prog = 'rtems-boot-image',
                                        description = description)
        argsp.add_argument('-l', '--log',
                           help = 'log file (default: %(default)s).',
                           type = str, default = log_default())
        argsp.add_argument('-v', '--trace',
                           help = 'enable trace logging for debugging.',
                           action = 'store_true')
        argsp.add_argument('-s', '--image-size',
                           help = 'image size in mega-bytes (default: %(default)s).',
                           type = str, action = valid_si, default = '64m')
        argsp.add_argument('-F', '--fs-format',
                           help = 'root file system format (default: %(default)s).',
                           type = str, action = valid_format, default = 'fat16')
        argsp.add_argument('-S', '--fs-size',
                           help = 'root file system size in SI units ' + \
                                  '(default: %(default)s).',
                           type = str, action = valid_si, default = 'auto')
        argsp.add_argument('-A', '--fs-align',
                           help = 'root file system alignment in SI units ' + \
                                  '(default: %(default)s).',
                           type = str, action = valid_si, default = '1m')
        argsp.add_argument('-k', '--kernel',
                           help = 'install the kernel (default: %(default)r).',
                           type = str, action = valid_file, default = None)
        argsp.add_argument('-d', '--fdt',
                           help = 'Flat device tree source/blob (default: %(default)r).',
                           type = str, action = valid_file, default = None)
        argsp.add_argument('-f', '--file',
                           help = 'install the file (default: None).',
                           type = str, action = valid_file, default = [])
        argsp.add_argument('--net-boot',
                           help = 'configure a network boot using TFTP ' + \
                                  '(default: %(default)r).',
                           action = 'store_true')
        argsp.add_argument('--net-boot-dhcp',
                           help = 'network boot using dhcp (default: %(default)r).',
                           action = 'store_true', default = False)
        argsp.add_argument('--net-boot-ip',
                           help = 'network boot IP address (default: %(default)r).',
                           type = str, action = valid_ip, default = None)
        argsp.add_argument('--net-boot-server',
                           help = 'network boot server IP address ' + \
                                  '(default: %(default)r).',
                           type = str, action = valid_ip, default = None)
        argsp.add_argument('--net-boot-file',
                           help = 'network boot file (default: %(default)r).',
                           type = str, default = 'rtems.img')
        argsp.add_argument('--net-boot-fdt',
                           help = 'network boot load a fdt file (default: %(default)r).',
                           type = str, default = None)
        argsp.add_argument('-U', '--custom-uenv',
                           help = 'install the custom uEnv.txt file ' + \
                                  '(default: %(default)r).',
                           type = str, action = valid_file, default = None)
        argsp.add_argument('-b', '--board',
                           help = 'name of the board (default: %(default)r).',
                           type = str, default = 'list')
        argsp.add_argument('--convert-kernel',
                           help = 'convert a kernel to a bootoader image ' + \
                                  '(default: %(default)r).',
                           action = 'store_true', default = False)
        argsp.add_argument('--build',
                           help = 'set the build directory (default: %(default)r).',
                           type = str, default = 'ribuild')
        argsp.add_argument('--no-clean',
                           help = 'do not clean when finished (default: %(default)r).',
                           action = 'store_false', default = True)
        argsp.add_argument('-o', '--output',
                           help = 'image output file name',
                           type = str, action = valid_file_if_exists, required = True)
        argsp.add_argument('paths',
                           help = 'files or paths, the number and type sets the mode.',
                           nargs = '+', action = valid_paths)

        argopts = argsp.parse_args(args[1:])

        load_log(argopts.log)
        log.notice('RTEMS Tools - Boot Image, %s' % (version.string()))
        log.output(log.info(args))
        log.tracing = argopts.trace

        if argopts.net_boot_dhcp or \
           argopts.net_boot_ip is not None:
            if argopts.convert_kernel:
                raise error.general('net boot options not valid with kernel convert.')
            if argopts.custom_uenv is not None:
                raise error.general('cannot set custom uenv and net boot options.')

        host.load()

        log.output('Platform: %s' % (host.name))

        if argopts.board == 'list':
            loader = bootloader(command_path)
            boards = loader.list_boards()
            log.notice(' Board list: bootloaders (%d)' % (len(boards)))
            for bl in sorted(boards):
                log.notice('  %s: %d' % (bl, len(boards[bl])))
                for b in boards[bl]:
                    log.notice('   ' + b)
            raise error.exit()

        loader = uboot_bootloader(command_path,
                                  argopts.build,
                                  argopts.convert_kernel,
                                  argopts.paths,
                                  argopts.board)

        loader.check_mandatory_configs()

        if loader.convert_kernel:
            if argopts.kernel is not None:
                raise error.general('kernel convert does not use the kernel option.')
            if len(argopts.paths) != 1:
                raise error.general('kernel convert take a single path.')
            argopts.kernel = argopts.paths[0]
        else:
            loader.clean = argopts.no_clean

        loader['build'] = argopts.build
        loader['board'] = argopts.board
        loader['output'] = argopts.output
        loader['image_size'] = argopts.image_size
        loader['fs_format'] = argopts.fs_format
        loader['fs_size'] = argopts.fs_size
        loader['fs_align'] = argopts.fs_align
        loader['kernel'] = argopts.kernel
        loader['fdt'] = argopts.fdt
        loader['files'] = ','.join(argopts.file)
        loader['net_dhcp'] = argopts.net_boot_dhcp
        loader['net_ip'] = argopts.net_boot_ip
        loader['net_server_ip'] = argopts.net_boot_server
        loader['net_exe'] = argopts.net_boot_file
        loader['net_fdt'] = argopts.net_boot_fdt
        loader['uenv_txt'] = argopts.custom_uenv

        loader.log()

        if not loader.convert_kernel:
            if loader['fs_size'] == 'auto':
                loader['fs_size'] = \
                    str(_si_size(loader['image_size']) - _si_size(loader['fs_align']))
            elif _si_size(loader['image_size']) < \
                 _si_size(loader['fs_align']) + _si_size(loader['fs_size']):
                raise error.general('filesystem partition size larger than image size.')

        if host.name not in builders:
            err = 'no builder; platform not supported: %s' % (host.name)
            raise error.general(err)

        builder = builders[host.name](loader)

        if not loader.check_exes() or not builder.check_exes():
            raise error.general('command(s) not found; please fix.')

        builder.build()

    except error.general as gerr:
        notice = str(gerr)
        ec = 1
    except error.internal as ierr:
        notice = str(ierr)
        ec = 1
    except error.exit as eerr:
        pass
    except KeyboardInterrupt:
        notice = 'abort: user terminated'
        ec = 1
    except:
        raise
        notice = 'abort: unknown error'
        ec = 1
    if builder is not None:
        del builder
    if notice is not None:
        log.stderr(notice)
    sys.exit(ec)

if __name__ == "__main__":
    run()
