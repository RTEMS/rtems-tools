#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010-2016 Chris Johns (chrisj@rtems.org)
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
# Manage paths locally. The internally the path is in Unix or shell format and
# we convert to the native format when performing operations at the Python
# level. This allows macro expansion to work.
#

from __future__ import print_function

import glob
import os
import shutil
import string
import sys

from rtemstoolkit import error
from rtemstoolkit import log
windows_posix = sys.platform == 'msys'
windows = os.name == 'nt'

win_maxpath = 254

def host(path):
    if path is not None:
        while '//' in path:
            path = path.replace('//', '/')
        if windows:
            if len(path) > 2 and \
               path[0] == '/' and path[2] == '/' and \
               (path[1] in string.ascii_lowercase or \
                path[1] in string.ascii_uppercase):
                path = '%s:%s' % (path[1], path[2:])
            path = path.replace('/', '\\')
            if len(path) > win_maxpath:
                if path.startswith('\\\\?\\'):
                    path = path[4:]
                path = u'\\'.join([u'\\\\?', path])
    return path

def shell(path):
    if isinstance(path, bytes):
        path = path.decode('ascii')
    if path is not None:
        if windows or windows_posix:
            path = path.encode('ascii', 'ignore').decode('ascii')
            if path.startswith('\\\\?\\'):
                path = path[4:]
            if len(path) > 1 and path[1] == ':':
                path = '/%s%s' % (path[0].lower(), path[2:])
            path = path.replace('\\', '/')
        while '//' in path:
            path = path.replace('//', '/')
    return path

def basename(path):
    path = shell(path)
    return shell(os.path.basename(path))

def dirname(path):
    path = shell(path)
    return shell(os.path.dirname(path))

def is_abspath(path):
    if path is not None and len(path) > 0:
        return '/' == path[0]
    return False

def join(path, *args):
    path = shell(path)
    for arg in args:
        if len(path):
            path += '/' + shell(arg)
        else:
            path = shell(arg)
    return shell(path)

def abspath(path):
    path = shell(path)
    return shell(os.path.abspath(host(path)))

def relpath(path, start = None):
    path = shell(path)
    if start is None:
        path = os.path.relpath(host(path))
    else:
        path = os.path.relpath(host(path), start)
    return shell(path)

def splitext(path):
    path = shell(path)
    root, ext = os.path.splitext(host(path))
    return shell(root), ext

def listdir(path, error = True):
    path = host(path)
    files = []
    if not os.path.exists(path):
        if error:
            raise error.general('path does not exist : %s' % (path))
    elif not isdir(path):
        if error:
            raise error.general('path is not a directory: %s' % (path))
    else:
        if windows:
            try:
                files = os.listdir(host(path))
            except IOError:
                raise error.general('Could not list files: %s' % (path))
            except OSError as e:
                raise error.general('Could not list files: %s: %s' % (path, str(e)))
            except WindowsError as e:
                raise error.general('Could not list files: %s: %s' % (path, str(e)))
        else:
            try:
                files = os.listdir(host(path))
            except IOError:
                raise error.general('Could not list files: %s' % (path))
            except OSError as e:
                raise error.general('Could not list files: %s: %s' % (path, str(e)))
    return files

def exists(paths):
    def _exists(p):
        if not is_abspath(p):
            p = shell(join(os.getcwd(), host(p)))
        return basename(p) in ['.'] + listdir(dirname(p), error = False)

    if type(paths) == list:
        results = []
        for p in paths:
            results += [_exists(shell(p))]
        return results
    return _exists(shell(paths))

def isdir(path):
    path = shell(path)
    return os.path.isdir(host(path))

def isfile(path):
    path = shell(path)
    return os.path.isfile(host(path))

def isabspath(path):
    path = shell(path)
    return path[0] == '/'

def isreadable(path):
    path = shell(path)
    return os.access(host(path), os.R_OK)

def iswritable(path):
    path = shell(path)
    return os.access(host(path), os.W_OK)

def isreadwritable(path):
    path = shell(path)
    return isreadable(path) and iswritable(path)

def ispathwritable(path):
    path = host(path)
    while len(path) != 0:
        if exists(path):
            return iswritable(path)
        path = os.path.dirname(path)
    return False

def mkdir(path):
    path = host(path)
    if exists(path):
        if not isdir(path):
            raise error.general('path exists and is not a directory: %s' % (path))
    else:
        if windows:
            try:
                os.makedirs(host(path))
            except IOError:
                raise error.general('cannot make directory: %s' % (path))
            except OSError as e:
                raise error.general('cannot make directory: %s: %s' % (path, str(e)))
            except WindowsError as e:
                raise error.general('cannot make directory: %s: %s' % (path, str(e)))
        else:
            try:
                os.makedirs(host(path))
            except IOError:
                raise error.general('cannot make directory: %s' % (path))
            except OSError as e:
                raise error.general('cannot make directory: %s: %s' % (path, str(e)))

def chdir(path):
    path = shell(path)
    os.chdir(host(path))

def removeall(path):
    #
    # Perform the removal of the directory tree manually so we can
    # make sure on Windows the files are correctly encoded to avoid
    # the file name size limit. On Windows the os.walk fails once we
    # get to the max path length.
    #
    def _isdir(path):
        hpath = host(path)
        return os.path.isdir(hpath) and not os.path.islink(hpath)

    def _remove_node(path):
        hpath = host(path)
        if not os.path.islink(hpath) and not os.access(hpath, os.W_OK):
            os.chmod(hpath, stat.S_IWUSR)
        if _isdir(path):
            os.rmdir(hpath)
        else:
            os.unlink(hpath)

    def _remove(path):
        dirs = []
        for name in listdir(path):
            path_ = join(path, name)
            hname = host(path_)
            if _isdir(path_):
                dirs += [name]
            else:
                _remove_node(path_)
        for name in dirs:
            dir = join(path, name)
            _remove(dir)
            _remove_node(dir)

    path = shell(path)
    hpath = host(path)

    if os.path.exists(hpath):
        _remove(path)
        _remove_node(path)

def expand(name, paths):
    l = []
    for p in paths:
        l += [join(shell(p), name)]
    return l

def expanduser(path):
    path = host(path)
    path = os.path.expanduser(path)
    return shell(path)

def collect_files(path_):
    #
    # Convert to shell paths and return shell paths.
    #
    # @fixme should this use a passed in set of defaults and not
    #        not the initial set of values ?
    #
    path_ = shell(path_)
    if '*' in path_ or '?' in path_:
        dir = dirname(path_)
        base = basename(path_)
        if len(base) == 0:
            base = '*'
        files = []
        for p in dir.split(':'):
            hostdir = host(p)
            for f in glob.glob(os.path.join(hostdir, base)):
                files += [host(f)]
    else:
        files = [host(path_)]
    return sorted(files)

def copy(src, dst):
    src = shell(src)
    dst = shell(dst)
    hsrc = host(src)
    hdst = host(dst)
    try:
        shutil.copy(hsrc, hdst)
    except OSError as why:
        if windows:
            if WindowsError is not None and isinstance(why, WindowsError):
                pass
        else:
            raise error.general('copying tree (1): %s -> %s: %s' % (hsrc, hdst, str(why)))

def copy_tree(src, dst):
    trace = False

    hsrc = host(src)
    hdst = host(dst)

    if exists(hsrc):
        if isdir(hsrc):
            names = listdir(hsrc)
        else:
            names = [basename(hsrc)]
            hsrc = dirname(hsrc)
    else:
        names = []

    if trace:
        print('path.copy_tree:')
        print('   src: "%s"' % (src))
        print('  hsrc: "%s"' % (hsrc))
        print('   dst: "%s"' % (dst))
        print('  hdst: "%s"' % (hdst))
        print(' names: %r' % (names))

    if not os.path.isdir(hdst):
        if trace:
            print(' mkdir: %s' % (hdst))
        try:
            os.makedirs(hdst)
        except OSError as why:
            raise error.general('copying tree: cannot create target directory %s: %s' % \
                                (hdst, str(why)))

    for name in names:
        srcname = host(os.path.join(hsrc, name))
        dstname = host(os.path.join(hdst, name))
        try:
            if os.path.islink(srcname):
                linkto = os.readlink(srcname)
                if exists(shell(dstname)):
                    if os.path.islink(dstname):
                        dstlinkto = os.readlink(dstname)
                        if linkto != dstlinkto:
                            log.warning('copying tree: link does not match: %s -> %s' % \
                                            (dstname, dstlinkto))
                            os.remove(dstname)
                    else:
                        log.warning('copying tree: destination is not a link: %s' % \
                                        (dstname))
                        os.remove(dstname)
                else:
                    os.symlink(linkto, dstname)
            elif os.path.isdir(srcname):
                copy_tree(srcname, dstname)
            else:
                shutil.copyfile(host(srcname), host(dstname))
                shutil.copystat(host(srcname), host(dstname))
        except shutil.Error as err:
            raise error.general('copying tree (2): %s -> %s: %s' % \
                                (hsrc, hdst, str(err)))
        except EnvironmentError as why:
            raise error.general('copying tree (3): %s -> %s: %s' % \
                                (srcname, dstname, str(why)))
    try:
        shutil.copystat(hsrc, hdst)
    except OSError as why:
        if windows:
            if WindowsError is not None and isinstance(why, WindowsError):
                pass
        else:
            raise error.general('copying tree (4): %s -> %s: %s' % (hsrc, hdst, str(why)))

def get_size(path, depth = -1):
    #
    # Get the size the directory tree manually to the required depth.
    # This makes sure on Windows the files are correctly encoded to avoid
    # the file name size limit. On Windows the os.walk fails once we
    # get to the max path length on Windows.
    #
    def _isdir(path):
        hpath = host(path)
        return os.path.isdir(hpath) and not os.path.islink(hpath)

    def _node_size(path):
        hpath = host(path)
        size = 0
        if not os.path.islink(hpath):
            size = os.path.getsize(hpath)
        return size

    def _get_size(path, depth, level = 0):
        level += 1
        dirs = []
        size = 0
        for name in listdir(path):
            path_ = join(path, shell(name))
            hname = host(path_)
            if _isdir(path_):
                dirs += [shell(name)]
            else:
                size += _node_size(path_)
        if depth < 0 or level < depth:
            for name in dirs:
                dir = join(path, name)
                size += _get_size(dir, depth, level)
        return size

    path = shell(path)
    hpath = host(path)
    size = 0

    if os.path.exists(hpath):
        size = _get_size(path, depth)

    return size

def get_humanize_size(path, depth = -1):
    size = get_size(path, depth)
    for unit in ['','K','M','G','T','P','E','Z']:
        if abs(size) < 1024.0:
            return "%5.3f%sB" % (size, unit)
        size /= 1024.0
    return "%.3f%sB" % (size, 'Y')

if __name__ == '__main__':
    print(host('/a/b/c/d-e-f'))
    print(host('//a/b//c/d-e-f'))
    print(shell('/w/x/y/z'))
    print(basename('/as/sd/df/fg/me.txt'))
    print(dirname('/as/sd/df/fg/me.txt'))
    print(join('/d', 'g', '/tyty/fgfg'))
    windows = True
    print(host('/a/b/c/d-e-f'))
    print(host('//a/b//c/d-e-f'))
    print(shell('/w/x/y/z'))
    print(shell('w:/x/y/z'))
    print(basename('x:/sd/df/fg/me.txt'))
    print(dirname('x:/sd/df/fg/me.txt'))
    print(join('s:/d/', '/g', '/tyty/fgfg'))
    print(join('s:/d/e\\f/g', '/h', '/tyty/zxzx', '\\mm\\nn/p'))
