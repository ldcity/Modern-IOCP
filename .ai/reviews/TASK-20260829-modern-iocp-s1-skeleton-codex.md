# Review: TASK-20260829-modern-iocp-s1-skeleton (Codex)

## B. S1 신규 리뷰 Findings

### [P1] S1 문서는 덤프 plumbing 이 “CI 에서도 같은 방식으로 재현된다”는 주장을 아직 독립 검증 가능하게 만들지 못했다

- File/line: `TASK-20260829-modern-iocp-s1-skeleton.md:26-28,64-70,99-107`
- 확인된 사실: Goal 은 clone 직후 `build`/`test`/`--crash-test` 와 CI 재현까지 말한다. 그러나 완료 조건은 로컬 `scripts\phase1.ps1 crash`, 수동 문서 `docs/dump-analysis-00-plumbing.md`, 그리고 일반적인 “Windows workflow green”만 요구한다. 워크플로가 실제로 crash 경로를 실행하는지, `.dmp` 와 짝이 맞는 `.pdb` 를 보존하는지, 심볼 경로가 무엇인지에 대한 기계적 판정 항목이 없다.
- 판단: 지금 문장으로는 CI 가 green 이어도 crash 경로가 죽어 있거나, 러너에서 생성된 dump/PDB 가 재검증 불가능한 상태일 수 있다. 그러면 “덤프 plumbing 을 S1 에서 끝냈다”는 판정이 성립하지 않는다.
- 수정: `scripts\phase1.ps1 verify-dump` 또는 `verify-s1` 를 추가해 `Release build -> intentional crash -> dump 생성 확인 -> 대응 PDB 존재 확인`까지 exit code 로 판정하게 하라. GitHub Actions 는 그 서브커맨드를 실제로 실행하고 `*.dmp` 와 대응 `*.pdb` 를 artifact 로 업로드하도록 완료 조건에 넣어라. `docs/dump-analysis-00-plumbing.md` 에는 로컬에서 사용한 정확한 dump 파일명과 symbol path 문자열을 고정해라.

### [P2] crash handler 계약이 느슨하고, 현재 문서만으로는 `MiniDumpWriteDump` 호출 지점과 Release 검증 구성이 고정되지 않는다

- File/line: `TASK-20260829-modern-iocp-s1-skeleton.md:48-52,64-69,99-107`
- 확인된 사실: 작성자 분담 표는 사용자 산출물로 `SetUnhandledExceptionFilter` + `MiniDumpWriteDump` 를 적는다. 하지만 어떤 스레드/코드 경로가 의도적 크래시를 발생시키는지, dump 가 어디에 어떤 이름으로 쓰이는지, `docs/vs-build.md` 가 `build` 와 같은 x64 Release preset 을 써야 하는지는 적혀 있지 않다.
- 판단: S1 이 실패할 가장 가능성 높은 경로가 여기다. `SetUnhandledExceptionFilter` 만 적어두면 의도적 crash 시나리오가 느슨해지고, VS 문서가 Debug 기준으로 채워져도 형식상 통과할 수 있다. 그러면 Release dump plumbing 검증과 수동 디버거 검증이 서로 다른 대상을 보게 된다.
- 수정: task 문서에 crash contract 를 고정해라. `--crash-test` 는 빌드된 실행 파일의 고정된 코드 경로에서 의도적 예외를 발생시키고, dump 는 제어된 SEH 경계에서 정해진 디렉터리/파일명으로 기록한다고 명시하라. `docs/vs-build.md` 와 `docs/dump-analysis-00-plumbing.md` 는 동일한 x64 Release-with-PDB preset 을 사용한다고 못 박고, 최적화 때문에 S1 에서 보장하는 것은 “심볼이 붙은 stack”까지라고 범위를 적어라.

### [P2] 작성자 분담이 dump plumbing 경계에서 파일 단위로 유지되기 어렵다

- File/line: `TASK-20260829-modern-iocp-s1-skeleton.md:43-53,99-101`
- 확인된 사실: Claude 는 `CMakeLists.txt`/preset/script/PDB 설정을 맡고, 사용자는 `src/**` 의 crash handler 와 `--crash-test` 경로를 맡는다. 실제로는 `DbgHelp.lib` 링크, dump 출력 경로, CLI 옵션, artifact 업로드가 `CMakeLists.txt`, `src/**`, `scripts/phase1.ps1`, `.github/workflows/ci.yml` 에 걸쳐 한 덩어리로 움직인다.
- 판단: 지금 표는 개념 분담이지 파일 경계가 아니다. 이 상태로 구현이 시작되면 Claude 가 소스 쪽 crash 동선을 만지거나, 사용자가 infra 파일을 손대야 완료되는 순간이 나온다. 그 순간 역할 경계는 문서상으로만 남는다.
- 수정: S1 문서에 file-level ownership 을 추가해라. 예를 들어 사용자는 `src/crash_handler.*`, `src/main.cpp` 의 `--crash-test` 분기, dump 파일명 규칙을 맡고, Claude 는 `CMakeLists.txt` 의 링크/플래그, `CMakePresets.json`, workflow artifact 업로드, `scripts/phase1.ps1` 래퍼를 맡는다고 고정하라. 경계를 넘는 수정은 task 본문에 예외로 기록하게 하라.

### [P2] 도구 버전 고정이 문서 서술에 머물러 있고, 실제 재현성을 보장할 기계 판정 항목이 빠져 있다

- File/line: `TASK-20260829-modern-iocp-s1-skeleton.md:21-22,45-49,74-75`
- 확인된 사실: 문서는 “toolset·SDK·vcpkg baseline 버전이 고정”돼야 한다고 쓰지만, 완료 조건은 그것이 `README.md` 와 `docs/infra/` 에 적혀 있음을 요구할 뿐이다. `CMakePresets.json` 에 toolset/Windows SDK 가 박혀 있어야 한다는 조건, vcpkg baseline 이 manifest/config 에 박혀 있어야 한다는 조건, workflow runner 를 뜨는 이미지로 고정해야 한다는 조건이 없다.
- 판단: prose pinning 은 레거시의 재현 불가를 막지 못한다. 가장 먼저 깨지는 곳은 fresh clone 과 CI 다.
- 수정: 완료 조건을 바꿔라. `vcpkg` baseline 은 manifest/config 에, MSVC toolset 과 Windows SDK 는 `CMakePresets.json` 또는 동등한 기계 파일에, CI runner 는 `windows-2022` 같은 명시적 이미지에 고정해야 한다고 적어라. `scripts\phase1.ps1` 에 `doctor` 서브커맨드를 추가하거나 `build` 초기에 VS instance, MSVC toolset, SDK, vcpkg baseline 을 출력·검증하게 하라.

## A. 이전 8건 재검증

| # | 판정 | 근거 |
|---|---|---|
| 1 | 해소 | 완료 조건이 명령/산출물 경로로 치환됐다. `scripts\phase1.ps1 build`, `ctest -L regression`, `docs/...` 경로, 최신 리뷰 verdict 까지 판정 문장이 고정돼 있다 (`TASK-20260829-modern-iocp-phase1.md:89-115`). |
| 2 | 해소 | 역할 경계가 경로 표로 내려왔고, `INDEX` 에 저장소 차원의 역할 규칙과 `docs/infra/` 해설 의무가 추가됐다 (`TASK-20260829-modern-iocp-phase1.md:15-39`, `.ai/tasks/INDEX.md:40-55`). |
| 3 | 부분 해소 | D1 과 D2 는 타당해졌다. 특히 D2 를 UB 재현 대신 caller-visible failure fault injection 으로 바꾼 것은 정직하다 (`TASK-20260829-modern-iocp-phase1.md:64-65`). 그러나 D3 의 canary 희생 구조체는 “폭 불일치로 인접 메모리가 오염된다”는 injected stomp 는 잡아도, 레거시 세션 객체에서의 실제 오염 경로 자체를 재현한다고 보기는 어렵다 (`TASK-20260829-modern-iocp-phase1.md:66`). 결정 문서의 “결함 패턴을 신규 코드에 이식해 FAIL” 표현도 아직 더 넓다 (`.ai/decisions/20260829-modern-iocp-scope.md:108-111`). |
| 4 | 해소 | Phase 1 단독 종료점에 VS 축과 운영 관측이 들어왔다. `docs/vs-build.md`, `docs/ops-observability.md`, dump analysis 조건이 명시됐다 (`TASK-20260829-modern-iocp-phase1.md:97-105`). |
| 5 | 해소 | zero-length payload 가 D4 로 승격됐고 회귀 필수 목록과 S3 단계에 들어갔다 (`TASK-20260829-modern-iocp-phase1.md:52,94-95,143`). |
| 6 | 해소 | gate 가 `INDEX` 의 task 생성 규칙으로 내려왔다. 활성 실행 task 1개 제한, Phase 2/3 task 금지, 마감 후 bugfix/doc/release 제한이 구조적 장치다 (`.ai/tasks/INDEX.md:43-55`). |
| 7 | 해소 | 순서가 조정됐다. dump plumbing 이 S1 로, transport-only baseline 이 DB 앞 S4 로 이동했다 (`TASK-20260829-modern-iocp-phase1.md:138-147`). |
| 8 | 부분 해소 | 이전의 `untracked 6` 오류는 사라졌다. 그러나 당시 Phase 문서는 `Baseline: main, 커밋 없음` 이라고 썼지만 실제 HEAD 에는 첫 커밋이 존재했고 worktree 도 변경 중이었다 (`TASK-20260829-modern-iocp-phase1.md:8`, `git rev-parse HEAD`, `git status --short --untracked-files=all`). 공개 히스토리 재작성 후 활성 task 의 기준은 새 루트로 갱신한다. |

## B. 기타 S1 판정

- 종료 경로를 S1 에 넣은 판단은 맞다. `stop -> PostQueuedCompletionStatus -> join -> 자원 해제` 는 S2 침범이 아니라 smoke test 와 dump plumbing 을 프로세스 kill 없이 끝내기 위한 최소 전제다 (`TASK-20260829-modern-iocp-s1-skeleton.md:84-86`).
- `README.md` 뼈대를 S1 에서 만들고 단계마다 갱신하는 선택 자체는 타당하다. 다만 그 문서는 현재 상태와 실행 절차만 담아야 하고, 향후 단계 계획은 계속 phase/task 문서에 남겨야 한다 (`TASK-20260829-modern-iocp-s1-skeleton.md:71-72`).
- 가장 가능성 높은 실패 경로는 “자동화 가능한 dump 생성 증거”와 “수동 디버거 확인 증거”가 섞인 채 진행되는 것이다. 방지 장치는 `verify-dump` 같은 자동 검증 서브커맨드와, workflow artifact 업로드, 그리고 로컬 수동 확인 문서의 역할 분리를 문서에 못 박는 것이다.
- 위에서 다루지 않은 빠진 위험은 fresh clone 전제다. `scripts\phase1.ps1 build` 한 줄 목표를 유지하려면 VS/Windows SDK 미설치 시 무엇이 부족한지 바로 실패 메시지로 알려줘야 한다. 그렇지 않으면 첫 재현 실패가 환경탓인지 저장소탓인지 분리되지 않는다.

## Verdict

- (A) 재검증: `해소 6`, `부분 해소 2`, `미해소 0`
- (B) S1 신규 리뷰: `Request changes`
- Final verdict: `Request changes`
