# TASK-20260829-modern-iocp-phase1: Phase 1 — raw Win32 IOCP 네트워크 라이브러리

- Status: Draft — 사용자 승인 대기 (승인 전 코드·CMake 를 만들지 않는다)
- Implementer: **사용자** (하네스 기본값 교체 — 아래 "역할 경계" 참조)
- Advisor: Claude (인프라 작성 + 설계 상대·조사. **네트워크 코어 구현 대행 아님**)
- Reviewer: Codex
- Repositories: `D:\GameProjects\Portfolio`
- Baseline: `main` `ec16031` (2026-08-31 공개 히스토리 새 루트). 변동성 있는 worktree 개수는 적지 않는다
- Owned paths: `.ai/tasks/TASK-20260829-modern-iocp-phase1.md`
  (**이 문서는 Phase 정의이지 실행 task 가 아니다.** `modern-iocp/**` 는 활성 S 단계 task 가 소유한다)

범위와 스택의 근거는 `.ai/decisions/20260829-modern-iocp-scope.md` 다. 이 문서는 그 결정을
반복하지 않고 **Phase 1 의 완료 조건과 작업 순서**만 고정한다.

## 역할 경계

소유자가 설명할 수 없는 코드는 포트폴리오로서 값을 하지 못한다. 그것이 이 저장소를 별도로 둔
전제이므로 **면접에서 질문받을 수 있는 것은 전부 소유자가 쓴다.** `.ai/README.md` 역할 표의 "사용자가 특정 task 에서 명시한
경우에만 역할을 교체한다" 조항으로 교체한다 (사용자 결정 2026-08-29).

| 경로 | 작성자 | 근거 |
|---|---|---|
| `modern-iocp/src/**`, `include/**` | **사용자** | 네트워크 코어. 이 저장소의 존재 이유 |
| `modern-iocp/test/**` | **사용자** | 결함 재현·회귀 테스트가 핵심 증거다 |
| `modern-iocp/bench/**` | **사용자** | 부하 모델 설계가 곧 성능 이해의 증거 |
| `docs/dump-analysis-*.md` | **사용자** | 덤프 분석은 운영 역량의 직접 증거 |
| `README.md`, `EXPLAIN.md` | **사용자** | 포트폴리오의 얼굴이자 이해도의 증거 |
| `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json` | Claude | 빌드 인프라 |
| `.github/workflows/**` | Claude | CI |
| `docker-compose*.yml`, DB/Redis 초기화 스크립트 | Claude | 실행 환경 |
| `scripts/**` (빌드·실행 래퍼) | Claude | 원커맨드 편의 |
| `.ai/**` | Claude | 하네스 문서 |

🔴 **Claude 가 쓴 인프라에는 해설 의무가 붙는다.** `modern-iocp/docs/infra/` 에 각 인프라
파일이 **무엇을 왜 그렇게 하는지** 사용자가 읽고 이해할 수 있는 수준으로 남긴다. 해설이
없는 인프라 파일은 이 저장소에 남기지 않는다 (사용자 결정 2026-08-29).

경계를 옮기려면 이 표를 고치고 그 이유를 적는다. 암묵적으로 넘어가지 않는다.

## Problem evidence

레거시 `MMOFighter_IOCP_Server` 에서 감사가 지목한 결함의 앵커를 2026-08-29 에 실측 확인했다
(저장소 `a2ede8c`, 경로는 `portfolio-legacy/MMOFighter_IOCP_Server/MMOFighter_IOCP_Server/`).
줄 번호는 부패하므로 심볼명으로 찾는다.

| ID | 결함 | 위치 | 확인된 사실 |
|---|---|---|---|
| D1 | 부분 송신 미처리 | `NetServer.cpp` `NetServer::SendProc()` | completion 의 `cbTransferred` 를 소비하지 않고 WSABUF 에 넣은 전량을 전송된 것으로 계산 후 해제. 부분 완료 시 패킷 뒷부분 유실 |
| D2 | 성공 경로 반환 누락 | `NetServer.cpp` `NetServer::SendPacket()` · `SendPacketAndDisconnect()` | 마지막 성공 경로에 `return true` 없음 |
| D3 | interlocked 폭 불일치 | `NetServer.cpp` accept 실패 경로 ↔ `Session.h` `stSESSION::_ioRefCount` | 카운터는 `DWORD`(32비트)인데 한 곳만 `InterlockedDecrement64((LONG64*)&...)` |
| D4 | zero-length payload 미처리 | `NetServer.cpp` 수신 루프 | `useSize <= sizeof(WanHeader)` 조건이라 payload 0 패킷은 헤더가 다 와도 처리되지 않는다 |

**레거시는 현재 빌드 가능 여부가 미확인이다** (v143 toolset 부재로 감사 시점 재빌드 실패).
따라서 레거시에서 실행해 재현하지 않는다.

## 결함 재현 계약

레거시 결함을 신규 코드에 그대로 옮겨 심는 방식은 D2·D3 에서 비결정적이라 가짜 PASS 를
만든다 (Codex 리뷰 P1). 결함별로 **무엇을 주입해 무엇이 FAIL 하는지**를 먼저 고정한다.

| ID | legacy symptom | injected behavior | expected FAIL | fix |
|---|---|---|---|---|
| D1 | 부분 송신 시 뒷부분 유실 | transport seam 이 요청보다 짧은 `cbTransferred` 를 강제로 반환 | 수신측 바이트열이 송신측과 불일치 | 송신 커서 유지 + 남은 WSABUF 재구성 |
| D2 | 성공 경로 반환값 미정의 | UB 를 재현하지 않는다. 성공 경로가 `false` 를 반환하도록 fault injection | 호출자가 성공을 실패로 처리해 세션이 조기 종료 | 반환 계약을 타입으로 강제(`enum class SendResult` 등) |
| D3 | 32비트 카운터에 64비트 원자 연산 | 카운터 앞뒤에 canary 를 둔 희생 구조체에 폭 불일치 연산 수행 | canary 오염 검출 | **타입 차원 차단** — `std::atomic<uint32_t>` 로 두어 64비트 연산이 애초에 컴파일되지 않게 한다 |
| D4 | payload 0 패킷 미처리 | 없음. 결함 주입 없이 payload 0 패킷을 보내는 테스트를 먼저 작성 | 수신 핸들러가 호출되지 않음 | 경계 조건을 `< ` 가 아닌 정확한 비교로 수정 |

D1 의 transport seam 은 **테스트 전용 주입점**이지 프로덕션 추상화가 아니다. 구현체가
2개(실 소켓 / 테스트 seam)이므로 `CONSTITUTION.md` 의 DIP 조건을 만족한다.

🔴 **D3 이 증명하는 것과 증명하지 못하는 것** (Codex 재검증 A-3 부분 해소):
canary 테스트는 "32비트 카운터에 64비트 원자 연산을 하면 인접 메모리가 오염된다"는
**메커니즘**을 보인다. 레거시 세션 객체에서 실제로 무엇이 오염됐는지는 재현하지 않는다 —
그건 메모리 레이아웃과 타이밍에 달려 있어 재현해도 flaky 하다. 따라서 D3 의 방어선은
테스트가 아니라 **타입**이다. 카운터를 `std::atomic<uint32_t>` 로 두면 폭이 다른 연산은
컴파일되지 않으므로 결함이 표현 불가능해진다. 테스트는 그 판단의 근거를 남기는 용도다.
이 한계를 `docs/regressions/D3.md` 에 그대로 적는다.

## Goal

`modern-iocp/` 에 raw Win32 IOCP 기반 네트워크 라이브러리와 최소 서버가 있고, 리뷰어가
저장소를 clone 해 **한 명령으로 빌드·실행·테스트**할 수 있다. 결함 4종은 FAIL-first 기록을
남긴 뒤 수정돼 있고, 부하 측정치·크래시 덤프 분석·운영 관측 artifact 가 문서로 남아 있다.

**Phase 1 은 최종 정지점으로 설계한다.** 여기서 멈춰도 포트폴리오로 성립해야 한다.

## Non-goals

- Phase 2(Asio/코루틴) · Phase 3(IOCP 코루틴 어웨이터) — Phase 1 릴리즈 태그 전 착수 금지
- 게임 로직(이동·공격·섹터 전파) 재현
- 레거시 5종 저장소 수정. `legacy` 로 표시만 한다
- Linux 이식
- 레거시 코드 복사. 기법만 가져오고 새로 작성한다
- **lock-free 자료구조 선제 도입.** 측정된 병목이 나온 뒤에만 검토한다 (`CONSTITUTION.md`)

## Acceptance criteria

각 항목은 **명령 또는 산출물 경로**로 판정한다. 사람의 인상은 판정 근거가 아니다.

- [ ] `scripts\phase1.ps1 build` 가 x64 Release · `/WX` 로 exit 0
- [ ] `ctest -L regression` 이 `regr.partial_send` · `regr.send_result` · `regr.iorefcount_width` ·
      `regr.zero_length_payload` 4개를 포함해 통과
- [ ] `modern-iocp/docs/regressions/<ID>.md` 4개가 각각 **주입 내용 · FAIL 출력 원문 · 수정 커밋**을 담는다
- [ ] `modern-iocp/docs/vs-build.md` — Visual Studio 2026 (toolset v145) 로 열어 빌드하고 중단점 디버그한 절차
      (preset 이름 또는 sln 경로 포함)
- [ ] `scripts\phase1.ps1 up` 이 MySQL/Redis 를 올리고 `ctest -L e2e` 가 인증 · 세션 TTL ·
      중복 로그인 · 재시도 멱등성을 통과
- [ ] `modern-iocp/docs/bench/<commit-hash>.md` — 하드웨어 · 빌드 설정 · 부하 모델 · p50/p95/p99.
      부하가 실제로 걸렸는지 확인하는 **유효성 게이트가 있고, 미달 시 PASS/FAIL 이 아니라 INVALID**
- [ ] `modern-iocp/docs/ops-observability.md` — 노출한 카운터 목록과 실제 캡처 1건
- [ ] `modern-iocp/docs/dump-analysis-01.md` — 재현 명령 · 덤프 파일명 · 심볼 경로 · stack ·
      exception · 근본원인 · 패치 커밋 · 회귀 테스트 이름
- [ ] GitHub Actions Windows 워크플로가 green
- [ ] `modern-iocp/README.md` 에 SDK/toolset/MySQL/Redis **버전이 고정**돼 있고 원커맨드 절차가 있다.
      뼈대는 S1 에서 만들고 단계마다 갱신한다 — 별도 ROADMAP 문서를 만들지 않는다
      (사용자 결정 2026-08-29: 로드맵을 새로 쓰면 `CONSTITUTION`·결정·task 문서와 3중 중복이 된다)
- [ ] `modern-iocp/docs/infra/` 에 Claude 가 쓴 인프라 파일별 해설이 있다 (역할 경계 조항)
- [ ] **`modern-iocp/EXPLAIN.md` 를 사용자가 직접 쓴다** — IOCP 완료 통지, IO 참조 카운트 수명,
      부분 송신 처리를 코드를 보지 않고 설명한 문서. 이 저장소의 존재 이유이므로 다른 조건이
      다 통과해도 이것이 없으면 완료가 아니다
- [ ] `.ai/reviews/` 의 최신 Codex verdict 가 `Approve` 또는 `Approve with changes` 이고
      미해결 P1/P2 가 0

## Invariants and risks

- **Ownership and lifetime**: 소켓·버퍼·세션·OVERLAPPED context 의 소유자와 해제 시점을 타입으로
  드러낸다. IO 참조 카운트가 0 이 되는 지점은 한 곳뿐이어야 한다
- **Thread affinity and synchronization**: IO 워커가 만지는 상태와 accept/타이머 스레드가 만지는
  상태를 분리한다. 원자 연산 폭을 카운터 타입과 일치시킨다 (D3)
- **Input and failure boundaries**: 패킷 길이·ID·enum·배열 인덱스를 사용 전에 검증한다.
  payload 0 을 정상 입력으로 취급한다 (D4)
- **Shutdown, retry and backpressure**: stop 신호 → 대기 해제 → join → 자원 해제 순서.
  송신 큐에 용량과 과부하 정책을 정의하고 slow client 를 측정 대상으로 둔다
- **Security and data**: DB/Redis 자격증명은 예제 설정과 런타임 주입을 분리한다. 커밋 금지
- **Performance budget**: Phase 1 에서는 절대 수치 목표를 두지 않는다. 기준선을 만드는 것이
  목적이고, 목표치는 Phase 2/3 비교 시점에 정한다
- 🔴 **역할 경계 침식**: 인프라와 증거의 경계가 무너지면 이 저장소의 주장이 무너진다.
  위 표를 고치지 않은 채 Claude 가 표 밖의 파일을 쓰면 그것은 위반이다

## Plan

각 단계는 별도 task 문서로 쪼갠다. 이 문서는 순서와 게이트만 고정한다.
순서는 Codex 리뷰 P2 를 반영해 **덤프 plumbing 을 앞으로, 부하 baseline 을 DB 앞으로** 당겼다.

1. **S1 골격 + 덤프 plumbing** — CMake/vcpkg/CI/원커맨드, PDB·심볼 경로·의도적 크래시 skeleton,
   `README.md` 뼈대. 코드는 최소 echo 서버 하나. 덤프가 나오고 열리는 것을 여기서 확인한다.
   → [`TASK-20260829-modern-iocp-s1-skeleton.md`](TASK-20260829-modern-iocp-s1-skeleton.md)
2. **S2 세션 수명** — RAII 소켓/핸들, `std::atomic` IO 참조 카운트, operation 별 OVERLAPPED
   context. **D3 회귀 테스트**
3. **S3 송수신 경로** — 송신 커서, WSABUF 재구성, 수신 경계 조건. **D1·D2·D4 회귀 테스트**
4. **S4 transport-only 부하 baseline** — DB 없이 네트워크 코어만. 동접·packet rate·slow client·
   백프레셔. 기준선이 스토리지와 섞이지 않게 여기서 한 번 잰다
5. **S5 인증·저장소** — MySQL/Redis + Compose, 세션 TTL·중복 로그인·재시도 멱등성, e2e 시나리오
6. **S6 덤프 분석 + 운영 관측 + 릴리즈** — 덤프 분석 문서, 카운터 캡처, README·EXPLAIN, 태그

게이트: S1~S6 완료 전에는 Phase 2/3 를 시작하지 않는다. 구조적 장치는 `.ai/tasks/INDEX.md` 에 있다.

## Implementation log

- 2026-08-29 범위·스택 결정 (`.ai/decisions/20260829-modern-iocp-scope.md`). 코드 미착수
- 2026-08-29 Codex 리뷰 `Request changes` 반영 — 완료 조건을 명령·산출물로 치환, 역할 경계 표
  확정, 결함 재현 계약 신설, D4 를 회귀 필수로 승격, S1~S6 순서 조정, VS·운영 관측 항목 추가

## Verification

| Command or check | Result | Evidence/notes |
|---|---|---|
| 레거시 결함 앵커 존재 확인 | 확인 | `git ls-files` + `grep` 실측, `MMOFighter_IOCP_Server` `a2ede8c` |
| Codex 독립 리뷰 (문서) | Request changes | `.ai/reviews/TASK-20260829-modern-iocp-phase1-codex.md` |
| 빌드 | 미실행 | `modern-iocp/` 가 비어 있다 |
| 테스트 | 미실행 | 동일 |

## Handoff to reviewer

- Changed files: `.ai/decisions/20260829-modern-iocp-scope.md`, 이 문서, `.ai/tasks/INDEX.md`
- Key decisions: 범위 = IOCP Reliability Lab / 스택 = raw Win32 IOCP 부터 단계적 진화 /
  Phase 1 을 최종 정지점으로 설계 / 인프라는 Claude + 해설 의무, 증거는 사용자
- Known risks: 역할 경계 침식. Phase 게이트 실효성
- Review focus: 완료 조건이 이제 기계적으로 판정 가능한가. 결함 재현 계약이 실제 결함을
  재현하는가. Phase 1 단독 종료가 성립하는가
- Checks not run: 빌드·테스트 전부. 코드가 없다

## Review resolution

`.ai/reviews/TASK-20260829-modern-iocp-phase1-codex.md` (2026-08-29, `Request changes`)

| 발견 | 상태 | 해소 방식 |
|---|---|---|
| P1 완료 조건 검증 불가 | 해소 | Acceptance criteria 를 명령·산출물 경로로 전면 치환. `EXPLAIN.md` 도입 |
| P1 AI 경계 미확정 | 해소 | 역할 경계 표 확정 (사용자 결정: 인프라 Claude + 해설 의무) |
| P1 가짜 PASS 위험 | 해소 | "결함 재현 계약" 절 신설. D2 를 UB 재현에서 fault injection 으로 교체 |
| P2 Phase 1 종료점 결손 | 해소 | `docs/vs-build.md` · `docs/ops-observability.md` 를 완료 조건에 추가 |
| P2 D4 누락 | 해소 | D4 를 4번째 회귀 필수 항목으로 승격 |
| P2 Phase 게이트 약함 | 해소 | `.ai/tasks/INDEX.md` 에 구조적 제한 추가 |
| P2 S1~S6 순서 | 해소 | 덤프 plumbing → S1, transport-only baseline → S4 로 이동 |
| P3 untracked 개수 오기 | 해소 | Baseline 에 실제 기준 커밋을 명시하고 변동성 있는 개수 제거 |

재리뷰 필요. 위 반영이 실제로 검증 가능한지 Codex 가 다시 판정해야 한다.
