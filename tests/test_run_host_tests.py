"""Guard the portable runner against silently skipping core CI commands."""
import importlib.util
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'run_host_tests', ROOT / 'tools/run_host_tests.py')
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


class HostRunnerParserTest(unittest.TestCase):
    def test_every_core_ci_compile_and_run_is_parsed(self):
        workflow = (ROOT / '.github/workflows/core-regression-tests.yml').read_text()
        blocks = re.findall(r'        run: \|\n((?:          .*\n)+)', workflow)
        tests = [test for block in blocks
                 for test in RUNNER.parse_test_block(block)]
        self.assertEqual(len(tests), sum(
            len(re.findall(r'^          g\+\+ ', block, re.MULTILINE))
            for block in blocks))
        self.assertGreaterEqual(len(tests), 32)
        self.assertEqual(
            [Path(test[0][test[1]]).name for test in tests].count(
                'gbgfx_stage_test'), 1)
        self.assertEqual(sum(test[3] for test in tests), 2)

    def test_multi_command_block_preserves_both_tests_and_directory(self):
        block = '''          g++ first.cpp -o /tmp/first
          /tmp/first
          g++ second.cpp -o /tmp/second
          trace_dir="$(mktemp -d)"
          /tmp/second "$trace_dir/"
'''
        tests = RUNNER.parse_test_block(block)
        self.assertEqual([test[0][test[1]] for test in tests],
                         ['/tmp/first', '/tmp/second'])
        self.assertEqual(tests[1][2:], (['/'], True))

    def test_unknown_command_is_not_skipped(self):
        block = '''          g++ first.cpp -o /tmp/first
          /tmp/first
          echo hidden
'''
        with self.assertRaises(RuntimeError):
            RUNNER.parse_test_block(block)


if __name__ == '__main__':
    unittest.main()
