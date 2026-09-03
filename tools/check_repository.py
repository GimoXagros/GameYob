#!/usr/bin/env python3
"""Read-only consistency checks; generators write only into a temporary folder."""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from urllib.parse import unquote

import generate_unicode_assets as unicode_assets

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def resources(galmuri):
    with tempfile.TemporaryDirectory(prefix='gameyob-resources-') as temporary:
        temp = Path(temporary)
        subprocess.run([sys.executable, str(ROOT / 'tools/generate_language_files.py'),
                        '--output-dir', str(temp / 'languages'),
                        '--builtin-output', str(temp / 'builtin_languages.inc')], check=True)
        pairs = [(p, ROOT / 'languages' / p.name) for p in (temp / 'languages').iterdir()]
        pairs.append((temp / 'builtin_languages.inc', ROOT / 'platform/common/builtin_languages.inc'))
        for generated, tracked in pairs:
            require(generated.read_text(encoding='utf-8') == tracked.read_text(encoding='utf-8'),
                    f'Stale generated text: {tracked}')
        print('Language resources: all 12 files and built-in table match (text newline normalized)')
    cp949 = (ROOT / 'assets/fonts/cp949_table.bin').read_bytes()
    require(cp949 == unicode_assets.generate_cp949_table(), 'Stale CP949 table')
    font = (ROOT / 'assets/fonts/unicode_font.bin').read_bytes()
    magic, version, reserved, count = struct.unpack_from('<4sHHI', font)
    require((magic, version, reserved) == (b'GYUF', 1, 0), 'Bad font header')
    require(len(font) == 12 + 10 * count, 'Bad font record count')
    codes = [struct.unpack_from('<H', font, 12 + i * 10)[0] for i in range(count)]
    require(codes == sorted(set(codes)), 'Unsorted or duplicate glyphs')
    if galmuri:
        regenerated = unicode_assets.generate_font(galmuri / 'Galmuri7.bdf', galmuri / 'Galmuri9.bdf')
        require(font == regenerated, 'Stale generated Unicode font')
        print('Unicode font: exact regeneration match')
    else:
        print('Unicode font regeneration: NOT RUN (supply --galmuri-dir); structure checked')
    for name, data in [('unicode_font.bin', font), ('cp949_table.bin', cp949)]:
        print(name, len(data), hashlib.sha256(data).hexdigest())
    print('Glyph records:', count)


def links():
    files = subprocess.check_output(['git', 'ls-files', '*.md'], cwd=ROOT).decode().splitlines()
    broken = []
    historical = []
    for name in files:
        for link in re.findall(r'\]\(([^)]+)\)', (ROOT / name).read_text(encoding='utf-8')):
            if '://' in link or link.startswith(('#', 'mailto:')):
                continue
            target = unquote(link.split('#')[0].strip('<>'))
            if target and not (ROOT / name).parent.joinpath(target).exists():
                entry = (name, link)
                (historical if name.startswith(('docs/releases/', 'backup/')) else broken).append(entry)
    print('Historical/archive broken local links (preserved):', historical)
    require(not broken, f'Broken active document links: {broken}')
    print('Active document file links: PASS (external URLs and anchors not tested)')


def crc16(data):
    result = 0xffff
    for byte in data:
        result ^= byte
        for _ in range(8):
            result = (result >> 1) ^ (0xa001 if result & 1 else 0)
    return result


def nds(path):
    data = path.read_bytes()
    require(len(data) >= 0x200, 'Truncated NDS header')
    ranges = []
    print(path.name, 'bytes=', len(data), 'sha256=', hashlib.sha256(data).hexdigest())
    for label, base in [('ARM9', 0x20), ('ARM7', 0x30)]:
        offset, entry, address, size = struct.unpack_from('<4I', data, base)
        require(size > 0 and offset + size <= len(data), f'{label} ROM bounds')
        require(address <= entry < address + size, f'{label} entry bounds')
        ranges.append((address, address + size))
        print(label, f'rom={offset:08x} entry={entry:08x} load={address:08x} size={size}')
    require(ranges[0][1] <= ranges[1][0] or ranges[1][1] <= ranges[0][0], 'ARM payload load overlap')
    banner = struct.unpack_from('<I', data, 0x68)[0]
    version = struct.unpack_from('<H', data, banner)[0]
    banner_size = {1: 0x840, 2: 0x940, 3: 0xa40, 0x103: 0x23c0}.get(version)
    require(banner_size and banner + banner_size <= len(data), 'Banner bounds/version')
    require(crc16(data[banner + 0x20:banner + 0x840]) == struct.unpack_from('<H', data, banner + 2)[0], 'Banner CRC')
    require(crc16(data[:0x15e]) == struct.unpack_from('<H', data, 0x15e)[0], 'NDS header CRC')
    require(any(data[banner + 0x20:banner + 0x220]), 'Empty banner icon')
    # Compare RGB555 pixels to the committed indexed BMP, independent of
    # ndstool's palette index remapping and tiled storage order.
    bmp = (ROOT / 'platform/ds/icon.bmp').read_bytes()
    require(bmp[:2] == b'BM' and struct.unpack_from('<iiH', bmp, 18) == (32, 32, 1), 'Icon BMP dimensions')
    require(struct.unpack_from('<H', bmp, 28)[0] == 8, 'Expected indexed 8-bit source icon')
    pixels = struct.unpack_from('<I', bmp, 10)[0]
    palette = 14 + struct.unpack_from('<I', bmp, 14)[0]
    for y in range(32):
        for x in range(32):
            index = bmp[pixels + (31 - y) * 32 + x]
            b, g, r = bmp[palette + index * 4:palette + index * 4 + 3]
            expected = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
            tile = (y // 8) * 4 + x // 8
            packed = data[banner + 0x20 + tile * 32 + (y % 8) * 4 + (x % 8) // 2]
            color = (packed >> (4 * (x % 2))) & 15
            actual = struct.unpack_from('<H', data, banner + 0x220 + color * 2)[0] & 0x7fff
            require(actual == expected, 'NDS banner differs from icon.bmp')
    print('Banner icon pixels match committed BMP')
    marker = data.find(b'\xed\xa5\x8d\xbf Chishm\x00')
    require(marker >= 0, 'DLDI marker missing')
    reserved = 1 << data[marker + 15]
    require(marker + reserved <= len(data), 'DLDI reserved bounds')
    print(f'banner={banner:08x} version={version} size={banner_size} DLDI={marker:08x} reserved={reserved}')
    for label, base in [('ARM9i', 0x1c0), ('ARM7i', 0x1d0)]:
        offset, _, address, size = struct.unpack_from('<4I', data, base)
        require(size > 0 and offset + size <= len(data), f'{label} ROM bounds')
        print(label, f'rom={offset:08x} load={address:08x} size={size}')
    print('unitCode=', data[0x12], 'gameCode=', data[12:16],
          'titleID=', hex(struct.unpack_from('<Q', data, 0x230)[0]))
    elf = path.parent / 'arm9.elf'
    if elf.exists():
        elf_data = elf.read_bytes()
        for asset in ('unicode_font.bin', 'cp949_table.bin'):
            require((ROOT / 'assets/fonts' / asset).read_bytes() in elf_data, f'{asset} missing from ARM9 ELF')
        print('Embedded Unicode/CP949 bytes in ARM9 ELF: PASS')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--galmuri-dir', type=Path)
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    resources(args.galmuri_dir)
    links()
    if args.build_dir:
        for name in ('gameyob.nds', 'gameyob_dsi.nds'):
            nds(args.build_dir / name)


if __name__ == '__main__':
    main()
