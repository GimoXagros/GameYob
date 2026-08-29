# Deferred native 3DSX work

These tasks are postponed while DS/DSi development remains active.

1. Profile and reduce intermittent gameplay frame drops on Old 3DS/2DS and New
   3DS hardware.
2. Reimplement aspect-preserving and full-screen scaling with stable output,
   no corruption or ghosting, and acceptable Old 3DS/2DS performance.
3. Validate the renderer, SGB/custom borders, CSND/NDSP audio, BIOS selection,
   cheats, RTC, repeated ROM loading, and long sessions on physical hardware.
4. Validate native LAN link play on two physical Nintendo 3DS systems,
   including discovery, reconnect, packet loss, SRAM/RTC synchronization, and
   long sessions.
5. Design and evaluate a bridge between native 3DS UDP and the `.nds`
   raw-802.11 NiFi transport.
6. Re-enable automatic native 3DS CI and default release packaging only after
   the hardware acceptance matrix passes.

# 보류된 네이티브 3DSX 작업

다음 작업은 DS/DSi 개발을 우선하는 동안 보류합니다.

1. Old 3DS/2DS 및 New 3DS 실기에서 간헐적인 게임 프레임 드롭을 분석하고
   줄입니다.
2. 화면 깨짐과 잔상이 없고 Old 3DS/2DS에서도 충분히 빠른 비율 유지·전체
   화면 확대 기능을 다시 구현합니다.
3. 렌더러, SGB/사용자 보더, CSND/NDSP 사운드, BIOS 선택, 치트, RTC, ROM
   반복 실행 및 장시간 구동을 실기에서 검증합니다.
4. 실제 Nintendo 3DS 두 대에서 검색·재접속·패킷 유실·SRAM/RTC 동기화와
   장시간 구동을 포함한 네이티브 LAN 링크를 검증합니다.
5. 네이티브 3DS UDP와 `.nds` raw-802.11 NiFi 사이의 브리지를 설계하고
   검토합니다.
6. 실기 승인 시험표를 통과한 뒤 네이티브 3DS 자동 CI와 기본 릴리스 패키징을
   다시 활성화합니다.
