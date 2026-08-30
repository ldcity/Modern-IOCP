# Claude Project Instructions

`AGENTS.md` 를 이 저장소의 공통 진입점으로 사용하고 완전히 읽는다.

비단순 작업에서는 다음 공용 원본도 읽는다.

- `.ai/README.md` — 실행 계약 · 규범 우선순위 · Codex 표준 호출
- `.ai/CONSTITUTION.md`
- `.agents/skills/cross-agent-development/SKILL.md`
- `.ai/tasks/INDEX.md` 가 가리키는 활성 작업 문서
- `seamless-world/` 작업이면 `seamless-world/ROADMAP.md`(정본) + `seamless-world/CLAUDE.md`(🔴 **역할 예외** — 거기선 Claude 가 구현자가 아니다)

## 상위 지침과의 관계

`D:\GameProjects\CLAUDE.md` 는 트리 전체의 공통 규범이다. 이 하네스는 그 위에 얹히는 저장소 전용
절차이며 **충돌 시 이 하네스가 우선한다.** 리뷰 레인은 상위 문서의 서브에이전트 표가 아니라
`.ai/README.md` 의 Codex 표준 호출을 쓴다.

## 역할

기본 역할은 Claude 가 구현 담당이고 Codex 가 독립 리뷰 담당이다. Claude 는 task 명세·완료 조건·
담당 경로를 먼저 확정하고 구현·자체 검증·인계 기록까지 수행한다. 이후 Codex 가 리뷰하는 동안
같은 파일을 수정하지 않는다.

**Git 저장소 작업은 Codex 전담이다.** Claude 는 `git status` / `log` / `diff` / `grep` 같은 읽기
명령만 쓰고, `add` · `commit` · `branch` · `push` · `restore` 는 실행하지 않는다.
커밋 메시지 규약은 로드맵 §8 을 따른다.

사용자가 특정 task 에서 역할을 명시적으로 바꾼 경우에만 Claude 가 리뷰를 맡는다.

🔴 **`modern-iocp/` 는 역할 예외다.** 이 저장소는 소유자가 직접 설계·구현하고 설명할 수 있는
코드만 포트폴리오로 남기기 위해 분리됐다. 경계는 **"면접에서 질문받을 수 있는가"**로 긋는다 —
IOCP 코어·회귀 테스트·벤치·덤프 분석 문서는 사용자가 쓰고, Claude 는 빌드·CI·
스크립트·테스트 하네스 같은 배관만 쓴다. 상세 표는
`.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md` 의 "작성자 분담" 절에 있다.

## 관례

`modern-iocp/` 기준. 새 코드는 여기에 맞추고, 새로 정해진 관례는 이 절에 추가한다.

- 파일 배치: `Source/` · `Test/` (PascalCase 파일명, 예 `IOCPServer.cpp`) — 레거시 5종과 동일
- 멤버 변수: `m_` 접두 + camelCase (`m_listenSocket`) (근거 `Source/IOCPServer.h:56`)
- 상수: `k` 접두 + PascalCase (`kMaxSockBuf`) (근거 `Source/IOCPServer.h:12`)
- 함수: **camelCase** (`initSocket`, `installCrashHandler`) (근거 `Source/CrashHandler.h:5`)
- **접근자는 `get`/`set` 접두**: `getServerPort() const` / `setServerPort(int)`
  (사용자 결정 2026-08-30, 근거 `Source/IOCPServer.h:48`)
- 🔴 멤버 함수 이름이 Winsock 전역 함수(`bind`·`listen`·`send`·`recv`·`accept`)와 같으면
  클래스 스코프가 먼저 잡혀 전역 함수를 가린다. 멤버 이름을 다르게 짓는다
- 에러 처리: 예외를 쓰지 않는다. `bool` 반환 + `printf("[ERROR] ...")`
- 로그 접두: `[INFO]` / `[ERROR]` / `[CRASH]`
- 소스 인코딩: 한글 주석이 있으므로 **UTF-8 with BOM** (MSVC C4819 회피).
  `.ps1` 도 동일 — Windows PowerShell 5.1 은 BOM 이 없으면 cp949 로 읽는다
- 빌드는 `/W4 /WX`. 경고가 곧 빌드 실패다 (레거시 결함 D2 가 C4715 로 걸리는 설정)
