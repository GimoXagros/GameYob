"""Check the packager with synthetic executable bytes, never game data."""
import hashlib
from pathlib import Path
import posixpath
import re
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]


class PackageTest(unittest.TestCase):
    def test_integrity_and_guide_links(self):
        with tempfile.TemporaryDirectory() as temporary:
            temp = Path(temporary)
            nds = temp / 'synthetic.nds'
            nds.write_bytes(b'Synthetic package-test input; not an NDS program')
            outputs = [temp / 'first.zip', temp / 'second.zip']
            for output in outputs:
                subprocess.run([sys.executable, str(ROOT / 'tools/package_release.py'),
                                '--nds', str(nds), '--dsi', str(nds),
                                '--release-notes', str(ROOT / 'docs/releases/v0.5.9-ko.md'),
                                '--output', str(output)], check=True)
            self.assertEqual(outputs[0].read_bytes(), outputs[1].read_bytes())
            with zipfile.ZipFile(outputs[0]) as archive:
                names = archive.namelist()
                self.assertNotIn('gameyob.3dsx', names)
                for required in ('LICENSE', 'OFL.txt', 'STB_LICENSE.txt',
                                 'THIRD_PARTY_NOTICES.txt', 'gameyob.nds', 'gameyob_dsi.nds'):
                    self.assertIn(required, names)
                for line in archive.read('SHA256SUMS.txt').decode().splitlines():
                    expected, name = line.split('  ', 1)
                    self.assertEqual(hashlib.sha256(archive.read(name)).hexdigest(), expected)
                for code in ('en', 'ja', 'ko'):
                    name = f'gameyob/docs/user-guide.{code}.md'
                    for link in re.findall(r'\]\(([^)]+)\)', archive.read(name).decode('utf-8')):
                        if '://' in link or link.startswith('#'):
                            continue
                        target = posixpath.normpath(posixpath.join(posixpath.dirname(name), link))
                        self.assertIn(target, names, f'Broken packaged link in {name}: {link}')


if __name__ == '__main__':
    unittest.main()
