# Repository branch, tag, and release policy

The default branch `master` is the current integration line. Topic branches
may be removed **from the remote** after their commits are confirmed to be
ancestors of `master` and no open pull request needs them. Local branches,
worktrees, uncommitted files, and private diagnostics are separate and must not
be removed by that cleanup. An unmerged branch requires review of its unique
commits, not an automatic merge or deletion.

Published version tags and release assets are historical records. Do not move
tags, replace files attached to an existing release, or rewrite release bodies
as a cosmetic cleanup. Display-title normalization is allowed when it does
not change the tag, body, assets, or latest-release designation. New stable
packages require a separately verified tagged build and release gate; routine
documentation changes do not create another version or release.

The latest stable DS/DSi release is [v0.5.11](https://github.com/GimoXagros/GameYob/releases/tag/v0.5.11).
The earlier packages remain in [`old_releases`](../../old_releases), and the
native 3DSX archive remains in [`backup/3dsx`](../../backup/3dsx). See
[BUILDING.md](../../BUILDING.md) for current build checks and
[CONTRIBUTORS.md](../../CONTRIBUTORS.md) for historical attribution.

## 2026-10-02 remote branch cleanup record

All three removed remote topic heads were freshly fetched and verified as
ancestors of `master` at `55a4a867e9b2a70355a84ce8afc5e863ba3657b6`.
The exact former heads are recorded for recovery; local branches and worktrees
were not deleted:

| Former remote branch | Last verified commit |
| --- | --- |
| `feature/sgb-host-runtime-next` | `bde82c1fa3dada826d43e5b6d7ef9a9a180d7a82` |
| `fix/rom-reload-lifecycle-stability` | `763f6bfab8d7f3bed164976277516954141c1618` |
| `fix/sgb-printer-video-prerelease` | `ce755b90beb468546182432ceecb977f76ce2179` |

The v0.5.5-ko, v0.5.3-ko, and v0.5.2-ko.1 release **display titles** were
normalized to `GameYob v0.5.x`; their tags, release bodies, and assets were
checked unchanged. All eight published release/tag pairs and the v0.5.11
Latest designation were retained. This is housekeeping, not a new build.
