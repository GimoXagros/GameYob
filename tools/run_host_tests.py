#!/usr/bin/env python3
"""Run the exact portable compile/test commands maintained in the core CI."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


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
    with tempfile.TemporaryDirectory(prefix='gameyob-tests-') as temp:
        for block in blocks:
            lines = '\n'.join(line[10:] for line in block.splitlines())
            commands = lines.replace('\\\n', '').splitlines()
            if len(commands) != 2:
                raise RuntimeError('Unexpected core CI test command layout')
            compile_cmd = shlex.split(commands[0])
            if compile_cmd[0] != 'g++' or '-o' not in compile_cmd:
                raise RuntimeError('Expected a g++ test command')
            output_index = compile_cmd.index('-o') + 1
            name = Path(compile_cmd[output_index]).name
            if commands[1].strip() != compile_cmd[output_index]:
                raise RuntimeError('Compile and run target mismatch')
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
            if built.returncode or subprocess.run(
                    [output], cwd=root, env=run_environment).returncode:
                failures.append(name)
    print('Tests:', len(blocks), 'Failures:', failures, flush=True)
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
