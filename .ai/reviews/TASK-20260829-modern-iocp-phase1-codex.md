# Review: TASK-20260829-modern-iocp-phase1 (Codex)

> 이 문서는 Codex 가 생성한 리뷰 원문이다. 공개 저장소에 두기 위해 구직 일정 등
> 개인 정보에 해당하는 표현만 `[비공개]` 로 가렸다. 판정과 논거는 그대로다.


- Reviewer: Codex
- Reviewed baseline: `main` (`git rev-parse --verify HEAD` 실패, 아직 커밋 없음)
- Reviewed head/worktree: 문서와 하네스만 있는 초기 worktree, `modern-iocp/` 코드는 아직 없음
- Verdict: Request changes

## Independent checks

| Check | Result | Evidence |
|---|---|---|
| 규범 문서 확인 | 확인 | `.ai/README.md`, `.ai/CONSTITUTION.md`, `.ai/tasks/INDEX.md`, `.agents/skills/cross-agent-development/SKILL.md` 직접 읽음 |
| 결정 문서 확인 | 확인 | `.ai/decisions/20260829-modern-iocp-scope.md:1-107` |
| task 문서 확인 | 확인 | `.ai/tasks/TASK-20260829-modern-iocp-phase1.md:1-121` |
| 저장소 상태 확인 | 확인 | `git status --short --branch` = `No commits yet on main`, top-level untracked 7개 표기 |

## Findings

### [P1] 완료 조건이 독립 검증 가능한 문장으로 고정돼 있지 않다

- File/line: `TASK-20260829-modern-iocp-phase1.md:56-66`, `CONSTITUTION.md:60-66`
- 확인된 사실: `타입으로 강제된다`, `검증한다`, `근본원인`, `원커맨드 실행 절차`, `차단 발견이 해소됐다`, `코드 없이 설명할 수 있다`는 표현에 명령, 산출물, 판정자, 실패 기준이 없다.
- 판단: 이 상태로는 `Done` 판정을 기계적으로 막지 못한다. 이 저장소의 핵심 가치인 “증거 기반 포트폴리오”와 충돌한다.
- 영향: 사람이 마음먹으면 같은 결과물을 두고도 PASS/FAIL 을 다르게 쓸 수 있다. 특히 마지막 항목은 현재 문장만으로는 감사 가능한 완료 조건이 아니다.
- Suggested safe direction: 각 항목을 산출물과 명령으로 분해하라. 예시는 다음 정도가 최소다. `ctest -L regression`에 결함별 테스트 이름 고정, `scripts\phase1.ps1` 같은 정확한 원커맨드 지정, 벤치 문서에 `INVALID` 게이트 포함, 덤프 문서에 재현 명령·덤프 파일명·심볼 경로·패치 커밋 연결, 리뷰 항목은 “최신 리뷰 verdict 가 `Approve` 또는 `Approve with changes` 이고 미해결 P1/P2 없음”으로 치환. 마지막 항목은 사용자 작성 `EXPLAIN.md` 또는 코드 비가시 상태의 질의응답 기록으로 바꿔라.

### [P1] AI 역할 경계가 선언만 있고 실제 작업 장치로 내려오지 않았다

- File/line: `20260829-modern-iocp-scope.md:60-64`, `TASK-20260829-modern-iocp-phase1.md:14-19`, `INDEX.md:26-29`, `README.md:33-45`
- 확인된 사실: 모든 문서가 “Claude 는 구현 대행자가 아니다”라고 쓰지만, 정작 어떤 파일을 누가 써도 되는지는 `S1 착수 전에 정한다`로 미뤄져 있다.
- 판단: 이 상태로 S1 이 시작되면 경계가 가장 먼저 무너질 곳은 테스트 하네스, CI, Compose, README, 덤프 재현 스크립트다. 그런데 이 파일들이 바로 포트폴리오 증거다.
- 영향: 네트워크 코어만 사용자가 쓰고 검증 체계는 AI 가 쓰는 형태가 가능해진다. 그러면 “사용자가 직접 구현하고 이해했다”는 저장소의 핵심 주장이 약해진다.
- Suggested safe direction: S1 전에 경계 표를 고정하라. 내 권고는 보수적으로 가는 것이다. `modern-iocp/**`, `.github/**`, `scripts/**`, `docker-compose*.yml`, 테스트, 벤치, 덤프 재현 코드, README 는 모두 사용자 작성으로 고정하고, Claude/Codex 는 `.ai/**` 문서와 리뷰, 질문, Git 작업만 맡겨라. 예외를 둘 거면 파일 경로 단위로 명시하고 “왜 역량 증거를 오염시키지 않는가”까지 적어라.

### [P1] “결함 패턴을 임시로 넣어 FAIL 확인” 계획은 그대로 두면 가짜 PASS 를 만들기 쉽다

- File/line: `TASK-20260829-modern-iocp-phase1.md:35-37,57`, `20260829-modern-iocp-scope.md:96-99`, `CONSTITUTION.md:44-50`
- 확인된 사실: 계획은 레거시 빌드 대신 신규 코드에 버그 패턴을 임시 삽입해 FAIL 을 확인하는 방식이다.
- 판단: 이 방식은 실행 가능하지만, 결함별 재현 seam 을 먼저 설계하지 않으면 증거가 약하다. `return true` 누락은 UB 라 재현이 비결정적이고, `InterlockedDecrement64` 폭 불일치는 메모리 stomp 라 flaky 해지기 쉽고, 부분 송신은 loopback 에서 자연 재현이 안 될 수 있다.
- 영향: “FAIL 을 봤다”가 실제 레거시 결함과 다른 인공 실패가 될 수 있다. 그러면 회귀 테스트 축이 무너진다.
- Suggested safe direction: 결함별로 재현 계약을 먼저 문서화하라. 부분 송신은 짧은 completion 을 강제로 돌려주는 transport seam, 폭 불일치는 canary 를 둔 희생 구조체 단위 테스트, 반환값 문제는 UB 재현이 아니라 “성공 경로가 false/unknown 을 반환하는 fault injection”으로 바꿔라. 각 결함에 대해 `legacy symptom -> injected behavior -> expected FAIL -> fix -> PASS` 매핑을 남겨라.

### [P2] “Phase 1만 완료하고 중단해도 성립”이라는 가장 중요한 조건이 아직 문서상 완결되지 않았다

- File/line: `20260829-modern-iocp-scope.md:68-81,90-92`, `TASK-20260829-modern-iocp-phase1.md:41-65,86-92`
- 확인된 사실: 현재 Phase 1 은 raw IOCP, DB/Redis, 성능 측정, WinDbg, CI 까지는 담고 있다. 반면 결정 문서가 범위 1을 고른 이유였던 `운영/모니터링` 증거와 `Visual Studio` 축은 acceptance criteria 에 명시돼 있지 않다.
- 판단: 방향 자체는 맞다. 다만 지금 문서만으로는 “이 Phase 1 릴리즈 하나로 Windows/VS/덤프/네트워크/DB/성능을 보여준다”는 종료점이 약간 비어 있다.
- 영향: 목표일 전에 여기서 멈추면 포트폴리오로는 거의 성립하지만, 면접관이 `VS 로 어떻게 빌드/디버그했는지`, `운영 관측은 무엇을 남겼는지`를 물을 때 문서상 증거가 약하다.
- Suggested safe direction: Phase 1 을 최종 정지점으로 삼고 두 항목을 추가하라. `Visual Studio 2022` 로 실제 열고 빌드/덤프 재현한 절차 또는 preset, 그리고 exported counter/metrics 캡처 같은 운영 관측 artifact. 이 둘이 들어가면 Phase 1 단독 종료가 성립한다. 반대로 Asio 증거는 별도 커리어 결손일 뿐, Phase 1 유효성의 선행조건으로 만들면 안 된다.

### [P2] 이미 확인된 zero-length payload 결함이 회귀 필수 목록에서 빠져 있다

- File/line: `TASK-20260829-modern-iocp-phase1.md:32-33,57,89`
- 확인된 사실: 문제 증거 섹션은 `useSize <= sizeof(WanHeader)` 결함을 이미 알고 있다고 적는다. 그런데 acceptance criteria 는 “결함 3종”만 강제하고, S3 도 수신 경계 조건을 다루지만 FAIL-first 기록 의무는 없다.
- 판단: 알려진 입력 경계 결함을 문서에 올려놓고도 필수 회귀 항목에서 빼면 Reliability Lab 의 설득력이 떨어진다.
- 영향: Phase 1 이 PASS 해도 알려진 parser boundary bug 가 남을 수 있다.
- Suggested safe direction: 이 결함을 4번째 회귀 항목으로 승격하거나, 정말 Phase 1 에서 안 다룰 거면 문제 증거에서 빼고 별도 defer 이유를 남겨라. 지금처럼 “알고는 있지만 필수는 아님” 상태가 가장 나쁘다.

### [P2] Phase gate 는 선언에 가깝고, 범위 팽창을 막는 구조적 장치가 약하다

- File/line: `20260829-modern-iocp-scope.md:57-58,85-89,103-105`, `TASK-20260829-modern-iocp-phase1.md:48,94`, `INDEX.md:36-39`
- 확인된 사실: 문서는 반복해서 “Phase 1 릴리즈 전에는 Phase 2/3 금지”라고 쓴다. 하지만 금지 위반을 구조적으로 어렵게 만드는 장치는 없다.
- 판단: 과거 실패 패턴이 “다음 마일스톤이 계속 생김”이었다면, 텍스트 경고만으로는 부족하다.
- 영향: 새 decision/task 를 계속 열면서도 형식상으로는 gate 를 어기지 않는 상황이 생길 수 있다.
- Suggested safe direction: `INDEX.md` 에 “활성 modern-iocp task 1개 제한”, “Phase 2/3 task 생성 금지”, “Phase 1 release tag 생성 전 backlog 문서 외 신규 스코프 금지”를 넣어라. 마감일(`[비공개]`) 이후에는 bugfix/doc/release task 만 허용하고, 범위 추가는 별도 결정 문서로만 열리게 하라.

### [P2] S1~S6 순서는 재학습 곡선과 증거 축에 맞게 조금 당겨야 할 항목이 있다

- File/line: `TASK-20260829-modern-iocp-phase1.md:86-92`
- 확인된 사실: 덤프 분석은 S6, 부하/백프레셔는 DB/Redis 뒤인 S5 에 있다.
- 판단: raw IOCP 재학습이 목적이면 crash/dump plumbing 과 transport-only 부하 baseline 은 더 앞에 와야 한다. 늦게 넣으면 재작업이 늘고, DB/auth 잡음 때문에 네트워크 코어 학습이 흐려진다.
- 영향: 덤프용 빌드 플래그, PDB, intentional crash path 를 마지막에 붙이게 되고, 성능 기준선이 “코어”가 아니라 “코어+스토리지”로 섞일 수 있다.
- Suggested safe direction: S1 또는 S2 에 덤프 생성/심볼 경로/intentional crash skeleton 을 당기고, S3 직후에 transport/backpressure baseline 을 한 번 측정하라. DB/Redis 는 그 다음 end-to-end 시나리오로 올리는 편이 맞다.

### [P3] 작업 문서의 현재 상태 사실이 정확하지 않다

- File/line: `TASK-20260829-modern-iocp-phase1.md:8`
- 확인된 사실: 문서는 `untracked 6` 이라고 적지만, 현재 `git status --short --branch` 는 top-level untracked 7개를 보여준다.
- 판단: 큰 문제는 아니지만, 이 저장소는 “문서 기록 자체가 자산”이라는 전제를 갖고 있다.
- 영향: 작은 사실 오차가 누적되면 이후 리뷰에서 문서 신뢰도가 떨어진다.
- Suggested safe direction: 변동성 있는 개수는 빼고, 날짜가 붙은 `git status` 스냅샷이나 “커밋 없음” 같은 안정 사실만 적어라.

## Residual risks

- 확인된 결손: 남는 빈칸은 `Visual Studio 로컬 빌드/디버그 증거`와 `운영 관측 artifact` 다. 둘 다 Phase 1 안에 넣어야 한다.
- 확인된 과잉: `Phase 2/3`, 게임 로직 복원, lock-free 구조 선제 도입은 전부 현 마감 기준 과잉이다. 일정이 흔들리면 가장 먼저 잘라야 한다.
- 판단: Boost.Asio 공개 이력 결손은 여전히 남는다. 다만 그것은 이 Phase 1 의 유효성을 깨는 차단 이슈가 아니라, Phase 1 릴리즈 이후 별도 자산으로 다룰 문제다.

## Decision

Request changes. 방향은 맞지만, 완료 조건의 검증성, AI 경계의 실운영 규칙, 결함 재현 방식, Phase 1 단독 종료점 정의를 먼저 고정해야 이 문서가 실제 게이트로 작동한다.