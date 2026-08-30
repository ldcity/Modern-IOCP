# Review: TASK-20260829-modern-iocp-s1 — CI toolset 고정 diff (재리뷰)

- Reviewer: Codex
- Reviewed baseline: `672182e`
- Reviewed scope: 현재 `git diff 672182e` 전체, 미추적 `.ai/reviews/TASK-20260829-modern-iocp-s1-ci-toolset-codex.md`
- Verdict: Request changes
- P1 차단 발견은 없다.

## Findings

### [P2 수정 권고] `modern-iocp/docs/infra/build-system.md:121`
이번 수정으로 SDK 고정 방식을 `architecture.value = "x64,version=10.0.26100.0"` 로 옮기고 `CMAKE_SYSTEM_VERSION` 을 제거했는데, 같은 문서의 “바꾸려면” 절은 아직 `Windows SDK 변경 → CMAKE_SYSTEM_VERSION 과 $RequiredSdk` 라고 적고 있다. 이 문서 본문 자체가 바로 위 `modern-iocp/docs/infra/build-system.md:76-91` 에서 왜 `CMAKE_SYSTEM_VERSION` 이 잘못된 고정인지 설명하고 있고, 실제 preset 도 `modern-iocp/CMakePresets.json:14-16` 에서 `architecture` 로 옮겨졌다. 유지보수 지침 한 줄이 이전의 잘못된 방식으로 남아 있는 상태다.

영향은 현재 동작 오작동이 아니라 다음 SDK 변경 때 같은 결함을 재도입할 수 있다는 점이다. 이번 diff의 핵심이 “기계 보장과 문서 주장을 맞추는 것”이었으므로, 이 한 줄은 같이 정리하는 편이 맞다.

## 1회차 발견 해소 여부

1. **P1 SDK 고정 문제 — 해소.** `modern-iocp/CMakePresets.json:14-16` 이 `x64,version=10.0.26100.0` 를 직접 넣고, `modern-iocp/CMakePresets.json:32-33` 의 `x64-release-ci` 가 이를 상속한다. CMake preset 문서상 `architecture` 는 Visual Studio generator 의 `-A` 값에 대응하고, 그 `version=` 필드는 exact Windows SDK 선택에 쓰인다. `CMAKE_SYSTEM_VERSION` 삭제로 인한 실질 부작용도 현재 프로젝트에서는 보이지 않았다. `modern-iocp/CMakeLists.txt:1-66` 와 저장소 검색 기준으로 `_WIN32_WINNT`/`WINVER`/`NTDDI`/`CMAKE_SYSTEM_VERSION` 의존이 없어서, 목표 OS 버전이 달라져 동작이 바뀌는 경로가 현재 범위에는 없다.

2. **P2 toolset patch 동일성 과대주장 — 해소.** 최종 문서는 `modern-iocp/README.md:44-49`, `modern-iocp/docs/infra/build-system.md:54-55`, `modern-iocp/docs/infra/build-system.md:94-100`, `.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:177-178,221-223` 에서 모두 “`14.51` 계열까지만 강제된다”로 낮췄다. 이 결론은 타당하다. CMake 문서는 `version=` 에 3-component toolset 값을 줄 수 있다고만 설명하고, 미설치 patch 불일치를 반드시 오류로 만든다고 약속하지 않는다. 구현자가 적은 `-T v145,version=14.51.99999` configure/build 성공 관측과 현재 문서 강도는 일치한다.

3. **P2 완료 상태/CI 상태 불일치 — 해소.** task 상태는 `.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:3-4` 에서 “고정 diff 는 로컬 검증만 했고 CI run #2 가 남았다”로 낮아졌고, README 상태표는 `modern-iocp/README.md:18-24` 에서 2026-08-30 run #1 링크로 갱신됐다. `modern-iocp/docs/infra/ci.md:10-12` 도 같은 run #1 사실과 맞는다. 이전처럼 문서끼리 서로 다른 상태를 말하지는 않는다.

## Requested-point audit

- `architecture.version=` 이 preset 경로에서도 실제로 `CMAKE_GENERATOR_PLATFORM` 으로 전달되느냐는 질문에는 **예**라고 본다. CMake preset 문서상 `architecture` 는 Visual Studio generator 의 플랫폼 선택 필드이고, `strategy: "set"` 은 그 값을 실제 configure 에 적용한다. `x64-release-ci` 는 `inherits` 로 그 필드를 그대로 받으므로 같은 보장을 가진다.
- `CMAKE_SYSTEM_VERSION` 삭제의 부작용은 현재 프로젝트 범위에서는 **실질적으로 없다**. 이 저장소 안에서 그 변수를 조건 분기나 컴파일 정의에 쓰는 흔적이 없고, 현재 빌드는 SDK 선택을 `version=` 으로 직접 고정한다.
- 미채택 1건은 **사유가 대체로 타당하다**. 지금은 SDK 선택이 `version=` exact pin 이라서 SDK 부재 시 configure 가 곧바로 실패한다. 따라서 `CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION` 추가 로깅은 관측성 강화일 뿐, correctness gap 을 메우는 필수 조건은 아니다. 다만 구현자가 든 “`cl.exe` 경로가 이미 로그에 남는다”는 근거는 SDK 증거라기보다 toolset 증거에 가깝다.
- 새로 생긴 문제는 위 P2 1건뿐이다. `modern-iocp/docs/infra/build-system.md` 의 실측표 자체는 과하지 않다. 이번 변경은 `CMP0149` 와 Visual Studio generator 동작 차이를 다루므로, 그 정도의 관측값은 문서로 남길 가치가 있다.
- 커밋 가능 여부는 **기능적으로는 거의 가능하지만, 이번 diff 성격상 위 문서 한 줄은 같이 고치고 묶는 편이 맞다**고 본다. 그 뒤 남는 주된 위험은 `.ai/tasks/TASK-20260829-modern-iocp-s1-skeleton.md:181,223` 에 적힌 대로, 고정된 preset 으로 도는 GitHub Actions run #2 가 아직 없다는 점뿐이다. 이는 이미 문서에 정직하게 남겨져 있으므로, 현재 상태의 잔여 위험 표기로는 충분하다.