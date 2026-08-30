# AI Development Harness — Portfolio

Codex 와 Claude 가 같은 사실·명세·검증 결과를 공유하기 위한 작업 제어면이다.
소스와 Git diff 가 구현의 진실이고, `.ai` 기록은 의도와 검증의 진실이다.

**`portfolio-legacy` 와 다른 점: 이 저장소는 루트가 git repo 라서 `.ai/` · `.agents/` 가 커밋된다.**
AI 교차개발 프로세스(명세 → 구현 → 독립 리뷰 → 판정)가 히스토리에 남는 것 자체가 포트폴리오 자산이다.

## 이 저장소가 담는 것

| 경로 | 프로젝트 | 정본 규범 |
|---|---|---|
| `modern-iocp/` | 2024 IOCP 포트폴리오를 현재 기술 스택으로 리뉴얼 | 이 하네스 |
| `seamless-world/` | 채널 없는 심리스 오픈월드 서버 | **`seamless-world/ROADMAP.md`** + `seamless-world/CLAUDE.md`(역할 예외) |

## 규범 우선순위 (중복 규정 금지)

로드맵이 이미 정한 것은 **여기서 다시 정의하지 않고 그대로 따른다.**

| 주제 | 정본 | 하네스가 하는 일 |
|---|---|---|
| 커밋 메시지·브랜치·릴리즈·커밋 리듬·devlog | 로드맵 **§8** | 없음 (위임) |
| AI 보조 범위 / 본인이 결정할 것 | 로드맵 **§9** | 없음 (위임) |
| Design Question 문서 형식 | 로드맵 **§10.1** | `decisions/` 가 이 형식을 쓴다 |
| Milestone 문서 형식 | 로드맵 **§10.2** | 없음 (위임) |
| 개발 원칙 6가지 | 로드맵 **§4** | `seamless-world/` 에서는 이쪽이 우선 |
| 엔지니어링 공통 원칙 | `CONSTITUTION.md` | 두 프로젝트 공통 |
| task 명세·독립 리뷰 절차 | `tasks/` · `reviews/` · `../.agents/skills/` | 하네스 고유 |
| Codex 실행 방법 | 아래 "실행 환경"·"표준 호출" | 하네스 고유 |

`modern-iocp/` 는 로드맵 적용 대상이 아니다 — `CONSTITUTION.md` 와 이 문서만 따른다.

## 역할 분담

| 담당 | 기본 역할 |
|---|---|
| Claude | 구현, 자체 검증, task·문서 작성 |
| Codex | 독립 리뷰, **Git 저장소 작업 전담**(스테이징·커밋·브랜치·태그·원격) |

- 같은 task 에서 둘을 동시에 구현자로 두지 않는다. 병렬이 필요하면 파일과 완료 조건이 겹치지 않는 별도 task 로 나눈다.
- **Claude 는 Git 상태를 읽기만 한다.** `status` / `log` / `diff` / `grep` 은 자유롭게 쓰되,
  `add` · `commit` · `branch` · `push` · `restore` 는 실행하지 않고 Codex 에 위임한다.
- 사용자가 특정 task 에서 명시한 경우에만 역할을 교체한다.
- 🔴 **`seamless-world/` 는 상시 예외다** — 그 프로젝트에서 Claude 는 **구현자가 아니라 조언자**이고
  설계 선택은 사용자가 한다. `seamless-world/CLAUDE.md` 가 이 표를 덮는다.

## 상위 지침과의 관계

`D:\GameProjects\CLAUDE.md` 는 트리 전체의 공통 규범이고, 이 하네스는 이 저장소의 구체 절차다.
**충돌 시 이 하네스가 우선한다.** 리뷰 레인은 상위 문서의 서브에이전트 표가 아니라 아래 Codex 호출을 쓴다.

## 실행 환경 (2026-08-28 실측)

- **CLI**: npm `@openai/codex` → `codex-cli 0.150.1`, PATH 등록. `codex login status` = `Logged in using ChatGPT`
- 🔴 **모델**: 호출마다 **`-m gpt-5.4`** 를 명시한다. `~/.codex/config.toml` 기본값 `gpt-5.3-codex` 는
  ChatGPT 계정에서 거부된다(`invalid_request_error`). **전역 설정은 건드리지 않는다**(사용자 소유)
- **경로**: `-C` / `-o` 에는 Windows 경로(`D:\...`)를 준다. Git Bash 형식(`/d/...`)은 `os error 3` 으로 실패한다.
  Git Bash 에서 호출하면 앞에 `MSYS_NO_PATHCONV=1` 을 붙인다
- **셸별 진입점**: Git Bash 는 `codex`. PowerShell 은 bare `codex` 가 `codex.ps1` 로 잡혀 실행정책 오류가 나므로 **`codex.cmd`**
- **무시할 노이즈**: `rmcp::transport ... 127.0.0.1:8080` (unityMCP 미기동), `Model metadata ... not found`.
  실제 실패 신호는 `invalid_request_error` 와 **비어 있는 결과 파일**이다
- **비용 기준선**: 문서 1개 읽고 한 문장 답하는 최소 호출 ≈ 18k 토큰 / 문서 리뷰 1회 ≈ 61k 토큰

## 표준 호출

아래는 **전부 실제로 실행해 확인한 형태만** 적는다. `codex review` 는 `-m`/`-C`/`-s`/`-o` 를 **받지 않고**,
`--uncommitted` 와 프롬프트를 **동시에 줄 수 없다.** 반드시 `codex exec` 의 서브커맨드 형태를 쓴다.

> 🔴 **문서에 적는 명령은 반드시 1회 실행한 뒤에 적는다.** `--help` 만 보고 적었다가
> Codex 리뷰에 걸린 전례가 있다(`portfolio-legacy` TASK-20260828).

### A. diff 독립 리뷰 — 기본형

```bash
MSYS_NO_PATHCONV=1 codex exec -m gpt-5.4 -s read-only \
  -C "D:\GameProjects\Portfolio" review --uncommitted
```

`--uncommitted` 대신 `--base <branch>` 또는 `--commit <sha>` 도 쓸 수 있다.

### B. 커스텀 지시 리뷰 (완료 조건·불변식 대조)

```bash
MSYS_NO_PATHCONV=1 codex exec -m gpt-5.4 -s read-only \
  -C "D:\GameProjects\Portfolio" \
  -o "D:\GameProjects\Portfolio\.ai\reviews\TASK-YYYYMMDD-name.md" \
  - < prompt.txt
```

프롬프트 끝에 반드시 넣는다 — 샌드박스가 read-only 라 Codex 가 파일을 쓰지 못한다:

> 파일을 직접 쓰지 마라. 너의 **마지막 메시지 전체**를 리뷰 문서 본문으로 출력해라.
> 서두·코드펜스 없이 `# Review: ...` 로 시작하는 마크다운 본문만 출력한다.

### C. Git 작업 위임 — ⚠ 비대화형은 막혀 있다

`-s workspace-write` 는 이 머신에서 `.git` 쓰기가 차단된다(`index.lock: Permission denied`).
`windows.sandbox` 기본값 `elevated` 는 아예 파일을 못 읽고 `Blocked` 를 낸다(유효값은 `elevated`/`unelevated` 뿐).

**따라서 커밋은 대화형 세션에서 사람 승인을 거친다.**

```powershell
cd D:\GameProjects\Portfolio
codex.cmd
```

`--dangerously-bypass-approvals-and-sandbox` 는 쓰지 않는다.

## 권장 흐름

1. 사용자가 목표를 정한다
2. Claude 가 task 문서를 만들고 기준 커밋과 완료 조건을 기록한다
3. Claude 가 `tasks/INDEX.md` 에 담당 파일을 표시한 뒤 구현한다
4. Claude 가 자체 검증 명령과 결과를 기록하고 `Review requested` 로 바꾼다
5. Claude 가 위 A 또는 B 로 Codex 리뷰를 받고, 결과를 `reviews/` 에 남긴다
6. 차단 발견을 해결한 뒤에만 task 를 `Done` 으로 바꾼다
7. Codex 가 커밋한다 (메시지 규약은 로드맵 §8)

에이전트가 둘 사용됐다는 사실만으로 교차검증이 성립하지 않는다.
**리뷰 파일이 실제로 작성되기 전에는 교차검증이 끝났다고 쓰지 않는다.**

## Git 취급

- `.ai/` · `.agents/` · `AGENTS.md` · `CLAUDE.md` — **커밋한다**. 프로세스가 자산이다
- `.omc/` — AI 세션 상태(머신 경로·세션 비용 포함). `.gitignore` 처리됨. **절대 커밋하지 않는다**
- `.local/` · `ClaudeCodeHistory/` — 로컬 세션·민감 기록. `.gitignore` 처리됨. **절대 커밋하지 않는다**
