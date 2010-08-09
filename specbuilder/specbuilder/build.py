#
# $Id$
#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2010 Chris Johns (chrisj@rtems.org)
# All rights reserved.
#
# This file is part of the RTEMS Tools package in 'rtems-tools'.
#
# RTEMS Tools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RTEMS Tools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RTEMS Tools.  If not, see <http://www.gnu.org/licenses/>.
#

#
# This code builds a package given a spec file. It only builds to be installed
# not to be package unless you run a packager around this.
#

import getopt
import os
import shutil
import stat
import sys
import urllib2
import urlparse

import defaults
import error
import execute
import log
import spec

#
# Version of Spec Builder.
#
version = '0.1'

def _notice(opts, text):
    if not opts.quiet() and not log.default.has_stdout():
        print text
    log.output(text)
    log.flush()

class script:
    """Create and manage a shell script."""

    def __init__(self, quiet = True):
        self.quiet = quiet
        self.reset()

    def reset(self):
        self.body = []
        self.lc = 0

    def append(self, text):
        if type(text) is str:
            text = text.splitlines()
        if not self.quiet:
            i = 0
            for l in text:
                i += 1
                log.output('script:%3d: ' % (self.lc + i) + l)
        self.lc += len(text)
        self.body.extend(text)

    def write(self, name, check_for_errors = False):
        s = None
        try:
            s = open(name, 'w')
            s.write('\n'.join(self.body))
            s.close()
            os.chmod(name, stat.S_IRWXU | \
                         stat.S_IRGRP | stat.S_IXGRP | \
                         stat.S_IROTH | stat.S_IXOTH)
        except IOError, err:
            raise error.general('creating script: ' + name)
        finally:
            if s is not None:
                s.close()

class build:
    """Build a package given a spec file."""

    def __init__(self, name, _defaults, opts):
        self.opts = opts
        _notice(opts, 'building: ' + name)
        self.spec = spec.file(name, _defaults = _defaults, opts = opts)
        self.script = script(quiet = opts.quiet())

    def _output(self, text):
        if not self.opts.quiet():
            log.output(text)

    def rmdir(self, path):
        if not self.opts.dry_run():
            self._output('removing: ' + path)
            if os.path.exists(path):
                try:
                    shutil.rmtree(path)
                except IOError, err:
                    raise error.error('error removing: ' + path)

    def mkdir(self, path):
        if not self.opts.dry_run():
            self._output('making dir: ' + path)
            try:
                os.makedirs(path)
            except IOError, err:
                raise error.general('error creating path: ' + path)

    def get_file(self, url, local):
        if not os.path.isdir(os.path.dirname(local)):
            raise error.general('source path not found: ' + os.path.dirname(local))
        if not os.path.exists(local):
            #
            # Not localy found so we need to download it. Check if a URL
            # has been provided on the command line.
            #
            url_bases = self.opts.urls()
            urls = []
            if url_bases is not None:
                for base in url_bases:
                    if base[-1:] != '/':
                        base += '/'
                    url_path = urlparse.urlsplit(url).path
                    slash = url_path.rfind('/')
                    if slash < 0:
                        url_file = url_path
                    else:
                        url_file = url_path[slash + 1:]
                    urls.append(urlparse.urljoin(base, url_file))
            urls.append(url)
            for url in urls:
                _notice(self.opts, 'download: ' + url + ' -> ' + local)
                if not self.opts.dry_run():
                    failed = False
                    _in = None
                    _out = None
                    try:
                        _in = urllib2.urlopen(url)
                        _out = open(local, 'wb')
                        _out.write(_in.read())
                    except IOError, err:
                        _notice(self.opts, 'download: ' + url + ': failed: ' + str(err))
                        if os.path.exists(local):
                            os.remove(local)
                        failed = True
                    finally:
                        if _out is not None:
                            _out.close()
                    if _in is not None:
                        del _in
                    if not failed:
                        if not os.path.isfile(local):
                            raise error.general('source is not a file: ' + local)
                        return
            raise error.general('downloading ' + url + ': all paths have failed, giving up')

    def parse_url(self, url):
        #
        # Split the source up into the parts we need.
        #
        source = {}
        source['url'] = url
        source['path'] = os.path.dirname(url)
        source['file'] = os.path.basename(url)
        source['name'], source['ext'] = os.path.splitext(source['file'])
        #
        # Get the file. Checks the local source directory first.
        #
        source['local'] = os.path.join(self.spec.abspath('_sourcedir'), 
                                       source['file'])
        #
        # Is the file compressed ?
        #
        esl = source['ext'].split('.')
        if esl[-1:][0] == 'gz':
            source['compressed'] = '%{__gzip} -dc'
        elif esl[-1:][0] == 'bz2':
            source['compressed'] = '%{__bzip2} -dc'
        elif esl[-1:][0] == 'bz2':
            source['compressed'] = '%{__zip} -u'
        source['script'] = ''
        return source

    def source(self, package, source_tag):
        #
        # Scan the sources found in the spec file for the one we are
        # after. Infos or tags are lists.
        #
        sources = package.sources()
        url = None
        for s in sources:
            tag = s[len('source'):]
            if tag.isdigit():
                if int(tag) == source_tag:
                    url = sources[s][0]
                    break
        if url is None:
            raise error.general('source tag not found: source' + str(source_tag))
        source = self.parse_url(url)
        self.get_file(source['url'], source['local'])
        if 'compressed' in source:
            source['script'] = source['compressed'] + ' ' + \
                source['local'] + ' | %{__tar_extract} -'
        else:
            source['script'] = '%{__tar_extract} ' + source['local']
        return source

    def patch(self, package, args):
        #
        # Scan the patches found in the spec file for the one we are
        # after. Infos or tags are lists.
        #
        patches = package.patches()
        url = None
        for p in patches:
            if args[0][1:].lower() == p:
                url = patches[p][0]
                break
        if url is None:
            raise error.general('patch tag not found: ' + args[0])
        patch = self.parse_url(url)
        self.get_file(patch['url'], patch['local'])
        if 'compressed' in patch:
            patch['script'] = patch['compressed'] + ' ' +  patch['local']
        else:
            patch['script'] = '%{__cat} ' + patch['local']
        patch['script'] += ' | %{__patch} ' + ' '.join(args[1:])
        self.script.append(self.spec.expand(patch['script']))

    def setup(self, package, args):
        self._output('prep: ' + package.name() + ': ' + ' '.join(args))
        opts, args = getopt.getopt(args[1:], 'qDcTn:b:a:')
        source_tag = 0
        quiet = False
        unpack_default_source = True
        delete_before_unpack = True
        create_dir = False
        name = None
        unpack_before_chdir = True
        for o in opts:
            if o[0] == '-q':
                quiet = True
            elif o[0] == '-D':
                delete_before_unpack = False
            elif o[0] == '-c':
                create_dir = True
            elif o[0] == '-T':
                unpack_default_source = False
            elif o[0] == '-n':
                name = o[1]
            elif o[0] == '-b':
                unpack_before_chdir = True
                if not o[1].isdigit():
                    raise error.general('setup source tag no a number: ' + o[1])
                source_tag = int(o[1])
            elif o[0] == '-a':
                unpack_before_chdir = False
                source_tag = int(o[1])
        source0 = None
        source = self.source(package, source_tag)
        if name is None:
            if source:
                name = source['name']
            else:
                name = source0['name']
        self.script.append(self.spec.expand('cd %{_builddir}'))
        if delete_before_unpack:
            self.script.append(self.spec.expand('%{__rm} -rf ' + name))
        if create_dir:
            self.script.append(self.spec.expand('%{__mkdir_p} ' + name))
        #
        # If -a? then change directory before unpacking.
        #
        if not unpack_before_chdir:
            self.script.append(self.spec.expand('cd ' + name))
        #
        # Unpacking the source. Note, treated the same as -a0.
        #
        if unpack_default_source and source_tag != 0:
            source0 = self.source(package, 0)
            if source0 is None:
                raise error.general('no setup source0 tag found')
            self.script.append(self.spec.expand(source0['script']))
        self.script.append(self.spec.expand(source['script']))
        if unpack_before_chdir:
            self.script.append(self.spec.expand('cd ' + name))
        self.script.append(self.spec.expand('%{__setup_post}'))
        if create_dir:
            self.script.append(self.spec.expand('cd ..'))

    def run(self, command, shell_opts = '', cwd = None):
        e = execute.capture_execution(log = log.default, dump = self.opts.quiet())
        cmd = self.spec.expand('%{___build_shell} -ex ' + shell_opts + ' ' + command)
        self._output('run: ' + cmd)
        exit_code, proc, output = e.shell(cmd, cwd = cwd)
        if exit_code != 0:
            raise error.general('shell cmd failed: ' + cmd)

    def builddir(self):
        builddir = self.spec.abspath('_builddir')
        self.rmdir(builddir)
        if not self.opts.dry_run():
            self.mkdir(builddir)

    def prep(self, package):
        self.script.append('echo "==> %prep:"')
        _prep = package.prep()
        for l in _prep:
            args = l.split()
            if args[0] == '%setup':
                self.setup(package, args)
            elif args[0].startswith('%patch'):
                self.patch(package, args)
            else:
                self.script.append(' '.join(args))

    def build(self, package):
        self.script.append('echo "==> %build:"')
        _build = package.build()
        for l in _build:
            args = l.split()
            self.script.append(' '.join(args))

    def install(self, package):
        self.script.append('echo "==> %install:"')
        _install = package.install()
        for l in _install:
            args = l.split()
            self.script.append(' '.join(args))

    def files(self, package):
        self.script.append('echo "==> %files:"')
        prefixbase = self.opts.prefixbase()
        if prefixbase is None:
            prefixbase = ''
        inpath = os.path.join('%{buildroot}', prefixbase)
        self.script.append(self.spec.expand('cd ' + inpath))
        tar = os.path.join('%{_rpmdir}', package.long_name() + '.tar.bz2')
        cmd = self.spec.expand('%{__tar} -cf - . ' + '| %{__bzip2} > ' + tar)
        self.script.append(cmd)
        self.script.append(self.spec.expand('cd %{_builddir}'))

    def clean(self, package):
        self.script.append('echo "==> %clean:"')
        _clean = package.clean()
        for l in _clean:
            args = l.split()
            self.script.append(' '.join(args))
        
    def cleanup(self):
        buildroot = self.spec.abspath('buildroot')
        builddir = self.spec.abspath('_builddir')
        self.rmdir(buildroot)
        self.rmdir(builddir)

    def make(self):
        packages = self.spec.packages()
        package = packages['main']
        _notice(self.opts, 'package: ' + package.name() + '-' + package.version())
        self.script.reset()
        self.script.append(self.spec.expand('%{___build_template}'))
        self.script.append('echo "=> ' + package.name() + '-' + package.version() + ':"')
        self.prep(package)
        self.build(package)
        self.install(package)
        self.files(package)
        if not self.opts.no_clean():
            self.clean(package)
        if not self.opts.dry_run():
            self.builddir()
            sn = self.spec.expand(os.path.join('%{_builddir}', 'doit'))
            self._output('write script: ' + sn)
            self.script.write(sn)
            self.run(sn)
        if not self.opts.no_clean():
            self.cleanup()

def run(args):
    try:
        opts, _defaults = defaults.load(args)
        log.default = log.log(opts.logfiles())
        _notice(opts, 'RTEMS Tools, Spec Builder, v%s' % (version))
        for spec_file in opts.spec_files():
            b = build(spec_file, _defaults = _defaults, opts = opts)
            b.make()
            del b
    except error.general, gerr:
        print gerr
        sys.exit(1)
    except error.internal, ierr:
        print ierr
        sys.exit(1)
    except KeyboardInterrupt:
        _notice(opts, 'user terminated')
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    run(sys.argv)
