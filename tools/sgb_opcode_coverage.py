#!/usr/bin/env python3
"""Report fault-explicit SGB host CPU opcode coverage from the source."""
import argparse
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


def cases(source: str, start: str, end: str) -> list[int]:
    section = source.split(start, 1)[1].split(end, 1)[0]
    return sorted({int(value, 16) for value in re.findall(r"case 0x([0-9a-fA-F]{2})\s*:", section)})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    source = (ROOT / "platform/common/sgb_host.cpp").read_text(encoding="utf-8")
    cpu = cases(source, "int SgbHostCpu::run", "SgbHostPpu::SgbHostPpu")
    apu = cases(source, "int SgbHostApu::run", "void SgbHostApu::keyOn")
    print(f"65C816: {len(cpu)}/256; remaining {256 - len(cpu)}")
    print(f"SPC700: {len(apu)}/256; remaining {256 - len(apu)}")
    print("65C816 opcodes:", " ".join(f"{value:02X}" for value in cpu))
    print("SPC700 opcodes:", " ".join(f"{value:02X}" for value in apu))
    if args.check:
        if len(cpu) != len(set(cpu)) or len(apu) != len(set(apu)):
            raise SystemExit("duplicate opcode coverage")
        if not cpu or not apu:
            raise SystemExit("empty opcode coverage")


if __name__ == "__main__":
    main()
