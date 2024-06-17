# SPDX-License-Identifier: BSD-2-Clause

# RTEMS Tools Project (http://www.rtems.org/)
# Copyright (C) 2020-2024 Amar Takhar <amar@rtems.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import sys
import re
import os
import pytest


from rtemstoolkit import execute


class TestClass:

    @classmethod
    def _capture_output(self, text):
        self.log += text

    @classmethod
    def setup_method(self, method):
        self.log = ""

    @classmethod
    def teardown_method(self, method):
        assert not self.log.startswith("TEST-ERROR")

    @classmethod
    def setup_class(cls):
        cls.e = execute.execute(error_prefix='TEST-ERROR',
                                output=cls._capture_output,
                                verbose=False)
        cls.log = ""

    @pytest.mark.unix
    def test_unix_shell_pwd(self):
        self.e.shell("pwd")
        assert self.log == os.getcwd() + "\n"

    @pytest.mark.unix
    def test_unix_shell_ls(self):
        assert self.e.shell("ls -las")

    @pytest.mark.unix
    def test_unix_shell(self):
        assert self.e.shell(
            'x="me"; if [ $x = "me" ]; then echo "It was me"; else "It was him"; fi'
        )
        assert self.log == "It was me\n"

#    def test_unix_shell_nonexistent(self):
#        assert self.e.shell(".\nonexistent")

    @pytest.mark.unix
    def test_unix_spawn_ls(self):
        assert self.e.spawn("ls")

    @pytest.mark.unix
    def test_unix_spawn_list(self):
        assert self.e.spawn(["ls", "-l"])

    @pytest.mark.unix
    def test_unix_cmd_date(self):
        assert self.e.command("date")

    @pytest.mark.unix
    def test_unix_cmd_date_r(self):
        assert self.e.command("date", "-R")

    @pytest.mark.unix
    def test_unix_cmd_date_u(self):
        assert self.e.command("date", ["-u", "+%d %D"])

    @pytest.mark.unix
    def test_unix_cmd_date_u(self):
        assert self.e.command("date", '-u "+%d %D %S"')

    @pytest.mark.unix
    def test_unix_cmd_subst_date_1(self):
        assert self.e.command_subst('date %0 "+%d %D %S"', ['-u'])

    @pytest.mark.unix
    def test_unix_cmd_subst_date_2(self):
        assert self.e.command_subst('date %0 %1', ['-u', '+%d %D %S'])

    def test_pipe(self):
        import subprocess
        ec, proc = self.e.command("grep",
                                  "testing",
                                  capture=False,
                                  stdin=subprocess.PIPE)
        out = "123testing123".encode()

        proc.stdin.write(out)
        proc.stdin.close()
        self.e.capture(proc)

        assert self.log == "123testing123\n"

    @pytest.mark.win32
    def test_win_shell(self):
        assert self.e.shell("cd")

    @pytest.mark.win32
    def test_win_shell(self):
        assert self.e.shell("dir /w")

    @pytest.mark.win32
    def test_win_shell(self):
        assert self.e.shell(
            'if "%OS%" == "Windows_NT" (echo It is WinNT) else echo Is is not WinNT'
        )
        assert self.log == "It is WinNT\r\n"

    @pytest.mark.win32
    def test_win_spawn_hostname(self):
        assert self.e.spawn("hostname")

    @pytest.mark.win32
    def test_win_spawn_netstat(self):
        assert self.e.spawn(["netstat", "/e"])

    @pytest.mark.win32
    def test_win_cmd_ipconfig(self):
        assert self.e.command("ipconfig")

    @pytest.mark.win32
    def test_win_cmd_ping(self):
        assert self.e.command("ping", "-s", "1")


#    @pytest.mark.skip("too slow!")

    @pytest.mark.win32
    def test_win_csubsts_1(self):
        assert self.e.command_subst('netstat %0', ['-a'])

    @pytest.mark.win32
    def test_win_csubsts_2(self):
        assert self.e.command_subst('netstat %0 %1', ['-a', '-n'])


def test_arg_list_1():
    assert execute.arg_list('cmd a1 a2 "a3 is a string" a4') == [
        'cmd', 'a1', 'a2', 'a3 is a string', 'a4'
    ]


def test_arg_list_2():
    assert execute.arg_list('cmd b1 b2 "b3 is a string a4') == \
                            ['cmd', 'b1', 'b2', '"b3', 'is', 'a', 'string',
                            'a4']


def test_arg_subst():
    assert execute.arg_subst(['nothing', 'xx-%0-yyy', '%1', '%2-something'],
                             ['subst0', 'subst1', 'subst2']) \
                              == ['nothing', 'xx-subst0-yyy', 'subst1', 'subst2-something']
