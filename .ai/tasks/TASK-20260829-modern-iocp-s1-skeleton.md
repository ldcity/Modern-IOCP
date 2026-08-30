# TASK-20260829-modern-iocp-s1-skeleton: S1 — 골격 + 덤프 plumbing

- Status: **Done** (2026-08-31) — Acceptance criteria 전부 충족. 고정된 preset 으로 도는
  CI run #2 green (`2f430c0`)
- Implementer: **사용자** (코드) + Claude (빌드·CI·스크립트 인프라). 아래 "작성자 분담" 참조
- Reviewer: Codex
- Repositories: `D:\GameProjects\Portfolio`
- Baseline: `main` `ec16031`
- Owned paths: `modern-iocp/**`, `.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md`

Phase 1 전체 정의는 [`TASK-20260829-modern-iocp-phase1.md`](TASK-20260829-modern-iocp-phase1.md) 다.
이 문서는 그 6단계 중 **S1 만** 다룬다. 완료 조건·역할 경계·결함 재현 계약을 여기서 반복하지 않는다.

## 환경 실측 (2026-08-29)

코드를 쓰기 전에 이 머신을 실측했다. **문서에 적혀 있던 전제 둘이 틀렸다.**

| 항목 | 실측값 |
|---|---|
| Visual Studio | **Community 2026** — `C:\Program Files\Microsoft Visual Studio\18\Community` |
| MSVC toolset | **v145 / 14.51.36231** (v143 은 설치돼 있지 않다) |
| Windows SDK | 10.0.26100.0 |
| CMake | 4.3.3 |
| vcpkg | 미설치였음 → 이번에 `D:\tools\vcpkg` 에 설치 |

감사 문서의 "레거시가 v143 부재로 재빌드 실패" 는 이 사실과 일치한다.
**v145 로 고정하고 CI 러너는 `windows-2025` 를 쓴다** (사용자 결정 2026-08-29).
로컬과 CI 의 toolset 이 완전히 같지 않을 수 있으므로 그 차이를 `README.md` 에 적는다.

## Problem evidence

`modern-iocp/` 는 빈 디렉터리다. 빌드 시스템·CI·실행 경로가 없어서 어떤 주장도 검증할 수 없다.

덤프 plumbing 을 S1 에 두는 이유는 Codex 리뷰 P2 다 — 마지막에 붙이면 덤프용 빌드 플래그와
PDB 를 나중에 소급 적용하게 되고, 그때 빌드 구성이 이미 굳어 있어 재작업이 커진다.
**덤프가 나오고 열린다는 것을 코드가 얇을 때 확인한다.**

레거시 5종은 v143 toolset 부재로 빌드가 확인되지 않았다. 그 재발을 막는 것이 이 단계의
숨은 목적이다 — **필요한 도구 버전을 처음부터 파일에 고정한다.**

## Goal

빈 저장소를 clone 한 사람이 `scripts\phase1.ps1 build` 한 줄로 x64 Release 를 빌드하고,
`scripts\phase1.ps1 test` 로 echo 왕복 smoke 를 통과시키고, `--crash-test` 로 minidump 를
만들어 심볼과 함께 열 수 있다. CI 가 같은 것을 Windows 러너에서 재현한다.

## Non-goals

- 결함 재현 테스트 D1~D4 — S2·S3
- 부하·백프레셔 측정 — S4
- MySQL/Redis·인증 — S5
- **실제 덤프 분석 문서**(`docs/dump-analysis-01.md`) — S6. S1 은 plumbing 확인만 한다
- 세션 관리·IO 참조 카운트 설계 — S2. S1 의 echo 서버는 **의도적으로 얇게** 만든다
- 성능 최적화 일체. lock-free 자료구조 금지

## 작성자 분담

Phase 1 역할 경계표를 이 단계에 적용한 결과다. 경계를 옮기려면 phase1 문서의 표를 고친다.

개념이 아니라 **파일 경로 단위**로 나눈다 (Codex 리뷰 P2). 덤프 plumbing 은 `src/` · `CMakeLists` ·
`scripts/` · workflow 에 걸쳐 한 덩어리로 움직이므로, 경로로 자르지 않으면 경계가 문서에만 남는다.

| 파일 | 작성자 |
|---|---|
| `src/crash_handler.h/.cpp` — `SetUnhandledExceptionFilter` + `MiniDumpWriteDump` 호출 | **사용자** |
| `src/main.cpp` 의 `--crash-test` 분기와 의도적 예외 발생 지점 | **사용자** |
| dump 파일명·출력 디렉터리 규칙 (코드에 있는 쪽) | **사용자** |
| `src/**` 나머지 — 최소 echo 서버 (IOCP 생성·accept·recv·send·종료) | **사용자** |
| `test/**` smoke 테스트 | **사용자** |
| `modern-iocp/README.md` 뼈대, `docs/vs-build.md`, `docs/dump-analysis-00-plumbing.md` | **사용자** |
| `CMakeLists.txt` — 타깃·`DbgHelp.lib` 링크·컴파일/링크 플래그 | Claude |
| `CMakePresets.json` — toolset·SDK 고정, Release-with-PDB preset | Claude |
| `vcpkg.json` + baseline 설정 | Claude |
| `scripts/phase1.ps1` — `build`/`test`/`run`/`crash`/`verify-dump`/`doctor` 래퍼 | Claude |
| `.github/workflows/ci.yml` — 러너 고정·`verify-dump` 실행·artifact 업로드 | Claude |
| `modern-iocp/docs/infra/*.md` — 위 파일들의 해설 | Claude |

🔴 **해설 없는 인프라 파일은 남기지 않는다.** Claude 가 쓴 파일마다 `docs/infra/` 에
"무엇을 왜 그렇게 했는지 · 바꾸려면 어디를 보는지"를 남긴다.

🔴 **경계를 넘는 수정이 필요해지면 이 문서 Implementation log 에 예외로 기록한다.**
조용히 넘어가지 않는다.

## 크래시 계약

느슨하면 S1 이 형식상 통과하고 실제로는 안 되는 상태가 된다 (Codex 리뷰 P2). 아래를 고정한다.

- `--crash-test` 는 **빌드된 실행 파일의 고정된 코드 경로**에서 의도적 예외를 발생시킨다.
  테스트 하네스가 밖에서 프로세스를 죽이는 방식이 아니다
- dump 는 제어된 SEH 경계에서 **정해진 디렉터리·파일명 규칙**으로 기록한다.
  실제 파일명과 심볼 경로 문자열은 `docs/dump-analysis-00-plumbing.md` 에 그대로 적는다
- `docs/vs-build.md` 와 `docs/dump-analysis-00-plumbing.md` 는 **같은 x64 Release-with-PDB
  preset** 을 쓴다. 한쪽이 Debug 로 채워지면 두 문서가 다른 대상을 보게 된다
- 🔴 **S1 이 보장하는 것은 "심볼이 붙은 call stack" 까지다.** Release 최적화로 인라인·프레임
  생략이 일어나므로 변수 값 정확도나 완전한 프레임 복원은 보장 대상이 아니다.
  이 범위 제한을 `docs/dump-analysis-00-plumbing.md` 첫 줄에 적는다

## Acceptance criteria

- [ ] `scripts\phase1.ps1 build` 가 x64 Release · `/WX` 로 exit 0
- [ ] `scripts\phase1.ps1 test` 가 `smoke.echo_roundtrip` 을 포함해 exit 0.
      이 테스트는 서버를 띄우고 바이트열을 보내 **송신한 것과 같은 바이트열을 받는지** 확인한다
- [ ] **`scripts\phase1.ps1 verify-dump` 가 exit 0** — Release 빌드 → 의도적 크래시 →
      dump 생성 확인 → **대응 `.pdb` 존재 확인**까지를 사람 눈이 아니라 exit code 로 판정한다.
      이 항목이 "덤프 plumbing 을 S1 에서 끝냈다"의 유일한 기계적 근거다
- [ ] **CI 가 `verify-dump` 를 실제로 실행**하고, 생성된 `*.dmp` 와 대응 `*.pdb` 를
      artifact 로 업로드한다. green 만으로는 crash 경로가 살아 있다는 증거가 되지 않는다
- [ ] `scripts\phase1.ps1 doctor` 가 VS instance · MSVC toolset · Windows SDK · vcpkg baseline 을
      출력하고, 없으면 **무엇이 부족한지 명시한 메시지로 실패**한다.
      (fresh clone 실패가 환경 탓인지 저장소 탓인지 구분되지 않으면 원커맨드 목표가 무너진다)
- [ ] `modern-iocp/docs/dump-analysis-00-plumbing.md` — 로컬에서 덤프를 WinDbg 또는 VS 로 열어
      **심볼이 붙은 call stack 을 확인한 기록**. 사용한 **정확한 dump 파일명과 symbol path
      문자열**을 고정해 적는다. 첫 줄에 S1 보장 범위(심볼 붙은 stack 까지) 명시.
      근본원인 분석은 하지 않는다 — 의도적 크래시라 원인이 자명하다
- [ ] `modern-iocp/docs/vs-build.md` — Visual Studio 2026 (toolset v145) 로 **`build` 와 같은 x64 Release-with-PDB
      preset** 을 열어 빌드하고 중단점이 걸리는지 확인한 절차. preset 이름 포함
- [ ] GitHub Actions Windows 워크플로가 green. 실행 URL 을 이 문서 Verification 표에 적는다
- [ ] `modern-iocp/README.md` 뼈대가 있다 — **무엇인가 · 무엇을 증명하려는가 · 현재 상태 ·
      빌드 한 줄**. 30줄 내외. 단계마다 갱신한다 (로드맵 대신 이것을 쓴다).
      🔴 **향후 단계 계획은 여기 쓰지 않는다** — phase/task 문서에 남긴다. README 가 계획을
      담기 시작하면 그게 로드맵이 되고, 문서가 구현보다 먼저 커지는 실패로 돌아간다
- [ ] `modern-iocp/docs/infra/` 에 Claude 가 쓴 파일 각각의 해설이 있다
- [ ] **버전 고정이 산문이 아니라 기계가 읽는 파일에 있다** (Codex 리뷰 P2).
      vcpkg baseline → manifest/config, MSVC toolset·Windows SDK → `CMakePresets.json`,
      CI 러너 → `windows-2025` 명시. `README.md` 와 `docs/infra/` 의 서술은
      그 파일들을 가리키기만 한다. "최신" 같은 표현을 쓰지 않는다
      (레거시가 v143 미고정으로 재현 불가가 된 재발 방지)

## Invariants and risks

- **Ownership and lifetime**: S1 의 echo 서버는 세션 1개만 다뤄도 된다. 다중 세션 수명 규칙은
  S2 의 주제이므로 **여기서 미리 설계하지 않는다**
- **Thread affinity and synchronization**: IO 워커 스레드 수를 고정값으로 두고 근거를 주석에 남긴다.
  튜닝은 S4 측정 이후
- **Input and failure boundaries**: N/A — S1 은 프로토콜을 정의하지 않는다. echo 는 바이트열 그대로
- **Shutdown, retry and backpressure**: 종료 순서(stop 신호 → `PostQueuedCompletionStatus` →
  join → 자원 해제)는 S1 에서 만든다. **여기서 안 만들면 이후 모든 테스트가 프로세스를
  죽여서 끝내게 되고 덤프 plumbing 과 충돌한다**
- **Security and data**: N/A — 자격증명·외부 저장소 없음
- **Performance budget**: N/A — S1 은 측정하지 않는다
- 🔴 위험: **echo 서버가 커지는 것.** S2 의 설계를 미리 당겨오면 S1 이 안 끝난다.
  "세션 1개 · 프로토콜 없음 · 스레드 수 고정" 을 넘으면 범위 이탈이다
- 🔴 **가장 가능성 높은 실패 경로** (Codex 판정): "자동으로 검증되는 덤프 생성 증거" 와
  "사람이 디버거로 확인한 증거" 가 섞인 채 진행되는 것. 그러면 둘 다 반쯤만 된 상태로
  통과한다. 방지 장치는 **역할을 문서에서 분리하는 것**이다 —
  `verify-dump`(자동·exit code) / CI artifact(보존) / `docs/dump-analysis-00-plumbing.md`(수동 확인).
  셋 중 하나라도 없으면 S1 은 완료가 아니다
- 위험: fresh clone 에서 VS·Windows SDK 가 없으면 실패 원인이 환경인지 저장소인지 섞인다.
  `doctor` 서브커맨드가 이것을 가른다
- 위험: CI 러너에 vcpkg 캐시가 없으면 빌드가 길어진다. 실패가 아니라 시간 문제이므로
  캐시 최적화는 green 을 본 뒤에 한다

## Plan

1. Claude — `vcpkg.json` · `CMakeLists.txt` · `CMakePresets.json`. 의존성은 테스트 프레임워크 하나만
2. 사용자 — 최소 echo 서버. IOCP 핸들 생성 → accept → recv → 같은 바이트 send → 종료 신호로 정지
3. 사용자 — `smoke.echo_roundtrip` 테스트
4. Claude — `scripts/phase1.ps1` (`doctor` / `build` / `test` / `run` / `crash` 서브커맨드)
5. Claude — Release PDB 생성 설정, `CMakePresets.json` 에 toolset·SDK 고정
6. 사용자 — 크래시 핸들러 + `--crash-test` 경로 + dump 파일명 규칙
7. Claude — `scripts/phase1.ps1 verify-dump` (6 의 파일명 규칙에 맞춘 자동 검증)
8. Claude — `.github/workflows/ci.yml` (러너 고정 · `verify-dump` 실행 · dmp/pdb artifact)
9. 사용자 — `docs/dump-analysis-00-plumbing.md`, `docs/vs-build.md`, `README.md` 뼈대
10. Claude — `docs/infra/` 해설
11. Codex 독립 리뷰 → 차단 발견 해소 → Codex 커밋

2~3 과 4~5 는 의존이 없으므로 순서를 바꿔도 된다. 다만 **6 은 5 이후**여야 하고
(PDB 설정이 있어야 심볼이 붙는다), **7 은 6 이후**여야 한다 (파일명 규칙을 알아야 검증한다).

## Implementation log

- 2026-08-29 문서 작성. 코드 미착수
- 2026-08-29 Codex 리뷰 `Request changes` 반영 — `verify-dump`·`doctor` 서브커맨드 신설,
  크래시 계약 절 추가, 작성자 분담을 파일 경로 단위로 재작성, 버전 고정을 기계 파일로 이동
- 2026-08-30 **경계 예외 2건** (사용자 요청)
  - IOCP 코어의 미완성 부분을 Claude 가 채웠다. `acceptThread` · `workerThread` · `stop` ·
    `closeClient` 와 `postSend` 버그 2건(컨텍스트 오기입, 송신 길이 `kMaxSockBuf`)
  - `docs/dump-analysis-00-plumbing.md` 를 Claude 가 작성했다. 분석 자체(VS·WinDbg 실행,
    함정 3건 발견)는 사용자가 했고 Claude 는 기록만 정리했다
  - 환경·명명 변경: VS 2026 / toolset v145 확정, 프로젝트명 `Core`/`GameServer`/`Tests`,
    디렉터리를 프로젝트별로 분리, 접근자 `get`/`set` 관례 확정
- 2026-08-31 로컬 검증 전부 통과 → 공개 전 민감 내용 정리 후 히스토리 재작성 → 첫 push (`672182e`)
- 2026-08-31 **CI 첫 실행 green.** 러너(`windows-2025`) 실측값이 이 문서의 전제를 뒤집었다 —
  Visual Studio **Enterprise 2026 / 18.9.12112.369**, MSVC **14.51.36231**(로컬과 같은 값),
  CMake 4.4.2. "러너에 VS 2026 이 없을 것" 이라는 전제로 CI preset 이 포기했던 toolset·SDK 고정을
  되살렸다 — `x64-release-ci` 를 `"inherits": "x64-release"` 로 바꾸고 `binaryDir` 만 덮어쓴다.
  워크플로 "툴체인 기록" 단계에 설치된 Windows SDK 목록 출력을 추가했다.
  변경 파일: `modern-iocp/CMakePresets.json` · `.github/workflows/ci.yml` ·
  `modern-iocp/docs/infra/{build-system,ci}.md`
- 2026-08-31 **Codex 리뷰 3회차 반영** (`Request changes`, P1 1건 · P2 2건).
  P1 이 실제 결함이었다 — `CMAKE_SYSTEM_VERSION` 은 SDK 를 고정하지 않는다(`CMP0149` 새 동작).
  격리 재현 후 `architecture` 의 `version=10.0.26100.0` 로 옮기고 무의미한 캐시 변수를 지웠다.
  toolset patch 는 애초에 강제할 수단이 없어(실측) 문서 주장을 실제 강도로 낮췄다.
  상세는 아래 "Review resolution 2".
- 2026-08-31 **경계 예외 3건째** (사용자 요청) — `README.md` 두 곳을 Claude 가 고쳤다:
  "요구 환경" 절의 CI toolset 문단(preset 변경으로 사실과 달라짐)과 상태표의 GitHub Actions 행
  (`미검증` → run #2 링크). 둘 다 내용이 CI 인프라라 같은 diff 로 묶었다.
  README 의 나머지는 사용자 소유 그대로다

## Verification

| Command or check | Result | Evidence/notes |
|---|---|---|
| `scripts\phase1.ps1 doctor` | 통과 | 2026-08-31 exit 0 — cmake 4.3.3 / VS 18 Community / toolset 14.51.36231 / SDK 10.0.26100.0 / vcpkg baseline 일치. `VCPKG_ROOT` 가 없는 셸에서는 exit 1 + 무엇이 없는지 명시하는 것까지 확인 |
| `scripts\phase1.ps1 build` | 통과 | 2026-08-31 exit 0 · 경고 0. **SDK 고정 수정 후 두 preset 다 build 디렉터리를 지우고 재구성** — `Selecting Windows SDK version 10.0.26100.0`, 컴파일러 `MSVC/14.51.36231/bin/Hostx64/x64/cl.exe` |
| `scripts\phase1.ps1 test` | 통과 | `smoke.echo_roundtrip` Passed 0.02s — 두 preset 모두 |
| `scripts\phase1.ps1 verify-dump` | 통과 | exit 0 — 덤프 생성 + 짝 PDB 확인, 두 preset 모두. PDB 를 지우면 exit 1 하는 것도 확인 |
| SDK 고정이 실제로 강제되는가 | 통과 | 빈 프로젝트 격리 실측 — `-A "x64,version=10.0.99999.0"` → `CMake Error` exit 1 / `-DCMAKE_SYSTEM_VERSION=10.0.99999.0` → 조용히 26100 선택 exit 0 (그래서 고정 위치를 옮겼다) |
| toolset patch 고정이 강제되는가 | **아니오** | `-T v145,version=14.51.99999` 로 configure·빌드 모두 exit 0. `14.51` 계열까지만 보장된다 — 문서에 그대로 적었다 |
| 덤프 수동 열기 (WinDbg/VS) | 통과 | `docs/dump-analysis-00-plumbing.md` — `.symfix` → `.sympath+` → `.ecxr` → `k`, 스크린샷 4장. 🔴 `k` 출력 텍스트 자체는 문서에 넣지 않았다 |
| VS 2026 빌드·중단점 | 통과 | `docs/vs-build.md` — `x64-release` preset |
| CI (`verify-dump` 실행 + artifact) | 통과 | run #1 `672182e` [runs/33319684014](https://github.com/ldcity/Modern-IOCP/actions/runs/33319684014) (고정 전) · **run #2 `2f430c0` [runs/33321582508](https://github.com/ldcity/Modern-IOCP/actions/runs/33321582508) (고정 후)** — 둘 다 빌드·테스트·`verify-dump`·artifact 업로드 전 스텝 성공 |
| 고정이 러너에서 실제로 물리는가 | 통과 | run #2 "툴체인 기록" — VS Enterprise 2026 / 18.9.12112.369 · MSVC 14.51.36231 · 설치 SDK `10.0.26100.0`. preset 이 요구하는 조합이 러너에 있고 그것으로 빌드됐다 |

## Handoff to reviewer

- Changed files: S1 본체는 커밋 `672182e` 에 있다. 이번 리뷰 대상은 그 위의 미커밋 diff —
  `modern-iocp/CMakePresets.json` · `.github/workflows/ci.yml` ·
  `modern-iocp/docs/infra/{build-system,ci}.md` · `modern-iocp/README.md`(경계 예외) · 이 문서
- Key decisions: 덤프 plumbing 을 S1 로 당김 / echo 서버를 의도적으로 얇게 유지 /
  로드맵 대신 `README.md` 뼈대를 S1 에서 만들고 단계마다 갱신 / 도구 버전을 처음부터 고정
- Known risks: echo 서버 범위 팽창. S2 설계 선취
- Review focus: S1 완료 조건이 S2 를 침범하지 않는가. 종료 경로를 S1 에 넣은 판단이 맞는가.
  도구 버전 고정이 재현성을 실제로 보장하는가
- Checks not run: 없음. 마지막에 고친 `build-system.md` "바꾸려면" 한 줄(재리뷰 P2)만
  재리뷰를 거치지 않았다 — 문서 한 줄이고 빌드에 영향이 없다

## Review resolution

`.ai/reviews/TASK-20260829-modern-iocp-s1-skeleton-codex.md` (2026-08-29, `Request changes`)

| 발견 | 상태 | 해소 방식 |
|---|---|---|
| P1 덤프 plumbing 의 CI 재현이 기계 판정 불가 | 해소 | `verify-dump` 서브커맨드 + CI 실행 + `.dmp`/`.pdb` artifact 업로드를 완료 조건으로. 문서에 정확한 파일명·심볼 경로 고정 |
| P2 크래시 계약이 느슨함 | 해소 | "크래시 계약" 절 신설 — 고정 코드 경로 · dump 파일명 규칙 · 두 문서가 같은 Release-with-PDB preset · S1 보장 범위는 "심볼 붙은 stack" 까지 |
| P2 작성자 분담이 파일 경계가 아님 | 해소 | 표를 파일 경로 단위로 재작성. 경계 초과 시 Implementation log 에 예외 기록 의무 |
| P2 버전 고정이 산문에만 있음 | 해소 | vcpkg baseline → manifest, toolset·SDK → `CMakePresets.json`, 러너 → 명시 이미지. `doctor` 서브커맨드 추가 |
| (기타) fresh clone 실패 원인 구분 불가 | 해소 | `doctor` 가 부족한 것을 명시해 실패 |
| (기타) README 가 계획을 담을 위험 | 해소 | 향후 단계 계획은 README 에 쓰지 않는다고 명시 |

**A 재검증 부분 해소 2건**은 상위 문서에서 처리했다.
- A-3 D3 재현 한계 → `TASK-...-phase1.md` 에 "D3 이 증명하는 것과 못하는 것" 추가,
  수정 방식을 테스트가 아니라 **타입 차단**으로 변경. 결정 문서의 과대 표현도 수정
- A-8 Baseline 오기 → 공개 히스토리의 새 루트 `main ec16031` 로 수정, 변동성 있는 개수 표기 제거

재리뷰 필요. 다만 S1 코드가 나온 뒤 함께 판정하는 편이 낫다.

## Review resolution 2 — CI toolset 고정 diff

`.ai/reviews/TASK-20260829-modern-iocp-s1-ci-toolset-codex.md` (2026-08-31, `Request changes`)

| 발견 | 상태 | 해소 방식 |
|---|---|---|
| **P1** `CMAKE_SYSTEM_VERSION` 은 SDK 를 고정하지 못한다 (`CMP0149` 새 동작) | 해소 | 격리 프로젝트로 재현 확인 후 `architecture` 를 `x64,version=10.0.26100.0` 으로 바꾸고 `CMAKE_SYSTEM_VERSION` 삭제. 실측표를 `docs/infra/build-system.md` 에 남겼다 |
| **P2** toolset 은 `14.51` 계열까지만 고정되는데 문서는 patch 동일성을 주장 | 해소 | patch 고정에는 **강제력이 없다**(실측: `version=14.51.99999` 로 configure·빌드 모두 exit 0). 그래서 exact 고정 대신 문서 주장을 실제 강도로 낮췄다 — README·build-system 둘 다 |
| **P2** 완료 상태와 CI 검증 상태를 문서가 다르게 말함 | 해소 | Status 를 "고정 diff 는 로컬 검증만, CI run #2 남음"으로 낮추고 README 상태표의 "GitHub Actions 미검증"을 run #1 링크로 갱신 |
| (리뷰어 제안) 선택된 SDK 를 CI 로그에 남길 것 | 미채택 | SDK 는 이제 없으면 configure 가 실패하므로 **silent fallback 경로 자체가 없다** — 로그로 메울 correctness 공백이 아니다. (재리뷰 지적: configure 출력의 `cl.exe` 경로는 toolset 증거이지 SDK 증거가 아니다. 맞는 지적이라 근거에서 뺐다.) 관측성 강화는 필요해지면 그때 넣는다 |

리뷰어가 확인한 것 중 유지된 판단: `inherits` 로 로컬/CI 를 묶은 것(부작용 없음),
`binaryDir` 만 덮어쓰는 것, 워크플로의 SDK 목록 출력.

### 재리뷰 (2회차)

`.ai/reviews/TASK-20260829-modern-iocp-s1-ci-toolset-codex-r2.md` (2026-08-31, `Request changes`,
**차단 없음**). 1회차 발견 3건 전부 `해소` 판정.

| 발견 | 상태 | 해소 방식 |
|---|---|---|
| **P2** `build-system.md` "바꾸려면" 절이 아직 `CMAKE_SYSTEM_VERSION` 을 가리킨다 | 해소 | `architecture` 의 `version=` 으로 고치고 되돌리지 말라는 경고를 붙였다. 다음 SDK 변경 때 같은 결함이 재도입되는 경로였다 |
| (지적) 미채택 사유의 근거 하나가 부정확 | 해소 | `cl.exe` 경로는 toolset 증거이지 SDK 증거가 아니다. 위 표에서 그 근거를 뺐다 |

리뷰어가 독립 확인한 것: `CMAKE_SYSTEM_VERSION` 삭제 부작용 없음
(`_WIN32_WINNT`/`WINVER`/`NTDDI` 의존이 저장소에 없다), preset 의 `architecture` 가
`-A` 와 같은 경로로 전달되고 `x64-release-ci` 도 상속으로 같은 보장을 받는다.

그 위험(고정된 preset 의 CI 미검증)은 2026-08-31 run #2 green 으로 해소됐다 — 위 Verification 표.
