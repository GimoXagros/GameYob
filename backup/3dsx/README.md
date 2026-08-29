# Native 3DSX backup

Native `gameyob.3dsx` development is paused after `v0.5.5-ko`. Nintendo 3DS
users can run the supported `gameyob.nds` build in DS mode, so current active
development and automatic CI focus on the DS/DSi targets.

This directory preserves the native 3DS executable published in the
`v0.5.5-ko` release:

- `gameyob_v0.5.5-ko.3dsx`
- Source revision: `6b26cfeb892a6b37666b5a1d528cbb7ce7fe4561`
- SHA-256: `f8c4978dd588e49b946b4318e0d340858eb01ac45bc0ce062120392f96622481`

The native source remains under [`platform/3ds`](../../platform/3ds) so its
history and future development are not lost. The 3DS workflow is retained as a
manual workflow, and release packaging accepts a 3DSX only when explicitly
provided.

See [TODO.md](TODO.md) for the deferred native 3DS work.

## 네이티브 3DSX 백업

네이티브 `gameyob.3dsx` 개발은 `v0.5.5-ko` 이후 잠시 보류합니다. Nintendo
3DS에서도 지원되는 `gameyob.nds`를 DS 모드로 실행할 수 있으므로 현재 자동
빌드와 개발은 DS/DSi 대상에 집중합니다.

이 폴더에는 `v0.5.5-ko`에서 배포한 3DSX 실행 파일을 보존합니다. 네이티브
소스는 이력과 차후 개발을 위해 [`platform/3ds`](../../platform/3ds)에 그대로
유지하며, 3DS 빌드는 필요할 때 수동으로만 실행합니다.
