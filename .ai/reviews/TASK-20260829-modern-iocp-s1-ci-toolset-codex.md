# Review: TASK-20260829-modern-iocp-s1 — CI toolset 고정 diff

- Reviewer: Codex
- Reviewed baseline: `672182e`
- Reviewed head/worktree: `672182e` 위 미커밋 diff 6개 파일
- Verdict: Request changes

## Independent checks

| Check | Result | Evidence |
|---|---|---|
| 대상 diff 직접 확인 | 완료 | `git diff` 로 6개 파일의 실제 변경을 읽었다 |
| 현재 본문 대조 | 완료 | 대상 6개 파일을 줄번호와 함께 읽고, 주장 검증에 필요한 [modern-iocp/CMakeLists.txt](D:/GameProjects/Portfolio/modern-iocp/CMakeLists.txt:1), [modern-iocp/scripts/phase1.ps1](D:/GameProjects/Portfolio/modern-iocp/scripts/phase1.ps1:1) 도 확인했다 |
| CMake 공식 문서 대조 | 완료 | preset 상속 동작은 맞고, Windows SDK 고정 방식은 현재 문서 주장과 어긋난다는 것을 공식 문서로 확인했다 |

## Findings

### [P1 차단] Windows SDK `10.0.26100.0` 은 현재 preset 으로 실제 고정되지 않는다

- File/line: [modern-iocp/CMakePresets.json](D:/GameProjects/Portfolio/modern-iocp/CMakePresets.json:24), [modern-iocp/docs/infra/build-system.md](D:/GameProjects/Portfolio/modern-iocp/docs/infra/build-system.md:55), [modern-iocp/docs/infra/build-system.md](D:/GameProjects/Portfolio/modern-iocp/docs/infra/build-system.md:70), [modern-iocp/README.md](D:/GameProjects/Portfolio/modern-iocp/README.md:45), [.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md](D:/GameProjects/Portfolio/.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:122)
- Evidence or reproduction: preset 이 고정하는 SDK 값은 `cacheVariables.CMAKE_SYSTEM_VERSION=10.0.26100.0` 뿐이다. 그런데 top-level [modern-iocp/CMakeLists.txt](D:/GameProjects/Portfolio/modern-iocp/CMakeLists.txt:1) 는 `cmake_minimum_required(VERSION 3.28)` 이고, CMake 공식 문서상 이 호출은 3.28 이하 정책을 `NEW` 로 올린다. `CMP0149` 의 `NEW` 동작에서는 Visual Studio generator 가 exact SDK 선택에 `CMAKE_SYSTEM_VERSION` 을 쓰지 않고, exact pin 은 `CMAKE_GENERATOR_PLATFORM` 의 `version=` 으로 해야 한다. 그래서 러너에서 `10.0.26100.0` 이 사라져도 configure 가 최신 설치 SDK 로 조용히 넘어갈 수 있다.
- Impact: 질문 3의 핵심 의도인 "SDK 가 없으면 configure 즉시 실패"가 현재는 성립하지 않는다. 동시에 README/build-system/task 문서의 "SDK 가 기계적으로 고정돼 있다"는 서술도 사실이 아니다. S1 재현성 주장에 직접 걸리는 차단 이슈다.
- Suggested safe direction: exact SDK 를 `CMAKE_GENERATOR_PLATFORM` 의 `version=10.0.26100.0` 로 옮기거나, 의도적으로 구식 동작을 택할 거면 `CMP0149` 를 `project()` 전에 명시적으로 다뤄야 한다. 그리고 CI 로그에는 설치 목록만이 아니라 configure 가 실제 선택한 `CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION` 도 남기는 편이 맞다.

### [P2 수정 권고] toolset 은 `14.51` 계열만 고정돼 있고, 문서는 `14.51.36231` 동일성을 더 강하게 주장한다

- File/line: [modern-iocp/CMakePresets.json](D:/GameProjects/Portfolio/modern-iocp/CMakePresets.json:19), [modern-iocp/docs/infra/build-system.md](D:/GameProjects/Portfolio/modern-iocp/docs/infra/build-system.md:67), [modern-iocp/docs/infra/ci.md](D:/GameProjects/Portfolio/modern-iocp/docs/infra/ci.md:51), [modern-iocp/README.md](D:/GameProjects/Portfolio/modern-iocp/README.md:51), [.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md](D:/GameProjects/Portfolio/.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:183)
- Evidence or reproduction: preset 값은 `v145,version=14.51` 이다. CMake 공식 문서상 `version=` 에 3-component 값도 줄 수 있으므로, 현재 설정은 exact `14.51.36231` 이 아니라 `14.51.x` 라인만 고정한 것이다. Microsoft 문서도 `14.xx` 는 최신 설치 `14.xx.yyyyy` 를 고른다고 설명한다.
- Impact: 현재 기계가 보장하는 것은 "로컬/CI 가 같은 `14.51` 계열을 쓴다"까지다. 문서처럼 exact `14.51.36231` 동일성이나 "누가 clone 해도 같은 결과"를 compiler patch 수준까지 읽히게 쓰면 과장이다. 기능상 즉시 깨지는 문제는 아니지만, 재현성 서술은 한 단계 낮춰야 정직하다.
- Suggested safe direction: 둘 중 하나를 택해야 한다. exact patch 재현성이 목표면 `14.51.36231` 까지 고정하고 `doctor` 도 같이 맞춘다. 운영상 `14.51` 라인 고정이면 충분하다는 판단이면 README/task/docs 문구를 그 수준으로 낮춘다.

### [P2 수정 권고] 완료 상태와 CI 검증 상태를 문서가 서로 다르게 말한다

- File/line: [modern-iocp/README.md](D:/GameProjects/Portfolio/modern-iocp/README.md:26), [.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md](D:/GameProjects/Portfolio/.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:3), [.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md](D:/GameProjects/Portfolio/.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:203)
- Evidence or reproduction: README 는 아직도 `GitHub Actions` 를 "미검증 — 원격 저장소 미연결"로 적고 있다. 반면 `docs/infra/ci.md` 는 run #1 green 이라고 쓴다. task 문서는 status 줄에서 "완료 조건 전부 충족"이라고 적으면서도, 같은 문서 Verification 표에서는 pinned preset 으로 도는 CI run #2 가 아직 미실행이라고 인정한다.
- Impact: 질문 5, 6에 대한 답으로, 지금 상태는 "CI 첫 실행은 확인됐지만 이번 고정 diff 자체는 원격 미재확인"이 맞다. `Review requested` 라벨 자체는 정직하지만, 그 뒤의 "완료 조건 전부 충족"은 아직 아니다. 저장소 규범의 "미검증을 검증으로 쓰지 않는다"에 걸린다.
- Suggested safe direction: `Review requested` 는 유지해도 되지만, status 설명은 "CI 첫 실행 green; pinned preset diff 리뷰/CI 재확인 대기" 정도로 낮추는 게 맞다. README 상태표도 현재 사실에 맞게 갱신해야 한다.

## Requested-point audit

- 1. `inherits` 로 묶은 판단 자체는 맞다. CMake preset 상속 규칙상 child 는 `generator`·`architecture`·`toolset`·`cacheVariables` 를 그대로 받고, [modern-iocp/CMakePresets.json](D:/GameProjects/Portfolio/modern-iocp/CMakePresets.json:34) 의 `binaryDir` 만 안전하게 덮어쓴다. `buildPresets`/`testPresets` 의 `configurePreset` 이름 참조도 의도대로 동작한다. 이 부분에 숨은 부작용은 보지 못했다.
- 2. 다만 그 상속으로 물려받는 toolset 고정의 강도는 `14.51.x` 까지다. exact patch 재현성을 주장하려면 현재 설정은 부족하다.
- 3. 실패 방향은 반반이다. VS 2026 generator/toolset 부재는 fail-fast 쪽으로 읽힌다. 반대로 SDK `10.0.26100.0` 부재는 현재 설정만으로는 fail-fast 가 아니다. 이게 P1이다.
- 4. workflow 의 "툴체인 기록" 단계는 설치된 VS/default toolset/SDK 디렉터리 목록을 남긴다는 점에서 진단용으로는 유용하다. `Windows Kits\10\Include` 경로 자체가 없으면 그 단계가 즉시 실패하므로 그 상황도 잡는다. 하지만 CMake 가 실제로 어떤 SDK 를 선택했는지는 남기지 않으므로, silent fallback 진단에는 충분하지 않다.
- 5. 문서 불일치는 세 군데가 핵심이다. README 의 GitHub Actions 상태표, build-system.md 의 SDK fail-fast 서술, task 문서의 "완료 조건 전부 충족" 문구다.
- 6. S1 acceptance 기준을 이 diff 상태까지 포함해 말하면 아직 닫히지 않았다. run #1 은 이전 CI preset 에 대한 green 이고, 이번 pin diff 는 로컬 검증만 있다. 그래서 `Review requested` 는 가능하지만 "전부 충족"은 과대 주장이다.

## Acceptance criteria audit

- [ ] 각 완료 조건을 독립 확인했다. SDK exact pin/fail-fast 와 pinned-preset CI green 은 현재 증거가 부족하다.
- [x] 불변식과 실패 경로를 확인했다.
- [x] 구현자가 생략한 검사를 검토했다.
- [x] 미커밋 사용자 변경을 침범하지 않았다.

## Residual risks

- SDK pin 방식을 바로잡은 뒤에는 CI run #2 를 다시 돌려야 한다. 그 전에는 "로컬/CI 가 같은 결과를 낸다"는 S1 핵심 문장을 닫으면 안 된다.
- toolset patch 를 exact 로 안 묶을 경우, 그 선택은 허용 가능한 운영 타협일 수는 있어도 문서에 그렇게 써야 한다.
- 공개 저장소 관점의 민감 정보 노출은 이번 diff 에서 보지 못했다.

## External references

- CMake preset inheritance: https://cmake.org/cmake/help/v3.30/manual/cmake-presets.7.html
- `cmake_minimum_required()` policy behavior: https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html
- `CMP0149` and Windows SDK selection: https://cmake.org/cmake/help/v4.1/policy/CMP0149.html
- `CMAKE_GENERATOR_PLATFORM` `version=` exact SDK selection: https://cmake.org/cmake/help/latest/variable/CMAKE_GENERATOR_PLATFORM.html
- MSVC toolset version granularity in CMake: https://cmake.org/cmake/help/latest/variable/CMAKE_VS_PLATFORM_TOOLSET_VERSION.html
- Microsoft toolset selection examples (`14.xx` vs `14.xx.yyyyy`): https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170