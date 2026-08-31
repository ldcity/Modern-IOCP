# Task Index

## Active

| Task | Status | Implementer | Reviewer | Owned paths |
|---|---|---|---|---|
| [TASK-20260829-modern-iocp-phase1](TASK-20260829-modern-iocp-phase1.md) | **Phase 정의** (실행 task 아님) | — | Codex | `.ai/tasks/TASK-20260829-modern-iocp-phase1.md` |
| [TASK-20260831-modern-iocp-s2-session-lifetime](TASK-20260831-modern-iocp-s2-session-lifetime.md) | **Approved** — 구현 착수 가능 (Codex 리뷰 1회차 반영) | 사용자(코드) + Claude(인프라) | Codex | `modern-iocp/**`, `.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md` |

활성 실행 task 1개(S2). Phase 게이트 준수 — S3 이후 task 를 미리 열지 않는다.

## Rules

- 새 task 의 기본 역할은 `Implementer: Claude`, `Reviewer: Codex` 다.
- 사용자가 task 단위로 명시한 경우에만 역할을 바꾼다.
- 파일 이름은 `TASK-YYYYMMDD-short-name.md` 형식을 쓴다.
- **하나의 경로를 동시에 소유하는 활성 task 를 만들지 않는다.**
- `Done` 작업은 `Completed` 섹션으로 옮긴다.
- 리뷰가 실제로 끝나기 전에는 reviewer 를 완료 표시하지 않는다.
- **Git 저장소를 바꾸는 작업은 task 소유자와 무관하게 Codex 가 수행한다.**

## 프로젝트별 정본

| 경로 | 정본 규범 |
|---|---|
| `modern-iocp/` | `.ai/CONSTITUTION.md` + `.ai/README.md` |
| `seamless-world/` | `seamless-world/ROADMAP.md` (§4 개발원칙 · §8 Git · §9 AI 경계 · §10 템플릿) + `seamless-world/CLAUDE.md` |

🔴 **역할 주의**: **이 저장소 전체에서 Claude 는 구현 대행자가 아니다.** `seamless-world/` 에만
적용되던 예외가 2026-08-29 에 `modern-iocp/` 까지 확대됐다. 이 저장소의 코드는 소유자가 직접
설계·구현한다는 것이 분리의 전제이기 때문이다. 소유자가 설명할 수 없는 코드는 들어오지 않는다.

## 확정된 결정

- **진행 순서: `modern-iocp` → `seamless-world`** (2026-08-28, 사용자 결정). 병행하지 않는다.
  재논의하지 않는다.
- `seamless-world/` 는 착수 전까지 `ROADMAP.md` 외 파일을 만들지 않는다.
- **`modern-iocp` 범위 = IOCP Reliability Lab, 스택 = raw Win32 IOCP(Phase 1) → Asio/코루틴(Phase 2)
  → IOCP 코루틴 어웨이터(Phase 3) 단계적 진화** (2026-08-29, 사용자 결정).
  근거와 버린 안은 [`../decisions/20260829-modern-iocp-scope.md`](../decisions/20260829-modern-iocp-scope.md).
- **역할 경계**: 네트워크 코어·테스트·벤치·덤프 분석·README·`EXPLAIN.md` 는 사용자가 쓴다.
  Claude 는 빌드/CI/Compose/스크립트만 쓰고 `modern-iocp/docs/infra/` 에 해설을 남긴다.

## Phase 게이트 (구조적 제한)

선언만으로는 범위 팽창을 막지 못한다는 Codex 리뷰 P2 를 반영한 규칙이다.
아래는 권고가 아니라 **task 생성 규칙**이다.

- **활성 `modern-iocp` 실행 task 는 항상 1개다.** S 단계 task 를 미리 여러 개 열지 않는다.
  `TASK-20260829-modern-iocp-phase1` 은 Phase 정의 문서이며 이 계수에 들어가지 않는다 —
  `modern-iocp/**` 를 소유하지 않으므로 소유 경계도 겹치지 않는다.
- **Phase 2/3 의 task 문서를 만들지 않는다.** Phase 1 릴리즈 태그가 생기기 전까지 금지다.
  아이디어는 task 가 아니라 결정 문서의 backlog 절에만 적는다.
- **Phase 1 릴리즈 목표일 이후에는 bugfix · 문서 · 릴리즈 task 만 연다.** 범위를 늘리는 변경은
  새 결정 문서를 열어 사용자 승인을 받아야 시작할 수 있다.
- Phase 1 완료 조건에서 항목을 빼려면 **task 가 아니라 결정 문서에서** 뺀다.

## Completed

| Task | 완료 | 결과 |
|---|---|---|
| [TASK-20260829-modern-iocp-s1-skeleton](TASK-20260829-modern-iocp-s1-skeleton.md) | 2026-08-31 | S1 골격 + 덤프 배관. Codex 리뷰 3회(1회차 사전 · 코드 · 툴체인 고정) 반영. CI run #2 green `2f430c0` |
