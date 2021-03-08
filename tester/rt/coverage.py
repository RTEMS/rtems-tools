#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2014 Krzysztof Miesowicz (krzysztof.miesowicz@gmail.com)
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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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

from __future__ import print_function

import datetime
import shutil
import os
import sys

try:
    import configparser
except:
    import ConfigParser as configparser

from rtemstoolkit import error
from rtemstoolkit import path
from rtemstoolkit import log
from rtemstoolkit import execute
from rtemstoolkit import macros
from rtemstoolkit import version


class summary:
    def __init__(self, p_summary_dir):
        self.summary_file_path = path.join(p_summary_dir, 'summary.txt')
        self.index_file_path = path.join(p_summary_dir, 'index.html')
        self.bytes_analyzed = 0
        self.bytes_not_executed = 0
        self.percentage_executed = 0.0
        self.percentage_not_executed = 100.0
        self.unreferenced_symbols = 0
        self.ranges_uncovered = 0
        self.branches_uncovered = 0
        self.branches_total = 0
        self.branches_always_taken = 0
        self.branches_never_taken = 0
        self.percentage_branches_covered = 0.0
        self.is_failure = False

    def parse(self):
        if not path.exists(self.summary_file_path):
            log.output('coverage: summary file %s does not exist!' % (self.summary_file_path))
            self.is_failure = True

        with open(self.summary_file_path, 'r') as summary_file:
           self.bytes_analyzed = self._get_next_with_colon(summary_file)
           self.bytes_not_executed = self._get_next_with_colon(summary_file)
           self.percentage_executed = self._get_next_with_colon(summary_file)
           self.percentage_not_executed = self._get_next_with_colon(summary_file)
           self.unreferenced_symbols = self._get_next_with_colon(summary_file)
           self.ranges_uncovered = self._get_next_with_colon(summary_file)
           summary_file.readline()
           summary_file.readline()
           self.branches_total = self._get_next_with_colon(summary_file)
           self.branches_uncovered = self._get_next_with_colon(summary_file)
           self.branches_always_taken = self._get_next_without_colon(summary_file)
           self.branches_never_taken = self._get_next_without_colon(summary_file)
        if len(self.branches_uncovered) > 0 and len(self.branches_total) > 0:
            self.percentage_branches_covered = \
                1.0 - (float(self.branches_uncovered) / float(self.branches_total))
        else:
            self.percentage_branches_covered = 0.0
        return

    def _get_next_with_colon(self, summary_file):
        line = summary_file.readline()
        if ':' in line:
            return line.split(':')[1].strip()
        else:
            return ''

    def _get_next_without_colon(self, summary_file):
        line = summary_file.readline()
        return line.strip().split(' ')[0]

class report_gen_html:
    def __init__(self, symbol_sets, build_dir, rtdir, bsp):
        self.symbol_sets = symbol_sets
        self.build_dir = build_dir
        self.partial_reports_files = list(['index.html', 'summary.txt'])
        self.number_of_columns = 1
        self.covoar_src_path = path.join(rtdir, 'covoar')
        self.bsp = bsp

    def _find_partial_reports(self):
        partial_reports = {}
        for symbol_set in self.symbol_sets:
            set_summary = summary(path.join(self.bsp + "-coverage",
                                            symbol_set))
            set_summary.parse()
            partial_reports[symbol_set] = set_summary
        return partial_reports

    def _prepare_head_section(self):
        head_section = '<head>' + os.linesep
        head_section += ' <title>RTEMS Coverage Report</title>' + os.linesep
        head_section += ' <style type="text/css">' + os.linesep
        head_section += '  progress[value] {' + os.linesep
        head_section += '    -webkit-appearance: none;' + os.linesep
        head_section += '    appearance: none;' + os.linesep
        head_section += '    width: 150px;' + os.linesep
        head_section += '    height: 15px;' + os.linesep
        head_section += '  }' + os.linesep
        head_section += '  progress::-webkit-progress-bar {' + os.linesep
        head_section += '    background: red;' + os.linesep
        head_section += '  }' + os.linesep
        head_section += '  table, th, td {' + os.linesep
        head_section += '    border: 1px solid black;' + os.linesep
        head_section += '    border-collapse: collapse;' + os.linesep
        head_section += '  }' + os.linesep
        head_section += '  td {' + os.linesep
        head_section += '    text-align:center;' + os.linesep
        head_section += '  }' + os.linesep
        head_section += ' </style>' + os.linesep
        head_section += ' <link rel="stylesheet" href="' + self.bsp + '-coverage/covoar.css">' + os.linesep
        head_section += ' <script type="text/javascript" src="' + self.bsp + '-coverage/table.js"></script>' + os.linesep
        head_section += '</head>' + os.linesep
        return head_section

    def _prepare_index_content(self, partial_reports):
        header = "<h1> RTEMS Coverage Analysis Report </h1>" + os.linesep
        header += "<h3>Coverage reports by symbols sets:</h3>" + os.linesep
        table = "<table class=\"covoar table-autosort:0 table-autofilter table-stripeclass:covoar-tr-odd\">" + os.linesep
        table += "<thead>" + os.linesep
        table += self._header_row()
        table += "</thead>" + os.linesep
        table += "<tbody>" + os.linesep
        for symbol_set in sorted(partial_reports.keys()):
            table += self._row(symbol_set, partial_reports[symbol_set])
        table += "</tbody>" + os.linesep
        table += "</table> </br>"
        timestamp = "Analysis performed on " + datetime.datetime.now().ctime()
        return "<body>\n" + header + table + timestamp + "\n</body>"

    def _row(self, symbol_set, summary):
        row = "<tr>" + os.linesep
        row += "<td>" + symbol_set + "</td>" + os.linesep
        if summary.is_failure:
            row += ' <td colspan="' + str(self.number_of_columns-1) \
                        + '" style="background-color:red">FAILURE</td>' + os.linesep
        else:
            row += ' <td>' + self._link(summary.index_file_path, 'Index') \
                   + '</td>' + os.linesep
            row += ' <td>' + self._link(summary.summary_file_path, 'Summary') \
                   + '</td>' + os.linesep
            row += ' <td>' + summary.bytes_analyzed + '</td>' + os.linesep
            row += ' <td>' + summary.bytes_not_executed + '</td>' + os.linesep
            row += ' <td>' + summary.unreferenced_symbols + '</td>' + os.linesep
            row += ' <td>' + summary.ranges_uncovered + '</td>' + os.linesep
            row += ' <td>' + summary.percentage_executed + '%</td>' + os.linesep
            row += ' <td>' + summary.percentage_not_executed + '%</td>' + os.linesep
            row += ' <td><progress value="' + summary.percentage_executed \
                    + '" max="100"></progress></td>' + os.linesep
            row += ' <td>' + summary.branches_uncovered + '</td>' + os.linesep
            row += ' <td>' + summary.branches_total + '</td>' + os.linesep
            row += ' <td> {:.3%} </td>'.format(summary.percentage_branches_covered)
            spbc = 100 * summary.percentage_branches_covered
            row += ' <td><progress value="{:.3}" max="100"></progress></td>'.format(spbc)
            row += '</tr>' + os.linesep
        return row

    def _header_row(self):
        row = "<tr>" + os.linesep
        rowAttributes = "class=\"table-sortable:default table-sortable\" title=\"Click to sort\""
        row += " <th " + rowAttributes + "> Symbols set name </th>" + os.linesep
        row += " <th " + rowAttributes + "> Index file </th>" + os.linesep
        row += " <th " + rowAttributes + "> Summary file </th>" + os.linesep
        row += " <th " + rowAttributes + "> Bytes analyzed </th>" + os.linesep
        row += " <th " + rowAttributes + "> Bytes not executed </th>" + os.linesep
        row += " <th " + rowAttributes + "> Unreferenced symbols </th>" + os.linesep
        row += " <th " + rowAttributes + "> Uncovered ranges </th>" + os.linesep
        row += " <th " + rowAttributes + "> Percentage covered </th>" + os.linesep
        row += " <th " + rowAttributes + "> Percentage uncovered </th>" + os.linesep
        row += " <th " + rowAttributes + "> Instruction coverage </th>" + os.linesep
        row += " <th " + rowAttributes + "> Branches uncovered </th>" + os.linesep
        row += " <th " + rowAttributes + "> Branches total </th>" + os.linesep
        row += " <th " + rowAttributes + "> Branches covered percentage </th>" + os.linesep
        row += " <th " + rowAttributes + "> Branch coverage </th>" + os.linesep
        row += "</tr>"
        self.number_of_columns = row.count('<th>')
        return row

    def _link(self, address, text):
        return '<a href="' + address + '">' + text + '</a>'

    def _create_index_file(self, head_section, content):
        name = path.join(self.build_dir, self.bsp + "-report.html")
        with open(name, 'w') as f:
            f.write(head_section)
            f.write(content)

    def generate(self):
        partial_reports = self._find_partial_reports()
        head_section = self._prepare_head_section()
        index_content = self._prepare_index_content(partial_reports)
        self._create_index_file(head_section,index_content)

    def add_covoar_css(self):
        table_js_path = path.join(self.covoar_src_path, 'table.js')
        covoar_css_path = path.join(self.covoar_src_path, 'covoar.css')
        coverage_directory = path.join(self.build_dir,
                                    self.bsp + '-coverage')
        path.copy_tree(covoar_css_path, coverage_directory)
        path.copy_tree(table_js_path, coverage_directory)

    def add_dir_name(self):
        for symbol_set in self.symbol_sets:
             symbol_set_dir = path.join(self.build_dir,
                                        self.bsp + '-coverage', symbol_set)
             html_files = path.listdir(symbol_set_dir)
             for html_file in html_files:
                 html_file = path.join(symbol_set_dir, html_file)
                 if path.exists(html_file) and 'html' in html_file:
                     with open(html_file, 'r') as f:
                         file_data = f.read()
                     text = file_data[file_data.find('<div class="heading-title">')\
                                     +len('<div class="heading-title">') \
                                     : file_data.find('</div')]
                     file_data = file_data.replace(text, text + '<br>' + symbol_set)
                     with open(html_file, 'w') as f:
                         f.write(file_data)

class build_path_generator(object):
    '''
    Generates the build path from the path to executables
    '''
    def __init__(self, executables, target):
        self.executables = executables
        self.target = target
    def run(self):
        build_path = '/'
        path_ = self.executables[0].split('/')
        for p in path_:
            if p == self.target:
                break
            else:
                build_path = path.join(build_path, p)
        return build_path

class symbol_parser(object):
    '''
    Parse the symbol sets ini and create custom ini file for covoar
    '''
    def __init__(self,
                 symbol_config_path,
                 symbol_select_path,
                 symbol_set,
                 build_dir,
                 bsp_name,
                 target):
        self.symbol_select_file = symbol_select_path
        self.symbol_file = symbol_config_path
        self.build_dir = build_dir
        self.symbol_sets = {}
        self.symbol_set = symbol_set
        self.ssets = []
        self.bsp_name = bsp_name
        self.target = target

    def parse(self):
        config = configparser.ConfigParser()
        try:
            config.read(self.symbol_file)
            if self.symbol_set is not None:
                self.ssets = self.symbol_set.split(',')
            else:
                self.ssets = config.get('symbol-sets', 'sets').split(',')
            for sset in self.ssets:
                lib = path.join(self.build_dir, config.get('libraries', sset))
                self.symbol_sets[sset] = lib
                ss = self.symbol_sets[sset]
                ss = ss.replace('@BSP@', self.bsp_name)
                ss = ss.replace('@BUILD-TARGET@', self.target)
                self.symbol_sets[sset] = ss
            return self.ssets
        except:
            raise error.general('Symbol set parsing failed for %s' % (sset))

    def write_ini(self, symbol_set):
        config = configparser.ConfigParser()
        try:
            sset = symbol_set
            config.add_section('symbol-sets')
            config.set('symbol-sets', 'sets', sset)
            config.add_section(sset)
            object_files = [o for o in path.listdir(self.symbol_sets[sset]) if o[-1] == 'o']
            object_paths = []
            for o in object_files:
                object_paths.append(path.join(self.symbol_sets[sset], o))
            config.set(sset, 'libraries', ','.join(object_paths))
            with open(self.symbol_select_file, 'w') as conf:
                    config.write(conf)
        except:
            raise error.general('symbol parser write failed for %s' % (sset))

class covoar(object):
    '''
    Covoar runner
    '''
    def __init__(self,
                 base_result_dir,
                 config_dir,
                 executables,
                 trace,
                 prefix,
                 covoar_cmd):
        self.base_result_dir = base_result_dir
        self.config_dir = config_dir
        self.executables = ' '.join(executables)
        self.project_name = 'RTEMS-' + str(version.version())
        self.trace = trace
        self.prefix = prefix
        self.covoar_cmd = covoar_cmd

    def _find_covoar(self):
        covoar_exe = 'covoar'
        tester_dir = path.dirname(path.abspath(sys.argv[0]))
        base = path.dirname(tester_dir)
        exe = path.join(self.prefix, 'share', 'rtems', 'tester', 'bin', covoar_exe)
        if path.isfile(exe):
            return exe
        exe = path.join(base, 'build', 'tester', 'covoar', covoar_exe)
        if path.isfile(exe):
            return exe
        raise error.general('coverage: %s not found'% (covoar_exe))

    def run(self, set_name, symbol_file):
        covoar_result_dir = path.join(self.base_result_dir, set_name)
        if not path.exists(covoar_result_dir):
            path.mkdir(covoar_result_dir)
        if not path.exists(symbol_file):
            raise error.general('coverage: no symbol set file: %s'% (symbol_file))
        exe = self._find_covoar()
        # The order of these arguments matters. Command line options must come
        # before the executable path arguments because covoar uses getopt() to
        # process the command line options.
        command = exe + ' -O ' + covoar_result_dir + \
                  ' -p ' + self.project_name + \
                  ' ' + self.covoar_cmd + ' '
        command += self.executables

        log.notice()
        log.notice('Running coverage analysis: %s (%s)' % (set_name, covoar_result_dir))
        start_time = datetime.datetime.now()
        executor = execute.execute(verbose = self.trace, output = self.output_handler)
        exit_code = executor.shell(command, cwd=os.getcwd())
        if exit_code[0] != 0:
            raise error.general('coverage: covoar failure:: %d' % (exit_code[0]))
        end_time = datetime.datetime.now()
        log.notice('Coverage time: %s' % (str(end_time - start_time)))

    def output_handler(self, text):
        log.output('%s' % (text))

class coverage_run(object):
    '''
    Coverage analysis support for rtems-test
    '''
    def __init__(self, macros_, executables, prefix, symbol_set = None, trace = False):
        '''
        Constructor
        '''
        self.trace = trace
        self.macros = macros_
        self.build_dir = self.macros['_cwd']
        self.test_dir = path.join(self.build_dir, self.macros['bsp'] + '-coverage')
        if not path.exists(self.test_dir):
            path.mkdir(self.test_dir)
        self.rtdir = path.abspath(self.macros['_rtdir'])
        self.rtscripts = self.macros.expand(self.macros['_rtscripts'])
        self.coverage_config_path = path.join(self.rtscripts, 'coverage')
        self.symbol_config_path = path.join(self.coverage_config_path,
                                            'symbol-sets.ini')
        self.symbol_select_path = self.macros.expand(self.macros['bsp_symbol_path'])
        self.executables = executables
        self.symbol_sets = []
        self.no_clean = int(self.macros['_no_clean'])
        self.report_format = self.macros['cov_report_format']
        self.symbol_set = symbol_set
        self.target = self.macros['arch']
        self.bsp_name = self.macros['bsp'].split('-')[0]
        self.prefix = prefix
        self.macros.define('coverage')
        self.covoar_cmd = self.macros.expand(self.macros['bsp_covoar_cmd'])
        self.covoar_cmd += ' -T ' + self.macros['arch'] + '-rtems' + str(version.version())

    def run(self):
        try:
            if self.executables is None:
                raise error.general('no test executables provided.')
            build_dir = build_path_generator(self.executables, self.target).run()
            parser = symbol_parser(self.symbol_config_path,
                                   self.symbol_select_path,
                                   self.symbol_set,
                                   build_dir,
                                   self.bsp_name,
                                   self.target)
            symbol_sets = parser.parse()
            for sset in symbol_sets:
                parser.write_ini(sset)
                covoar_runner = covoar(self.test_dir,
                                       self.symbol_select_path,
                                       self.executables,
                                       self.trace,
                                       self.prefix,
                                       self.covoar_cmd)
                covoar_runner.run(sset, self.symbol_select_path)
            self._generate_reports(symbol_sets);
            self._summarize();
        finally:
            self._cleanup();

    def _generate_reports(self, symbol_sets):
        log.notice('Coverage generating reports')
        if self.report_format == 'html':
            report = report_gen_html(symbol_sets,
                                     self.build_dir,
                                     self.rtdir,
                                     self.macros['bsp'])
            report.generate()
            report.add_covoar_css()
            report.add_dir_name()

    def _cleanup(self):
        if not self.no_clean:
            if self.trace:
                log.output('Coverage cleaning tempfiles')
            for exe in self.executables:
                trace_file = exe + '.cov'
                if path.exists(trace_file):
                    os.remove(trace_file)
            os.remove(self.symbol_select_path)

    def _summarize(self):
        log.notice('Coverage analysis finished: %s' % (self.build_dir))
