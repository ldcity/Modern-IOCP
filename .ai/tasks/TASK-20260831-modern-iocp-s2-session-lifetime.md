# TASK-20260831-modern-iocp-s2-session-lifetime: S2 — 세션 수명

- Status: **Approved** (2026-08-31, 사용자) — 구현 착수 가능. 코드 미착수.
  Codex 리뷰 1회차(`Request changes`) 반영 완료 → 아래 "Review resolution".
  재리뷰는 S2 코드가 나온 뒤 함께 받는다
- Implementer: **사용자** (코드) + Claude (빌드·CI·스크립트 인프라). 아래 "작성자 분담" 참조
- Reviewer: Codex
- Repositories: `D:\GameProjects\Portfolio`
- Baseline: `main` `273430c` (S1 완료 커밋)
- Owned paths: `modern-iocp/**`, `.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md`

Phase 1 전체 정의는 [`TASK-20260829-modern-iocp-phase1.md`](TASK-20260829-modern-iocp-phase1.md) 다.
이 문서는 그 6단계 중 **S2 만** 다룬다. 역할 경계·결함 재현 계약을 여기서 반복하지 않는다.

## 범위 결정 (2026-08-31, 사용자)

**S2 는 다중 세션까지 간다.** 세션 객체와 컨테이너를 여기서 도입한다.

근거: IO 참조 카운트의 존재 이유는 "워커가 완료를 처리하는 동안 다른 스레드가 세션을 닫는"
경합이다. 세션이 1개면 그 경합을 만들 수 없어 **참조 카운트가 필요하다는 것 자체를
증명하지 못한다.** 또 S4(transport-only 부하 baseline)가 동접을 전제하므로 다중 세션은
Phase 1 안에서 반드시 생긴다. S3(송수신 경로)로 미루면 S3 가 "세션 구조 + 송수신" 두 주제를
갖게 되므로 여기서 끝낸다.

대가: S2 가 Phase 1 에서 가장 큰 단계가 된다. 그래서 Non-goals 를 아래에 좁게 못박는다.

## Problem evidence

S1 의 echo 서버(`Core/IOCPServer.{h,cpp}`, `273430c`)는 **세션 1개를 서버 객체의 멤버로 편 채**
만들어졌다. S1 의 의도된 범위였고 그 자체는 결함이 아니다. 아래는 다중 세션으로 가는 순간
전부 실제 결함이 되는 지점들이며, 심볼명으로 적는다(줄 번호는 부패한다).

| ID | 지점 | 확인된 사실 |
|---|---|---|
| L1 | `IOCPServer::acceptThread()` ↔ `IOCPServer::closeClient()` | `m_clientSocket` 을 accept 스레드가 대입하고 워커·`stop()` 이 지운다. 동기화가 없다. 세션 1개일 때는 실질 문제가 없으나 수명 장치가 없다는 증거다 |
| L2 | `IOCPServer::start()` → `IOCPServer::initSocket()` · `CreateIoCompletionPort` | `WSAStartup()` 성공 후 `WSASocketW()`·`bind`·`listen`·**`CreateIoCompletionPort()`** 중 하나가 실패하면 `start()` 가 곧장 `false` 를 반환한다. 이때 `m_running` 이 아직 `false` 라 `stop()` 이 즉시 return 하므로 **`WSACleanup()` 도 `closesocket()` 도 호출되지 않는다.** 실패 지점이 뒤로 갈수록 새는 자원이 늘어난다 (Codex 리뷰 지적으로 범위 확대) |
| L3 | `IOCPServer` 멤버 `m_recvContext` · `m_sendContext` | OVERLAPPED context 가 **서버당 1쌍**이다. 세션이 둘이면 서로의 버퍼를 덮어쓴다 |
| L4 | `IOCPServer::acceptThread()` 의 `m_clientSocket = clientSocket;` | 두 번째 클라이언트가 붙으면 이전 소켓 핸들이 덮어써져 **누수**되고, 진행 중이던 그 소켓의 완료는 갈 곳을 잃는다 |
| L5 | `IOCPServer::workerThread()` 의 `!bSuccess` / `0 == transferredBytes` 경로 | 완료가 **어느 세션의 것인지 판정하지 않고** `closeClient()` 를 부른다. completionKey 로 `this` 를 넘기고 있어 세션 식별자가 없다 |
| L6 | `IOCPServer::acceptThread()` 의 `accept()` | 블로킹 accept 를 전용 스레드가 돈다. 종료를 위해 `stop()` 이 listen 소켓을 닫아 깨우는 방식이라 종료 경로가 소켓 상태에 의존한다 |

D3(interlocked 폭 불일치)의 재현 계약과 그 한계는 phase1 문서의 "결함 재현 계약" 절에 있다.
**방어선은 테스트가 아니라 타입**(`std::atomic<uint32_t>`)이라는 결론을 여기서 뒤집지 않는다.

## Goal

서버가 **동시에 여러 세션**을 다루고, 각 세션의 소켓·버퍼·OVERLAPPED context 의 소유자와
해제 시점이 타입으로 드러난다. **IO 참조 카운트가 0 이 되는 지점은 한 곳뿐이고, 세션 해제는
그 지점에서만 일어난다.** `start()` 가 중간에 실패해도 잡은 자원을 전부 되돌린다.
accept 는 AcceptEx 로 IOCP 에 올리고, 미완료 accept 를 남기지 않고 종료한다.

🔴 **의도적으로 낮춘 목표** (Codex 리뷰 P1-2, 사용자 결정 2026-08-31): 초안은
"워커가 완료를 처리하는 도중에 다른 스레드가 그 세션을 닫아도 **해제된 메모리를 만지지
않는다**" 였다. 그것을 이 범위에서 기계적으로 증명할 수단이 없어(아래 "판정 장치") 목표를
**"참조 카운트가 0 이 되기 전에는 해제되지 않는다"** 는 구조적 보장으로 낮췄다.
D3 에서 "방어선은 테스트가 아니라 타입"이라고 낸 결론과 같은 형태다 —
증명할 수 없는 것을 완료 조건에 넣지 않는다.

## Non-goals

S2 가 Phase 1 최대 단계이므로 아래를 **명시적으로 제외**한다. 넘으면 범위 이탈이다.

- **D1·D2·D4 회귀 테스트와 송신 커서·WSABUF 재구성·수신 경계 조건** — S3.
  `postSend()` 의 `TODO(S3)` 주석은 그대로 둔다
- **프로토콜·패킷 헤더 정의** — S3. S2 의 echo 는 여전히 바이트열 그대로다
- **부하·백프레셔·동접 측정** — S4. 다중 세션을 만들되 **성능을 재지 않는다**
- 송신 큐·용량 정책 — S4 에서 백프레셔와 함께
- MySQL/Redis·인증 — S5
- lock-free 자료구조. 세션 컨테이너는 가장 지루한 것으로 시작한다.
  측정된 병목이 나온 뒤에만 검토한다 (`CONSTITUTION.md`)
- 세션 풀·객체 재사용 최적화. 측정 전 도입 금지 (같은 이유)
- 워커 스레드 수 튜닝 — S4 측정 이후. `kWorkerThreadCount` 는 고정값 유지
- 덤프 분석 문서 `docs/dump-analysis-01.md` — S6
- **프로덕션 코드에 테스트 전용 동기화 훅** — 아래 "판정 장치" 참조. 루트
  `CLAUDE.md` §6 금지 조항을 이 task 에서 예외로 두지 않는다 (사용자 결정 2026-08-31)

## 작성자 분담

Phase 1 역할 경계표를 이 단계에 적용한 결과다. 경계를 옮기려면 phase1 문서의 표를 고친다.
개념이 아니라 **파일 경로 단위**로 나눈다.

| 파일 | 작성자 |
|---|---|
| `Core/Session.{h,cpp}` (또는 사용자가 정한 세션 타입 파일) | **사용자** |
| `Core/IOCPServer.{h,cpp}` 의 모든 변경 — 세션 컨테이너·AcceptEx·완료 라우팅·종료 순서 | **사용자** |
| RAII 래퍼(소켓·IOCP 핸들·WSA 초기화)의 설계와 구현 | **사용자** |
| IO 참조 카운트 타입과 증감 지점, 자원 수지 카운터 | **사용자** |
| `Tests/**` 전부 — `smoke.*` 추가분과 `regr.iorefcount_width` | **사용자** |
| `docs/regressions/D3.md` | **사용자** |
| `README.md` 갱신 (현재 상태 행) | **사용자** |
| `CMakeLists.txt` — 새 소스 등록, `smoke`/`regression` 라벨 분리 | Claude |
| `scripts/phase1.ps1` — 라벨별 테스트 실행 경로 | Claude |
| `.github/workflows/ci.yml` — `ctest -L regression` 을 CI 에서 실제로 실행 | Claude |
| `docs/infra/*.md` 갱신 — 위 변경의 해설 | Claude |

🔴 **해설 없는 인프라 파일은 남기지 않는다.**
🔴 **경계를 넘는 수정이 필요해지면 이 문서 Implementation log 에 예외로 기록한다.**

## 판정 장치 — 무엇을 어떻게 판정하는가

use-after-free 는 "안 터졌다"로 판정할 수 없다. 느슨하게 두면 S2 가 형식상 통과하고 실제로는
안 되는 상태가 된다(S1 의 크래시 계약과 같은 위험). 아래를 고정한다.

**1. 자원 수지를 서버가 노출한다.** 세션에만이 아니라 **RAII 로 감싼 자원 종류별로**
acquire 수와 release 수를 카운터로 둔다 — 최소한 (a) WSA 초기화, (b) 소켓, (c) 세션,
(d) pending accept. 테스트가 종료 후 **각 짝의 수가 같은지**를 exit code 로 판정한다.
누수와 이중 해제가 여기서 갈린다.

🔴 **왜 "재시도 성공" 이 아니라 수지인가** (Codex 리뷰 P1-1): 초안은 L2 를
"실패 후 같은 프로세스에서 `start()` 재시도가 성공한다"로 판정하려 했다. **그것은 L2 를
잡지 못한다.** `WSAStartup`/`WSACleanup` 은 프로세스 내 참조 카운트이므로 `WSACleanup()` 이
누락돼도 다음 `WSAStartup()` 은 성공하고, 누수된 listen 소켓도 재시도가 새 소켓을 만들면
방해하지 않는다. 그래서 판정을 **짝맞춤 수지**로 바꿨다.

**2. 참조 카운트 0 도달 지점이 한 곳임을 코드로 보장한다.** 해제는 그 지점에서만 일어나고,
세션 파괴 직전에 참조 카운트가 0 임을 코드가 확인한다.

🔴 **3. 이 범위에서 검출되지 않는 것** (Codex 리뷰 P1-2). 위 장치들은 **누수와 이중 해제**를
잡는다. **이미 해제된 세션을 워커가 읽었지만 크래시하지 않은 경우는 잡지 못한다** — 수지가
맞아도 통과한다. 그것을 결정론적으로 잡으려면 프로덕션 코드에 테스트 전용 동기화 훅을
넣어야 하고, 그것은 루트 `CLAUDE.md` §6("프로덕션 코드 구조를 테스트 편의로 바꾸지 않는다")
위반이다. phase1 의 D1 transport seam 은 구현체가 2개라 예외가 성립했지만 이 훅은 1개다.
**따라서 이 창은 테스트가 아니라 구조(해제 지점 단일화)가 막고, 문서는 그 한계를 인정한다.**
이 판단을 `docs/regressions/D3.md` 에 그대로 적는다. ASan(`/fsanitize=address`)도 같은 이유로
이번 범위에서 쓰지 않는다 — IOCP 와의 조합이 이 머신에서 미검증이고 검증 비용이 S2 본체를
넘을 위험이 있다. 도입한다면 별도 결정 문서를 연다.

**4. 경합 테스트는 확률적이다.** 접속·해제를 반복해 경합 창을 넓히는 방식이라 1회 PASS 가
무결성의 증명이 아니다. 반복 횟수를 테스트 소스에 상수로 고정하고, **이 한계도 위 문서에
적는다.** 결정론적 보장은 타입과 소유권 구조가 준다.

## Acceptance criteria

각 항목은 **명령 또는 산출물 경로**로 판정한다. 사람의 인상은 판정 근거가 아니다.

- [ ] `scripts\phase1.ps1 build` 가 x64 Release · `/WX` 로 exit 0, 경고 0
- [ ] `ctest -L smoke` 가 기존 `smoke.echo_roundtrip` 을 **여전히** 포함해 통과 (회귀 없음)
- [ ] `smoke.multi_session_echo` — **동시에 8개 이상**의 세션이 각각 서로 다른 바이트열을
      왕복하고, 어느 세션도 다른 세션의 바이트를 받지 않는다
- [ ] `smoke.resource_balance` — 다중 세션 시나리오를 반복 실행한 뒤 **자원 종류별 수지가
      전부 0** 이다: WSA 초기화 · 소켓 · 세션 · pending accept 각각의 acquire 수 == release 수.
      반복 횟수는 테스트 소스에 상수로 고정한다
- [ ] `smoke.close_race` — 워커가 완료를 처리하는 동안 다른 스레드가 같은 세션에 종료를
      요청하는 시나리오를 반복한다. 크래시 없이 종료하고 위 수지가 맞는다.
      **이 테스트가 보장하는 범위는 "판정 장치" 3항에 적은 대로다**
- [ ] `smoke.start_failure_balance` — `start()` 를 **실패 지점별로** 실패시킨 뒤
      (최소 bind 실패 · IOCP 생성 실패) **자원 수지가 0 으로 돌아온다.**
      "재시도가 성공한다"는 판정으로 대체하지 않는다 (Codex P1-1)
- [ ] `smoke.stop_without_client` — **클라이언트가 한 번도 붙지 않은 상태에서 `start()` 직후
      `stop()` 을 불러도 반환한다**(무한 대기 없음). 종료 후 pending accept 수지가 0 이다.
      🔴 AcceptEx 로 바뀌면 미완료 accept 가 남는데, **pending accept 는 세션으로 승격되지
      않아 세션 카운터에 들어가지 않는다** — 별도 조건이 없으면 누수가 통과한다 (Codex P1-3)
- [ ] `ctest -L regression` 이 **`regr.iorefcount_width`** 를 포함해 통과.
      내용은 phase1 "결함 재현 계약" 의 D3 행 그대로 — canary 를 둔 희생 구조체에
      폭 불일치 연산을 하면 오염이 검출된다
- [ ] `modern-iocp/docs/regressions/D3.md` — **주입 내용 · FAIL 출력 원문 · 수정 커밋** 세 가지를
      담고, phase1 이 정한 **D3 의 재현 한계**와 위 "판정 장치" 3·4 항의 한계
      (use-after-read 창 미검출 · 경합 테스트의 확률성)를 함께 적는다
- [ ] **`grep -rn "\baccept(" Core/` 가 비어 있고, 동시에 `grep -rn "AcceptEx" Core/` 가
      비어 있지 않다** — 블로킹 accept 가 사라졌다는 negative check 와 AcceptEx 로 갔다는
      positive check 를 짝으로 둔다. 한쪽만으로는 헬퍼 파일로 옮겨도 통과한다 (Codex P3)
- [ ] IO 참조 카운트의 타입이 **`std::atomic<uint32_t>`** 다 (phase1 D3 결정. 폭이 다른 원자
      연산이 컴파일되지 않는 것이 방어선이다)
- [ ] `CMakeLists.txt` 가 `smoke` 와 `regression` 라벨을 **분리 등록**하고,
      `ctest -L smoke` 와 `ctest -L regression` 이 서로 다른 집합을 실행한다
- [ ] **CI 가 `ctest -L regression` 을 실제로 실행**하고 green. 라벨이 CI 에서 돌지 않으면
      회귀 테스트는 로컬 관습일 뿐이다
- [ ] `scripts\phase1.ps1 verify-dump` 가 여전히 exit 0 (S1 덤프 plumbing 회귀 없음)
- [ ] `modern-iocp/docs/infra/` 의 해당 문서가 CMake 라벨 분리와 CI 변경을 반영한다
- [ ] `modern-iocp/README.md` 상태표가 S2 를 반영한다. **향후 단계 계획은 쓰지 않는다**
- [ ] `.ai/reviews/` 의 최신 Codex verdict 에 미해결 P1/P2 가 0

## Invariants and risks

- **Ownership and lifetime**: 세션의 소켓·버퍼·OVERLAPPED context 는 **세션이 소유한다.**
  서버는 세션의 소유자이지 그 내부 자원의 소유자가 아니다. IO 참조 카운트가 0 이 되는
  지점은 한 곳뿐이고, 해제는 그 지점에서만 일어난다
- **Thread affinity and synchronization**: accept 완료를 처리하는 스레드와 IO 워커가 같은
  세션 컨테이너를 만진다. 컨테이너의 소유권과 보호 방식을 **코드에서 읽히게** 한다
  (루트 `CLAUDE.md` §5.3). 락을 쓰면 획득 순서를 `Portfolio/CLAUDE.md` 에 `## 락 순서`
  섹션으로 적는다 (루트 `CLAUDE.md:181` 이 프로젝트 `CLAUDE.md` 를 지정한다).
  원자 연산 폭은 카운터 타입과 일치한다 (D3)
- **Input and failure boundaries**: N/A — S2 는 프로토콜을 정의하지 않는다. echo 는 바이트열 그대로.
  payload 0(D4)은 S3 주제다
- **Shutdown, retry and backpressure**: 종료 순서는 stop 신호 → 대기 해제 → join → 자원 해제를
  유지한다. 🔴 **AcceptEx 로 바뀌면 "listen 소켓을 닫아 accept 를 깨운다" 는 S1 의 종료 장치가
  성립하지 않는다.** 미완료 AcceptEx 의 취소·회수(`CancelIoEx` 등)가 이 단계의 진짜 어려운
  지점이고, **완료 조건은 `smoke.stop_without_client` + pending accept 수지다.**
  세션 수지만으로는 잡히지 않는다 (Codex P1-3). 백프레셔는 S4
- **Security and data**: N/A — 자격증명·외부 저장소 없음
- **Performance budget**: N/A — S2 는 측정하지 않는다. 동접 수치는 S4 의 것이다
- 🔴 **가장 가능성 높은 실패 경로**: 세션 수명이 "대충 되는" 상태로 통과하는 것.
  경합은 확률적이라 테스트가 몇 번 통과해도 증명이 아니다. 방지 장치는 위 "판정 장치" 의
  **자원 종류별 수지 + 해제 지점 단일화**이고, 이 둘이 없으면 S2 는 완료가 아니다.
  다만 그것이 막지 못하는 창이 있음을 3항에 명시했다 — **모르는 채로 통과하는 것과
  알고 인정하는 것은 다르다**
- 위험: 세션 컨테이너 설계가 lock-free·풀링으로 번지는 것. Non-goals 에 못박았다
- 위험: AcceptEx 취소 경로를 미루고 "종료가 가끔 멈추는" 상태로 두는 것.
  `smoke.stop_without_client` 가 이것을 잡는다

## Plan

1. 사용자 — RAII 래퍼(WSA 초기화·소켓·IOCP 핸들) + **자원 종류별 수지 카운터**.
   L2 를 먼저 없앤다. `smoke.start_failure_balance` 가 이 단계의 판정이다
2. 사용자 — 세션 타입 도입. 소켓·버퍼·operation 별 OVERLAPPED context 를 세션이 소유한다 (L3)
3. 사용자 — 세션 컨테이너 + 완료 라우팅. completionKey 로 세션을 식별한다 (L5)
4. 사용자 — IO 참조 카운트 `std::atomic<uint32_t>` 와 단일 해제 지점
5. 사용자 — AcceptEx 전환과 미완료 accept 회수를 포함한 종료 경로 (L6).
   `smoke.stop_without_client` 를 이 단계와 같이 만든다
6. 사용자 — `smoke.multi_session_echo` · `smoke.resource_balance` · `smoke.close_race`
7. 사용자 — `regr.iorefcount_width` (D3 canary) + `docs/regressions/D3.md`
8. Claude — `CMakeLists.txt` 라벨 분리, `scripts/phase1.ps1` 라벨 경로, CI 에 regression 추가
9. Claude — `docs/infra/` 해설 갱신
10. 사용자 — `README.md` 상태표 갱신
11. Codex 독립 리뷰(코드) → 차단 발견 해소 → Codex 커밋

1 은 나머지와 독립이라 먼저 끝내고 커밋해도 된다. **4 는 2·3 이후**여야 하고
(세션이 있어야 카운트할 대상이 있다), **8 은 7 이후**여야 한다 (라벨 붙일 테스트가 있어야 한다).

## Implementation log

- 2026-08-31 문서 작성. 코드 미착수. 범위를 다중 세션으로 확정 (사용자 결정, 위 "범위 결정")
- 2026-08-31 **Codex 리뷰 1회차 `Request changes` 반영** (P1 3건 · P2 1건 · P3 1건).
  P1 2건이 진짜 결함이었다 — L2 판정이 `WSAStartup` 참조 카운트 때문에 무효였고,
  AcceptEx pending 회수에 완료 조건이 없었다. 상세는 아래 "Review resolution"

## Verification

| Command or check | Result | Evidence/notes |
|---|---|---|
| S1 코드의 결손 지점 실측 | 확인 | `Core/IOCPServer.{h,cpp}` `273430c` 직접 확인. L1~L6 을 심볼명으로 고정. L2 는 리뷰 지적으로 `CreateIoCompletionPort` 실패까지 확대 |
| `Portfolio/CLAUDE.md` 존재 확인 (리뷰 P2 반박 근거) | 확인 | 파일 존재(3426 bytes), `## 관례` 섹션 있음. 루트 `CLAUDE.md:181` 이 프로젝트 `CLAUDE.md` 를 락 순서 기록 위치로 지정 |
| Codex 독립 리뷰 (문서, 1회차) | Request changes | `.ai/reviews/TASK-20260831-modern-iocp-s2-session-lifetime-codex.md` |
| 빌드 | 미실행 | 코드 미착수 |
| 테스트 | 미실행 | 동일 |

## Handoff to reviewer

- Changed files: 이 문서, `.ai/tasks/INDEX.md`,
  `.ai/reviews/TASK-20260831-modern-iocp-s2-session-lifetime-codex.md`(리뷰 기록)
- Key decisions: S2 범위를 다중 세션까지 확대 / 판정을 **자원 종류별 짝맞춤 수지**로 고정
  (세션 수지만으로는 부족) / **Goal 을 낮추고 미검출 창을 문서가 인정** / 테스트 훅과 ASan
  둘 다 미도입 / AcceptEx 회수에 별도 완료 조건 부여
- Known risks: 세션 수명이 "대충 되는" 상태로 통과. 컨테이너 설계의 범위 팽창.
  AcceptEx 취소 경로 미완
- Review focus: 낮춘 Goal 이 S2 의 존재 이유를 훼손하지 않는가. **자원 종류별 수지가
  L2·AcceptEx 누수를 실제로 잡는가.** Non-goals 가 S3·S4 를 실제로 지켜주는가
- Checks not run: 빌드·테스트 전부. 코드가 없다

## Review resolution

`.ai/reviews/TASK-20260831-modern-iocp-s2-session-lifetime-codex.md` (2026-08-31, `Request changes`)

| 발견 | 상태 | 해소 방식 |
|---|---|---|
| **P1** `smoke.start_failure_releases_resources` 가 L2 를 판정하지 못한다 (`WSAStartup` 이 프로세스 내 참조 카운트라 `WSACleanup()` 누락에도 재시도가 성공한다) | 해소 | 판정을 **자원 종류별 짝맞춤 수지**로 교체. `smoke.start_failure_balance` 로 이름과 내용을 바꾸고 실패 지점별(bind · IOCP 생성) 수지 0 을 요구한다. "판정 장치" 1항에 이유를 남겼다 |
| **P1** 세션 수지 + 크래시 없음으로는 Goal 을 증명하지 못한다 (해제된 세션을 읽었지만 안 터진 경우가 통과) | 해소 | **Goal 을 낮췄다** — "해제된 메모리를 만지지 않는다" → "참조 카운트가 0 이 되기 전에는 해제되지 않는다". 미검출 창을 "판정 장치" 3항에 명시하고 `docs/regressions/D3.md` 에 적게 했다. 리뷰어 제안(테스트 훅 동기화)은 **미채택** — 루트 `CLAUDE.md` §6 위반이고 D1 transport seam 과 달리 구현체가 1개라 예외가 성립하지 않는다 (사용자 결정 2026-08-31) |
| **P1** AcceptEx 취소·회수에 완료 조건이 없다. pending accept 는 세션 카운터에 들어가지 않아 수지로 잡히지 않는다 | 해소 | `smoke.stop_without_client` 신설 — 클라이언트 없이 `start()` → `stop()` 이 반환하고 **pending accept 수지 0**. 수지 카운터 대상에 pending accept 를 추가했다. Invariants 의 종료 항목에서 완료 조건을 명시적으로 연결했다 |
| **P2** `Portfolio/CLAUDE.md` 참조 경로가 틀렸다 (실재하지 않는 파일을 갱신 대상으로 읽을 수 있다) | **미채택** | 사실이 아니다. `D:\GameProjects\Portfolio\CLAUDE.md` 는 실재한다(3426 bytes, `## 관례` 섹션 포함). 루트 `CLAUDE.md:181` 이 "락을 쓰면 획득 순서를 **프로젝트** `CLAUDE.md` 의 `## 락 순서` 섹션에 적는다"고 지정하므로 이 문서의 대상 경로가 맞다. 오해를 줄이기 위해 해당 문장에 근거 줄 번호를 병기했다 |
| **P3** grep 기반 완료 조건이 형식적이고 우회 가능하다 (`accept(` 는 그 파일에서만, `m_recvContext` 는 이름만 바꿔도 통과) | 해소 | `accept` 는 `Core/**` 범위 negative check + `AcceptEx` positive check 를 **짝으로** 요구하도록 강화했다. `m_recvContext`/`m_sendContext` 부재 조건은 **삭제** — `smoke.multi_session_echo` 가 같은 것을 실질적으로 판정한다 |
| (추가 지적) L2 서술이 실제 코드보다 좁다 — `CreateIoCompletionPort` 실패도 같은 누수 패턴 | 해소 | L2 행에 `CreateIoCompletionPort()` 를 포함하고 "실패 지점이 뒤로 갈수록 새는 자원이 늘어난다"를 명시했다 |

리뷰어가 확인한 것 중 유지된 판단: L1~L6 의 사실 주장이 현재 코드와 일치한다,
Non-goals 가 S3·S4 경계를 지킨다, phase1 의 D3 계약과 충돌하지 않는다.

재리뷰 필요. 다만 위 반영은 완료 조건의 판정 방식 변경이므로 **S2 코드가 나온 뒤 함께
판정하는 편이 낫다** (S1 에서 같은 판단을 했다).
