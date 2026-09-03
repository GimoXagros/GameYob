#!/usr/bin/env python3
"""Create a deterministic GameYob archive with accompanying license notices."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import zipfile


ZIP_TIME = (2026, 1, 1, 0, 0, 0)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def packaged_guide(path: Path) -> bytes:
    """Resolve repository-relative links for gameyob/docs in the ZIP."""
    text = path.read_text(encoding="utf-8")
    text = text.replace("(../../languages/README.md)", "(../languages/README.md)")
    text = text.replace("(../../backup/3dsx", "(https://github.com/GimoXagros/GameYob/tree/master/backup/3dsx")
    return text.encode("utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nds", type=Path, required=True)
    parser.add_argument("--dsi", type=Path, required=True)
    parser.add_argument(
        "--3dsx",
        dest="three_dsx",
        type=Path,
        help="optional archived native 3DS build",
    )
    parser.add_argument("--output", type=Path, default=Path("gameyob.zip"))
    parser.add_argument(
        "--release-notes",
        type=Path,
        required=True,
        help="release record to store as RELEASE_NOTES.md",
    )
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    files: dict[str, bytes] = {
        "gameyob.nds": args.nds.read_bytes(),
        "gameyob_dsi.nds": args.dsi.read_bytes(),
        "LICENSE": (root / "LICENSE").read_bytes(),
        "OFL.txt": (root / "assets/fonts/OFL.txt").read_bytes(),
        "STB_LICENSE.txt": (root / "third_party/stb/LICENSE.txt").read_bytes(),
        "THIRD_PARTY_NOTICES.txt": (root / "THIRD_PARTY_NOTICES.txt").read_bytes(),
        "RELEASE_NOTES.md": args.release_notes.read_bytes(),
        "gameyob/docs/user-guide.en.md": packaged_guide(root / "docs/guides/user-guide.en.md"),
        "gameyob/docs/user-guide.ja.md": packaged_guide(root / "docs/guides/user-guide.ja.md"),
        "gameyob/docs/user-guide.ko.md": packaged_guide(root / "docs/guides/user-guide.ko.md"),
        "gameyob/languages/README.md": (root / "languages/README.md").read_bytes(),
    }
    if args.three_dsx is not None:
        files["gameyob.3dsx"] = args.three_dsx.read_bytes()
    for language in sorted((root / "languages").iterdir()):
        if language.suffix.lower() in {".ini", ".json", ".xml", ".yaml", ".yml"}:
            files[f"gameyob/languages/{language.name}"] = language.read_bytes()

    checksums = "".join(
        f"{digest(data)}  {name}\n" for name, data in sorted(files.items())
    ).encode("ascii")
    files["SHA256SUMS.txt"] = checksums

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, ZIP_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)

    output_hash = digest(args.output.read_bytes())
    args.output.with_suffix(args.output.suffix + ".sha256").write_text(
        f"{output_hash}  {args.output.name}\n", encoding="ascii"
    )
    print(f"created {args.output} ({output_hash})")


if __name__ == "__main__":
    main()
