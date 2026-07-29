# 검증 결과

- DS NDS 컴파일: PASS
- DSi NDS 컴파일: PASS
- 동일 소스 2회 클린 빌드 SHA-256 일치: PASS
- `ndstool` Nintendo logo/header/banner CRC: 두 파일 모두 PASS
- 완성형 한글 11,172자 비어 있지 않은 글리프 검사: PASS
- CP949 `B0 A1` → Unicode `U+AC00` 매핑 검사: PASS
- 생성 폰트 시각 스모크 테스트: PASS (`verification/hangul-preview.png`)

자동 NDS 실행 검증은 이 PC의 emucap DeSmuME 실행 바이너리가 준비되지 않아
수행하지 못했다. 따라서 최종 플래시카드/DSi 환경에서는 DLDI 자동 패치를 켜고,
한글 이름의 테스트 ROM과 UTF-8/CP949 치트 파일을 한 번 확인하는 것이 좋다.

## v0.5.2-ko.1 배포 파일

- `gameyob-v0.5.2-ko.1.nds`
  - SHA-256: `6B1DAF0B5099184434D5F44C30B40A9B543659C46367A717F1DA2920E6A60F7A`
  - 크기: 579,648 bytes
- `gameyob-v0.5.2-ko.1-dsi.nds`
  - SHA-256: `026902392078104E2E4AD7556607E3E33EB48C93EF9844EAAF88D9395EDC23C0`
  - 크기: 602,624 bytes
