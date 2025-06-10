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
# Execute commands or scripts.
#
# Note, the subprocess module is only in Python 2.4 or higher.
#

from __future__ import print_function

import functools
import io
import os
import re
import shlex
import sys
import subprocess
import threading
import time
import traceback

from rtemstoolkit import error
from rtemstoolkit import log
from rtemstoolkit import stacktraces

# Trace exceptions
trace_threads = False

# Redefine the PIPE from subprocess
PIPE = subprocess.PIPE

# Regular expression to find quotes.
qstr = re.compile('[rR]?\'([^\\n\'\\\\]|\\\\.)*\'|[rR]?"([^\\n"\\\\]|\\\\.)*"')


def check_type(command):
    """Checks the type of command we have. The types are spawn and
    shell."""
    if command in ['spawn', 'shell']:
        return True
    return False


def arg_list(args):
    """Turn a string of arguments into a list suitable for
    spawning a command. If the args are already a list return
    it."""
    if type(args) is list:
        return args
    argstr = args
    args = []
    while len(argstr):
        qs = qstr.search(argstr)
        if not qs:
            args.extend(argstr.split())
            argstr = ''
        else:
            # We have a quoted string. Get the string before
            # the quoted string and splt on white space then
            # add the quoted string as an option then remove
            # the first + quoted string and try again
            front = argstr[:qs.start()]
            args.extend(front.split())
            args.append(argstr[qs.start() + 1:qs.end() - 1])
            argstr = argstr[qs.end():]
    return args


def arg_subst(command, substs):
    """Substitute the %[0-9] in the command with the subst values."""
    args = arg_list(command)
    if substs:
        for a in range(0, len(args)):
            for r in range(0, len(substs)):
                args[a] = re.compile(('%%%d' % (r))).sub(substs[r], args[a])
    return args


def arg_subst_str(command, subst):
    cmd = arg_subst(command, subst)

    def add(x, y):
        return x + ' ' + str(y)

    return functools.reduce(add, cmd, '')


class execute(object):
    """Execute commands or scripts. The 'output' is a funtion that handles the
    output from the process. The 'input' is a function that blocks and returns
    data to be written to stdin"""

    def __init__(self,
                 output=None,
                 input=None,
                 cleanup=None,
                 error_prefix='',
                 verbose=False):
        self.lock = threading.Lock()
        self.output = output
        self.input = input
        self.cleanup = cleanup
        self.error_prefix = error_prefix
        self.verbose = verbose
        self.shell_exe = None
        self.shell_commands = False
        self.path = None
        self.environment = None
        self.outputting = False
        self.timing_out = False
        self.proc = None

    @staticmethod
    def _shlex_join(elements):
        try:
            return shlex.join(elements)
        except AttributeError:
            # Python older than 3.8 does not have shlex.join
            return ' '.join(elements)

    def capture(self, proc, command='pipe', timeout=None):
        """Create 3 threads to read stdout and stderr and send to the output handler
        and call an input handler is provided. Based on the 'communicate' code
        in the subprocess module."""

        def _writethread(exe, fh, input):
            """Call the input handler and write it to the stdin. The input handler should
            block and return None or False if this thread is to exit and True if this
            is a timeout check."""
            if trace_threads:
                print('execute:_writethread: start')
            encoding = True
            try:
                tmp = bytes('temp', sys.stdin.encoding)
            except:
                encoding = False
            input_types = [str, bytes]
            try:
                # Unicode is not valid in python3, not added to the list
                input_types += [unicode]
            except:
                pass
            try:
                while True:
                    if trace_threads:
                        print('execute:_writethread: call input', input)
                    lines = input()
                    if trace_threads:
                        print('execute:_writethread: input returned:',
                              type(lines))
                    if type(lines) in input_types:
                        try:
                            if encoding:
                                lines = bytes(lines, sys.stdin.encoding)
                            fh.write(lines)
                            fh.flush()
                        except:
                            break
                    if lines == None or \
                       lines == False or \
                       (lines == True and fh.closed):
                        break
            except:
                if trace_threads:
                    print('execute:_writethread: exception')
                    print(traceback.format_exc())
                pass
            try:
                fh.close()
            except:
                pass
            if trace_threads:
                print('execute:_writethread: finished')

        def _readthread(exe, fh, out, prefix=''):
            """Read from a file handle and write to the output handler
            until the file closes."""

            def _output_line(line, exe, prefix, out, count):
                #exe.lock.acquire()
                #exe.outputting = True
                #exe.lock.release()
                try:
                    if out:
                        out(prefix + line)
                    else:
                        log.output(prefix + line)
                        if count > 10:
                            log.flush()
                except:
                    stacktraces.trace()

            if trace_threads:
                print('execute:_readthread: start')
            count = 0
            line = ''
            try:
                while True:
                    #
                    # The io module file handling return up to the size passed
                    # in to the read call. The io handle has the default
                    # buffering size. On any error assume the handle has gone
                    # and the process is shutting down.
                    #
                    try:
                        data = fh.read1(4096)
                    except:
                        data = ''
                    if len(data) == 0:
                        if len(line) > 0:
                            _output_line(line + '\n', exe, prefix, out, count)
                        break
                    # str and bytes are the same type in Python2
                    if type(data) is not str and type(data) is bytes:
                        data = data.decode(sys.stdout.encoding)
                    last_ch = data[-1]
                    sd = (line + data).split('\n')
                    if last_ch != '\n':
                        line = sd[-1]
                    else:
                        line = ''
                    sd = sd[:-1]
                    if len(sd) > 0:
                        for l in sd:
                            if trace_threads:
                                print('execute:_readthread: output-line:',
                                      count, type(l))
                            _output_line(l + '\n', exe, prefix, out, count)
                            count += 1
                        if count > 10:
                            count -= 10
            except:
                raise
                if trace_threads:
                    print('execute:_readthread: exception')
                    print(traceback.format_exc())
                pass
            try:
                fh.close()
            except:
                pass
            if len(line):
                _output_line(line, exe, prefix, out, 100)
            if trace_threads:
                print('execute:_readthread: finished')

        def _timerthread(exe, interval, function):
            """Timer thread is used to timeout a process if no output is
            produced for the timeout interval."""
            count = interval
            while exe.timing_out:
                time.sleep(1)
                if count > 0:
                    count -= 1
                exe.lock.acquire()
                if exe.outputting:
                    count = interval
                    exe.outputting = False
                exe.lock.release()
                if count == 0:
                    try:
                        proc.kill()
                    except:
                        pass
                    else:
                        function()
                    break

        name = os.path.basename(command[0])

        stdin_thread = None
        stdout_thread = None
        stderr_thread = None
        timeout_thread = None

        if proc.stdout:
            stdout_thread = threading.Thread(
                target=_readthread,
                name='_stdout[%s]' % (name),
                args=(self,
                      io.open(proc.stdout.fileno(), mode='rb',
                              closefd=False), self.output, ''))
            stdout_thread.daemon = True
            stdout_thread.start()
        if proc.stderr:
            stderr_thread = threading.Thread(
                target=_readthread,
                name='_stderr[%s]' % (name),
                args=(self,
                      io.open(proc.stderr.fileno(), mode='rb',
                              closefd=False), self.output, self.error_prefix))
            stderr_thread.daemon = True
            stderr_thread.start()
        if self.input and proc.stdin:
            stdin_thread = threading.Thread(target=_writethread,
                                            name='_stdin[%s]' % (name),
                                            args=(self, proc.stdin,
                                                  self.input))
            stdin_thread.daemon = True
            stdin_thread.start()
        if timeout:
            self.timing_out = True
            timeout_thread = threading.Thread(target=_timerthread,
                                              name='_timeout[%s]' % (name),
                                              args=(self, timeout[0],
                                                    timeout[1]))
            timeout_thread.daemon = True
            timeout_thread.start()
        try:
            self.lock.acquire()
            try:
                self.proc = proc
            except:
                raise
            finally:
                self.lock.release()
            exitcode = proc.wait()
        except:
            proc.kill()
            raise
        finally:
            self.lock.acquire()
            try:
                self.proc = None
            except:
                raise
            finally:
                self.lock.release()
            if self.cleanup:
                self.cleanup(proc)
            if timeout_thread:
                self.timing_out = False
                timeout_thread.join(10)
            if stdin_thread:
                stdin_thread.join(2)
            if stdout_thread:
                stdout_thread.join(2)
            if stderr_thread:
                stderr_thread.join(2)
        return exitcode

    def open(self,
             command,
             capture=True,
             shell=False,
             cwd=None,
             env=None,
             stdin=None,
             stdout=None,
             stderr=None,
             timeout=None):
        """Open a command with arguments. Provide the arguments as a list or
        a string."""
        if self.output is None:
            raise error.general('capture needs an output handler')
        if os.name == 'nt':
            shell = True
            command = 'sh -c "' + execute._shlex_join(command) + '"'
        # If a string split and not a shell command split
        if not shell and isinstance(command, str):
            command = shlex.split(command)
        if shell and isinstance(command, list):
            if os.name != 'nt':
                command = execute._shlex_join(command)
            if self.shell_exe:
                command = self.shell_exe + ' ' + command
        if isinstance(command, list):
            cs = execute._shlex_join(command)
        else:
            cs = command
        what = 'spawn'
        if shell:
            what = 'shell'
        cs = what + ': ' + cs
        if self.verbose:
            log.output(what + ': ' + cs)
        log.trace('exe: %s' % (cs))
        if not stdin and self.input:
            stdin = subprocess.PIPE
        if not stdout:
            stdout = subprocess.PIPE
        if not stderr:
            stderr = subprocess.PIPE
        proc = None
        if cwd is None:
            cwd = self.path
        if env is None:
            env = self.environment
        try:
            # Work around a problem on Windows with commands that
            # have a '.' and no extension. Windows needs the full
            # command name.
            if sys.platform == "win32" and type(command) is list:
                if command[0].find('.') >= 0:
                    r, e = os.path.splitext(command[0])
                    if e not in ['.exe', '.com', '.bat']:
                        command[0] = command[0] + '.exe'
            pipe_commands = []
            if shell:
                pipe_commands.append(command)
            else:
                # See if there is a pipe operator in the command. If present
                # split the commands by the pipe and run them with the pipe.
                # if no pipe it is the normal stdin and stdout
                current_command = []
                for cmd in command:
                    if cmd == '|':
                        pipe_commands.append(current_command)
                        current_command = []
                    else:
                        current_command.append(cmd)
                pipe_commands.append(current_command)
            proc = None
            if len(pipe_commands) == 1:
                cmd = pipe_commands[0]
                proc = subprocess.Popen(cmd,
                                        shell=shell,
                                        cwd=cwd,
                                        env=env,
                                        stdin=stdin,
                                        stdout=stdout,
                                        stderr=stderr,
                                        close_fds=False,
                                        text=False)
            else:
                for i, cmd in enumerate(pipe_commands):
                    if i == 0:
                        proc = subprocess.Popen(cmd,
                                                shell=shell,
                                                cwd=cwd,
                                                env=env,
                                                stdin=stdin,
                                                stdout=subprocess.PIPE,
                                                text=False)
                    elif i == len(pipe_commands) - 1:
                        proc = subprocess.Popen(cmd,
                                                shell=shell,
                                                cwd=cwd,
                                                env=env,
                                                stdin=proc.stdout,
                                                stdout=stdout,
                                                stderr=stderr,
                                                close_fds=False,
                                                text=False)
                    else:
                        proc = subprocess.Popen(cmd,
                                                shell=shell,
                                                cwd=cwd,
                                                env=env,
                                                stdin=proc.stdout,
                                                stdout=subprocess.PIPE,
                                                text=False)
            if not capture:
                return (0, proc)
            if self.output is None:
                raise error.general('capture needs an output handler')
            exit_code = self.capture(proc, command, timeout)
            if self.verbose:
                log.output('exit: ' + str(exit_code))
        except OSError as ose:
            exit_code = ose.errno
            if self.verbose:
                log.output('exit: ' + str(ose))
        return (exit_code, proc)

    def spawn(self,
              command,
              capture=True,
              cwd=None,
              env=None,
              stdin=None,
              stdout=None,
              stderr=None,
              timeout=None):
        """Spawn a command with arguments. Provide the arguments as a list or
        a string."""
        return self.open(command, capture, False, cwd, env, stdin, stdout,
                         stderr, timeout)

    def shell(self,
              command,
              capture=True,
              cwd=None,
              env=None,
              stdin=None,
              stdout=None,
              stderr=None,
              timeout=None):
        """Execute a command within a shell context. The command can contain
        argumments. The shell is specific to the operating system. For example
        it is cmd.exe on Windows XP."""
        return self.open(command, capture, True, cwd, env, stdin, stdout,
                         stderr, timeout)

    def command(self,
                command,
                args=None,
                capture=True,
                shell=False,
                cwd=None,
                env=None,
                stdin=None,
                stdout=None,
                stderr=None,
                timeout=None):
        """Run the command with the args. The args can be a list
        or a string."""
        if args and not type(args) is list:
            args = arg_list(args)
        cmd = [command]
        if args:
            cmd.extend(args)
        return self.open(cmd,
                         capture=capture,
                         shell=shell,
                         cwd=cwd,
                         env=env,
                         stdin=stdin,
                         stdout=stdout,
                         stderr=stderr,
                         timeout=timeout)

    def command_subst(self,
                      command,
                      substs,
                      capture=True,
                      shell=False,
                      cwd=None,
                      env=None,
                      stdin=None,
                      stdout=None,
                      stderr=None,
                      timeout=None):
        """Run the command from the config data with the
        option format string subsituted with the subst variables."""
        args = arg_subst(command, substs)
        return self.command(args[0],
                            args[1:],
                            capture=capture,
                            shell=shell or self.shell_commands,
                            cwd=cwd,
                            env=env,
                            stdin=stdin,
                            stdout=stdout,
                            stderr=stderr,
                            timeout=timeout)

    def set_shell(self, execute):
        """Set the shell to execute when issuing a shell command."""
        args = arg_list(execute)
        if len(args) == 0 or not os.path.isfile(args[0]):
            raise error.general('could find shell: ' + execute)
        self.shell_exe = args

    def command_use_shell(self):
        """Force all commands to use a shell. This can be used with set_shell
        to allow Unix commands be executed on Windows with a Unix shell such
        as Cygwin or MSYS. This may cause piping to fail."""
        self.shell_commands = True

    def set_output(self, output):
        """Set the output handler. The stdout of the last process in a pipe
        line is passed to this handler."""
        old_output = self.output
        self.output = output
        return old_output

    def set_path(self, path):
        """Set the path changed to before the child process is created."""
        old_path = self.path
        self.path = path
        return old_path

    def set_environ(self, environment):
        """Set the environment passed to the child process when created."""
        old_environment = self.environment
        self.environment = environment
        return old_environment

    def kill(self):
        self.lock.acquire()
        try:
            if self.proc is not None:
                self.proc.kill()
        except:
            raise
        finally:
            self.lock.release()

    def terminate(self):
        self.lock.acquire()
        try:
            if self.proc is not None:
                self.proc.terminate()
        except:
            raise
        finally:
            self.lock.release()

    def send_signal(self, signal):
        self.lock.acquire()
        try:
            if self.proc is not None:
                #print("sending sig")
                self.proc.send_signal(signal)
        except:
            raise
        finally:
            self.lock.release()


class capture_execution(execute):
    """Capture all output as a string and return it."""

    class _output_snapper:

        def __init__(self, log=None, dump=False):
            self.output = ''
            self.log = log
            self.dump = dump

        def handler(self, text):
            if not self.dump:
                if self.log is not None:
                    self.log.output(text)
                else:
                    self.output += text

        def get_and_clear(self):
            text = self.output
            self.output = ''
            return text.strip()

    def __init__(self, log=None, dump=False, error_prefix='', verbose=False):
        self.snapper = capture_execution._output_snapper(log=log, dump=dump)
        execute.__init__(self,
                         output=self.snapper.handler,
                         error_prefix=error_prefix,
                         verbose=verbose)

    def open(self,
             command,
             capture=True,
             shell=False,
             cwd=None,
             env=None,
             stdin=None,
             stdout=None,
             stderr=None,
             timeout=None):
        if not capture:
            raise error.general(
                'output capture must be true; leave as default')
        #self.snapper.get_and_clear()
        exit_code, proc = execute.open(self,
                                       command,
                                       capture=True,
                                       shell=shell,
                                       cwd=cwd,
                                       env=env,
                                       stdin=stdin,
                                       stdout=stdout,
                                       stderr=stderr,
                                       timeout=timeout)
        return (exit_code, proc, self.snapper.get_and_clear())

    def set_output(self, output):
        raise error.general('output capture cannot be overrided')
