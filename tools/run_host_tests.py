#!/usr/bin/env python3
"""Run the exact portable compile/test commands maintained in the core CI."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def parse_test_block(block):
    """Parse the compile/run pairs used by core CI, rejecting unknown commands."""
    lines = '\n'.join(line[10:] for line in block.splitlines())
    commands = lines.replace('\\\n', '').splitlines()
    tests = []
    pending = None
    directory = None
    for command in commands:
        command = command.strip()
        match = re.fullmatch(r'(\w+)="\$\(mktemp -d\)"', command)
        if match:
            if pending is None or directory is not None:
                raise RuntimeError('Unexpected temporary directory command')
            directory = match.group(1)
            continue
        argv = shlex.split(command)
        if argv and argv[0] == 'g++':
            if pending is not None or '-o' not in argv:
                raise RuntimeError('Unexpected core CI compile command')
            output_index = argv.index('-o') + 1
            if output_index >= len(argv):
                raise RuntimeError('Missing core CI compile output')
            pending = (argv, output_index)
            continue
        if pending is None or not argv or argv[0] != pending[0][pending[1]]:
            raise RuntimeError('Compile and run target mismatch')
        args = argv[1:]
        if directory is not None:
            if len(args) != 1 or args[0] not in ('$' + directory,
                                                  '$' + directory + '/'):
                raise RuntimeError('Temporary directory argument mismatch')
            args = [('/' if args[0].endswith('/') else '')]
        elif args:
            raise RuntimeError('Unexpected core CI test arguments')
        tests.append((pending[0], pending[1], args, directory is not None))
        pending = None
        directory = None
    if pending is not None or not tests:
        raise RuntimeError('Incomplete core CI test block')
    return tests


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--asan', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    workflow = (root / '.github/workflows/core-regression-tests.yml').read_text()
    blocks = re.findall(r'        run: \|\n((?:          .*\n)+)', workflow)
    if not blocks:
        raise RuntimeError('No portable test commands found in core CI')
    failures = []
    test_count = 0
    with tempfile.TemporaryDirectory(prefix='gameyob-tests-') as temp:
        for block in blocks:
            for compile_cmd, output_index, test_args, needs_directory in \
                    parse_test_block(block):
                test_count += 1
                name = Path(compile_cmd[output_index]).name
                output = str(Path(temp) / name)
                compile_cmd[output_index] = output
                if args.sanitize:
                    compile_cmd[1:1] = ['-fsanitize=undefined',
                                       '-fno-sanitize-recover=all', '-g']
                if args.asan:
                    compile_cmd[1:1] = ['-fsanitize=address',
                                       '-fno-omit-frame-pointer', '-g']
                print('BUILD', name, flush=True)
                built = subprocess.run(compile_cmd, cwd=root)
                run_environment = None
                if args.asan:
                    run_environment = os.environ.copy()
                    run_environment['ASAN_OPTIONS'] = (
                        'detect_leaks=1:halt_on_error=1:strict_string_checks=1')
                argument = []
                if needs_directory:
                    directory = Path(temp) / (name + '-files')
                    directory.mkdir()
                    argument = [str(directory) + test_args[0]]
                if built.returncode or subprocess.run(
                        [output, *argument], cwd=root,
                        env=run_environment).returncode:
                    failures.append(name)
    print('Tests:', test_count, 'Failures:', failures, flush=True)
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
