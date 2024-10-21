#
# RTEMS Tools Project (http://www.rtems.org/)
# Copyright 2024 Matheus Pecoraro (matpecor@gmail.com)
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

from __future__ import print_function

import argparse
import difflib
import os
import sys

from datetime import datetime
from rtemstoolkit import error
from rtemstoolkit import git
from rtemstoolkit import path

#
# This table contains the ACPICA files imported into the RTEMS source tree
# and their respective directories
#
# A path to a directory implies all files inside of it (non recursive)
#
ACPICA_TO_RTEMS_TABLE = [
    ['source/include/', 'bsps/include/acpi/acpica'],
    [
        'source/include/platform/acenv.h',
        'bsps/include/acpi/acpica/platform/acenv.h'
    ],
    [
        'source/include/platform/acenvex.h',
        'bsps/include/acpi/acpica/platform/acenvex.h'
    ],
    [
        'source/include/platform/acgcc.h',
        'bsps/include/acpi/acpica/platform/acgcc.h'
    ],
    [
        'source/include/platform/acgccex.h',
        'bsps/include/acpi/acpica/platform/acgccex.h'
    ], ['source/components/dispatcher', 'bsps/shared/acpi/acpica/dispatcher'],
    ['source/components/events', 'bsps/shared/acpi/acpica/events'],
    ['source/components/executer', 'bsps/shared/acpi/acpica/executer'],
    ['source/components/hardware', 'bsps/shared/acpi/acpica/hardware'],
    ['source/components/namespace', 'bsps/shared/acpi/acpica/namespace'],
    ['source/components/parser', 'bsps/shared/acpi/acpica/parser'],
    ['source/components/resources', 'bsps/shared/acpi/acpica/resources'],
    ['source/components/tables', 'bsps/shared/acpi/acpica/tables'],
    ['source/components/utilities', 'bsps/shared/acpi/acpica/utilities']
]


def gen_rtems_style_tree(acpica_base_path, commit_hash):
    clone_repo_path = path.join(os.getcwd(), 'acpica-' + commit_hash)
    clone_repo = git.repo(clone_repo_path)
    rtems_style_tree_path = path.join(os.getcwd(),
                                      'acpica-rtems' + commit_hash)

    try:
        #
        # Make a copy of the base ACPICA source tree
        # and reset it to the specified commit
        #
        path.copy_tree(acpica_base_path, clone_repo_path)
        clone_repo.reset(['--hard', commit_hash])

        # Convert the tree hierarchy to the RTEMS source tree format
        for row in ACPICA_TO_RTEMS_TABLE:
            acpica_path = path.join(clone_repo_path, row[0])
            rtems_path = path.join(rtems_style_tree_path, row[1])

            if path.isdir(acpica_path):
                path.mkdir(rtems_path)
                for file in path.listdir(acpica_path):
                    file_path = path.join(acpica_path, file)
                    if path.isfile(file_path):
                        path.copy(file_path, rtems_path)
            else:
                path.mkdir(path.dirname(rtems_path))
                path.copy(acpica_path, rtems_path)
    except:
        try:
            path.removeall(rtems_style_tree_path)
        except:
            pass
        raise
    finally:
        try:
            path.removeall(clone_repo_path)
        except:
            raise

    return rtems_style_tree_path


def get_tree_diff(first_tree, second_tree):
    diff = ''
    subdir_queue = ['']

    try:
        while subdir_queue:
            subdirs = subdir_queue.pop(0)
            path1 = path.join(first_tree, subdirs)
            path2 = path.join(second_tree, subdirs)

            for file in path.listdir(path1):
                file1_path = path.join(path1, file)
                file2_path = path.join(path2, file)

                if path.isdir(file1_path):
                    subdir_queue.append(path.join(subdirs, file))
                    continue

                with open(file1_path, 'r') as file1, open(file2_path,
                                                          'r') as file2:
                    file1_lines = file1.readlines()
                    file2_lines = file2.readlines()
                    rel_file_path = path.join(subdirs, file)

                    unified_diff = difflib.unified_diff(
                        file1_lines, file2_lines,
                        path.join('a', rel_file_path),
                        path.join('b', rel_file_path))

                    for line in unified_diff:
                        diff += line
    except:
        raise

    return diff


def gen_patches(acpica_base_path, commit_list, output_path):
    patch_count = 0
    acpica_base_repo = git.repo(acpica_base_path)
    output_dir = 'rtems.acpica.patches.' + datetime.now().date().strftime(
        "%d-%m-%Y")

    try:
        output_path = path.join(output_path, output_dir)

        counter = 1
        orig_output_path = output_path
        while path.exists(output_path):
            output_path = orig_output_path
            output_path += '_' + str(counter)
            counter += 1

        path.mkdir(output_path)
    except:
        try:
            path.removeall(output_path)
        except:
            pass
        raise

    for commit in commit_list:
        try:
            parent_commit = acpica_base_repo.log(
                ['-1', commit + '^1', '--format=%H'])

            print('[' + commit + ']: Generating ACPICA source trees')
            before_tree = gen_rtems_style_tree(acpica_base_path, parent_commit)
            after_tree = gen_rtems_style_tree(acpica_base_path, commit)

            print('[' + commit + ']: Calculating diff')
            diff = get_tree_diff(before_tree, after_tree)

            if diff != '':
                full_commit = acpica_base_repo.show(
                    ['-s', '--format=%H', commit])
                patch_desc = acpica_base_repo.show([
                    '-s',
                    '--format=From: %an <%ae>%nDate: %ad%nSubject: ACPICA: %s%n%n%b',
                    commit
                ])
                patch_desc += '\n\nACPICA commit ' + full_commit
                patch_content = patch_desc + '\n' + diff
                patch_path = path.join(
                    output_path,
                    str(patch_count + 1) + '-' + commit + '.patch')

                with open(patch_path, 'w') as patch:
                    patch.write(patch_content)

                patch_count += 1
                print('[' + commit + ']: Patch saved under ' + patch_path)
            else:
                print('[' + commit + ']: No patch generated due to empty diff')

            path.removeall(before_tree)
            path.removeall(after_tree)
        except:
            try:
                path.removeall(before_tree)
                path.removeall(after_tree)
                path.removeall(output_path)
            except:
                pass
            raise

    return patch_count


def run(args=sys.argv):
    error_code = 0
    error_msg = None

    try:
        description = 'Generates patch files of commits in the ACPICA source tree '
        description += 'ready to be applied in the RTEMS source tree with a tool '
        description += 'like git-am.'

        usage = '%(prog)s [-h] (-c CHERRY_PICK | -r RANGE) -n NUM_DIGITS_HASH -a ACPICA_REPO [-o OUTPUT_DIR]\n\n'
        usage += 'CHERRY_PICK should be a list of commits to cherry-pick (e.g., \'commit1,commit2\').\n'
        usage += 'RANGE should specify a range of commits (e.g., \'commit1..commit2\').'

        args_parser = argparse.ArgumentParser(prog='rtems-gen-acpica-patches',
                                              description=description,
                                              usage=usage)
        args_parser_commits_grp = args_parser.add_mutually_exclusive_group(
            required=True)

        args_parser_commits_grp.add_argument(
            '-c',
            '--cherry-pick',
            help='Commit hashes to generate patches from.',
            type=str,
            default=None)
        args_parser_commits_grp.add_argument(
            '-r',
            '--range',
            help=
            'The first (exclusive) and last (inclusive) commits in the range of commits to generate patches from. If only the first is given HEAD is assumed as the end of the range.',
            type=str,
            default=None)
        args_parser.add_argument(
            '-n',
            '--num-digits-hash',
            help='Number of digits to use from the commit hash. Defaults to 8.',
            type=int,
            default=8)
        args_parser.add_argument('-a',
                                 '--acpica-repo',
                                 help='Path to ACPICA repository.',
                                 type=str,
                                 required=True)
        args_parser.add_argument(
            '-o',
            '--output-dir',
            help=
            'Output directory of the patches. If none is given the current directory is used.',
            type=str,
            default=os.getcwd())

        args = args_parser.parse_args()

        acpica_base_path = path.abspath(args.acpica_repo)
        acpica_base_repo = git.repo(acpica_base_path)

        commit_list = []
        if args.cherry_pick != None:
            commit_list = args.cherry_pick.split(',')
        else:
            range_commits = args.range.split('..')
            commit1 = range_commits[0]
            commit2 = 'HEAD'
            if len(range_commits) > 1:
                commit2 = range_commits[1]

            output = acpica_base_repo.log(
                ['--format=%H', commit1 + '..' + commit2])
            commit_list = output.split()
            commit_list.reverse()

        n = args.num_digits_hash
        for index, commit in enumerate(commit_list):
            if len(commit) < n:
                raise error.general('Commit ' + commit +
                                    ' contains less digits ' +
                                    'than the minimum of ' + str(n))

            # Cut down each commit to the value defined by --num-digits-hash
            commit_list[index] = commit[:n]

            if acpica_base_repo.validate_commit_hash(commit) == False:
                raise error.general('Commit ' + commit +
                                    ' not valid or ambiguous')

        patch_count = gen_patches(acpica_base_path, commit_list,
                                  path.abspath(args.output_dir))
        print('\n' + str(len(commit_list)) + ' commits parsed and ' +
              str(patch_count) + ' patches generated')

    except error.general as gerr:
        error_msg = str(gerr)
        error_code = 1
    except KeyboardInterrupt:
        error_msg = 'abort: User terminated'
        error_code = 1
    except:
        raise
        error_msg = 'abort: Unknown error'
        error_code = 1

    if error_msg != None:
        print(error_msg)

    sys.exit(error_code)


if __name__ == "__main__":
    run()
