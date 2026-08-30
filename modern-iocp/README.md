# modern-iocp

Windows IOCP 게임 서버를 **재현 가능하고 검증 가능한 형태로** 다시 만드는 프로젝트.

2024년에 만든 IOCP 서버 5종은 기능은 많았지만 테스트도 CI 도 없었고, 필요한 toolset 버전이
어디에도 적혀 있지 않아 몇 년 뒤 다른 PC 에서 빌드조차 되지 않았다. 여기서는 그 반대로 간다.

## 무엇을 증명하려는가

- **결함을 테스트로 고정한다** — 레거시에서 발견한 결함 4종을 재현 테스트로 먼저 실패시킨 뒤 고친다
- **크래시를 분석할 수 있다** — Release 에서도 심볼을 남기고, 미니덤프 생성부터 호출 스택
  복원까지가 자동 검증된다
- **부하를 측정한다** — 동시 접속·패킷 레이트·백프레셔를 commit hash 와 실행 환경과 함께 기록한다
- **누구나 같은 결과를 낸다** — 의존성·toolset·SDK 를 기계가 읽는 파일에 고정한다

## 현재 상태

**S1(골격 + 덤프 배관) 완료.** 단일 세션 echo 서버가 동작하고, 크래시 덤프 파이프라인이
자동 검증된다.

| 검증 | 상태 |
|---|---|
| 빌드 (x64 Release, `/W4 /WX`) | 통과 |
| `smoke.echo_roundtrip` | 통과 |
| 덤프 생성 + 짝 PDB 확인 | 통과 |
| GitHub Actions (`windows-2025`) | 통과 — [run #2](https://github.com/ldcity/Modern-IOCP/actions/runs/33321582508) |

아직 없는 것: 다중 세션, IO 참조 카운트, 부분 송신 처리, 부하 측정, DB/Redis 연동.

## 빌드

```powershell
.\scripts\phase1.ps1 doctor   # 환경 확인. 부족한 것을 이름으로 알려준다
.\scripts\phase1.ps1 build
.\scripts\phase1.ps1 test
```

`doctor` 가 통과하지 않으면 빌드도 통과하지 않는다.

## 요구 환경

| 항목 | 버전 | 고정 위치 |
|---|---|---|
| Visual Studio | 2026 (toolset v145 / 14.51 계열) | `CMakePresets.json` |
| Windows SDK | 10.0.26100.0 | `CMakePresets.json` |
| CMake | 3.28 이상 | `CMakeLists.txt` |
| vcpkg | baseline `114d9fe6` | `vcpkg.json` |

vcpkg 를 clone·bootstrap 한 뒤 그 경로를 `VCPKG_ROOT` 환경변수로 설정한다.

**CI 도 같은 것을 쓴다.** `x64-release-ci` preset 은 위 표를 그대로 상속하고 build 디렉터리만
분리한다 — GitHub Actions `windows-2025` 러너도 VS 2026 / MSVC 14.51 이다.
VS·SDK 가 없으면 configure 가 즉시 실패한다. 다만 **toolset 은 `14.51` 계열까지만 강제된다**
(patch 는 아니다) — 근거와 실측은 [`docs/infra/build-system.md`](docs/infra/build-system.md).

## 구조

```
Core/          IOCP 네트워크 코어, 크래시 핸들러   → Core.lib
GameServer/    서버 실행 파일                      → GameServer.exe
Tests/         gtest 기반 테스트                   → Tests.exe
scripts/       원커맨드 래퍼
docs/          덤프 분석, VS 빌드, 인프라 해설
```

`Core` 는 정적 라이브러리이고 서버와 테스트가 이를 링크한다.

## 문서

| 문서 | 내용 |
|---|---|
| [`docs/dump-analysis-00-plumbing.md`](docs/dump-analysis-00-plumbing.md) | 덤프 배관 검증. WinDbg 에서 겪은 함정 3건 |
| [`docs/vs-build.md`](docs/vs-build.md) | Visual Studio 로 열고 디버그하는 방법 |
| [`docs/infra/`](docs/infra/) | 빌드 시스템·스크립트·CI 가 왜 그렇게 돼 있는지 |

설계 결정과 버린 안은 저장소 루트의 `.ai/decisions/` 에 있다.
