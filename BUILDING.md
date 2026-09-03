# Build and maintenance baseline

## English

The active targets are `platform/ds/gameyob.nds` and
`platform/ds/gameyob_dsi.nds`. Both contain the same DS/TWL code and unit code
`0x02`; the latter retains the `GYOB` title ID. A launcher, not the filename,
decides the execution mode. Native 3DSX is manual-only/deferred. SDL and old
devkitARM DS Makefiles are legacy paths, not supported release build gates.

Use a **fresh checkout** for clean builds. The authoritative DS workflow is
`.github/workflows/build-ds.yml`; its pinned environment is BlocksDS 1.22.2:

```sh
docker run --rm --volume "$PWD:/workspace" --workdir /workspace \
  skylyrac/blocksds:slim-v1.22.2@sha256:a073c57ffd340542a6e09d14616dd925d219bc534399dbdc73d2abaa04c1628e \
  bash -euo pipefail -c 'git config --global --add safe.directory /workspace; make -f Makefile.blocksds clean; make -f Makefile.blocksds'
```

The container provides `BLOCKSDS` and `WONDERFUL_TOOLCHAIN`. Do not override
them with guessed SDK paths. The audited image reports Wonderful GCC
`16.1.1 20260516` and ndstool `1.22.2`. ARM7/ARM9 ELFs are in `build/`;
maps are under `build/blocksds/`. The ROM banner version remains v0.5.9-ko,
and Debug version information embeds the current 12-digit Git revision.

Portable tests require Linux, Python 3.10+ and g++. The runner reads the exact
compile/run blocks from `core-regression-tests.yml`, avoiding a duplicate list:

```sh
python3 tools/run_host_tests.py
python3 tools/run_host_tests.py --sanitize
python3 tests/package_release_test.py
python3 tools/check_repository.py --galmuri-dir /path/to/galmuri/dist
git diff --check
```

`--sanitize` enables UBSan, not ASan. The tests use `/tmp` and are not advertised
as native Windows tests. Resource/package checks can run with Windows Python.
Use Galmuri revision `71e1cacf1437a11220307120e63e30bc275312d4`; the checker
writes only temporary generated outputs and leaves tracked assets untouched.
Without `--galmuri-dir`, font structure is checked but full font regeneration
is explicitly **not run**. Language/CP949 regeneration still runs. See
[font provenance](assets/fonts/README.md) and [language sources](languages/README.md).

`.github/workflows/repository-preflight.yml` runs the host checks, pinned font
regeneration, two clean DS builds, binary comparison, ndstool inspection and
artifact upload. It can be dispatched manually. For downloaded artifacts:

```sh
python3 tools/check_repository.py --build-dir /path/to/artifact/first
```

There is no repository-wide formatter/linter configuration. `git diff --check`,
Python compilation, compiler warnings and UBSan are distinct checks, not a
claim of exhaustive static analysis or hardware validation.

Packaging (local verification only; this does not publish a release):

```sh
python3 tools/package_release.py --nds platform/ds/gameyob.nds \
  --dsi platform/ds/gameyob_dsi.nds --release-notes docs/releases/v0.5.9-ko.md \
  --output .codex-tmp/preflight-package/gameyob.zip
```

The ZIP contains two NDS programs, EN/JA/KO guides and language examples,
notices and checksums. It excludes native 3DSX unless explicitly requested
with the archived-only option. ROMs, BIOS and firmware are never inputs.
Do not replace published assets with an unreleased maintenance build.

## 日本語

現在の開発対象はDS/DSiの`gameyob.nds`と`gameyob_dsi.nds`です。両方とも
DS/TWLコードとunit code `0x02`を持ち、後者には`GYOB`のtitle IDがあります。
実行モードはランチャーが決定します。ネイティブ3DSXは保留・手動ビルドのみです。
旧SDL/devkitARM DS経路は現在のリリース検証対象ではありません。

上記の固定コンテナとコマンドを新しいチェックアウトで実行してください。
ホストC++試験にはLinux・Python 3.10以上・g++が必要です。`--sanitize`は
UBSanです。`--galmuri-dir`を省略した場合、フォントの完全再生成は未実行と
表示されます。言語・CP949検査は実行されます。生成物・ZIPの検査はWindows
Pythonでも実行できます。検査は実機承認や全コードの静的解析の代わりには
なりません。既存のバージョン・公開タグ・配布物を変更せず、未公開の整備ビルドは
`.codex-tmp`内で検査します。パッケージにはROM・BIOS・ファームウェアを含めません。

## 한국어

현재 개발 대상은 DS/DSi의 `gameyob.nds`와 `gameyob_dsi.nds`입니다. 둘 다 동일한
DS/TWL 코드와 unit code `0x02`를 가지며 후자에는 `GYOB` 제목 ID가 있습니다.
실행 모드는 런처가 결정합니다. 네이티브3DSX는 보류·수동 빌드만 유지합니다.
이전 SDL/devkitARM DS 경로는 현재 릴리스 검증 대상이 아닙니다.

위의 고정 컨테이너와 명령을 새 체크아웃에서 실행하십시오. 호스트 C++ 시험에는
Linux·Python 3.10 이상·g++가 필요하며 `--sanitize`는 UBSan입니다.
`--galmuri-dir`를 생략하면 글꼴 전체 재생성은 미실행으로 표시됩니다.
언어·CP949 검사는 계속 실행됩니다. 리소스·ZIP 검사는 Windows Python에서도
실행할 수 있습니다. 자동시험은 실기 승인이나 전체 코드 정적 분석을 대체하지
않습니다. 버전·공개 태그·배포물은 변경하지 않으며 미배포 정비 빌드는
`.codex-tmp`에서 검사합니다. 패키지에는 ROM·BIOS·펌웨어를 포함하지 않습니다.
