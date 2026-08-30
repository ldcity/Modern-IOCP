# CI

대상 파일: `../../.github/workflows/ci.yml` (저장소 루트)

🔴 **워크플로 파일은 저장소 루트의 `.github/workflows/` 에 있어야 한다.** GitHub 은 다른
위치를 인식하지 않는다. 이 저장소의 루트는 `Portfolio/` 이므로 `modern-iocp/` 아래가 아니다.

## 상태

**미검증.** 원격 저장소가 없어 아직 한 번도 실행되지 않았다. 로컬에서 CI 가 실행할 명령
3개를 CI preset 으로 돌려 통과하는 것만 확인했다. 원격에 push 하고 green 을 봐야
S1 완료 조건이 충족된다.

## 무엇을 하는가

`windows-2025` 러너에서:

1. 저장소 체크아웃
2. **vcpkg 를 baseline 커밋으로 직접 체크아웃** + 부트스트랩
3. 툴체인 버전 기록
4. `phase1.ps1 build -Preset x64-release-ci`
5. `phase1.ps1 test -Preset x64-release-ci`
6. `phase1.ps1 verify-dump -Preset x64-release-ci`
7. `.dmp` 와 `.pdb` 를 artifact 로 업로드

## 왜 vcpkg 를 직접 받는가

러너에도 vcpkg 가 미리 깔려 있다. 그러나 버전이 다르면 `vcpkg.json` 의 `builtin-baseline`
커밋을 찾지 못하고, **다른 gtest 버전이 설치된다.** baseline 을 고정한 의미가 사라진다.

```yaml
- uses: actions/checkout@v4
  with:
    repository: microsoft/vcpkg
    ref: 114d9fe62faf35856b45cf55cb93b57028a45d63
    path: vcpkg
```

🔴 이 `ref` 는 `modern-iocp/vcpkg.json` 의 `builtin-baseline` 과 **같아야 한다.**
한쪽만 바꾸면 CI 와 로컬이 다른 라이브러리를 쓰게 된다.

## 툴체인 기록 단계

로컬은 VS 2026 / toolset v145 로 고정돼 있지만 러너에는 그 버전이 없을 가능성이 높다.
그래서 CI preset 은 제너레이터·toolset·SDK 를 고정하지 않는다.

대신 러너의 실제 버전을 로그에 남긴다.

```yaml
- name: 툴체인 기록
  run: |
    cmake --version
    & $vswhere -latest -products * -property displayName
    Get-Content "...\Microsoft.VCToolsVersion.default.txt"
```

**첫 실행 로그를 보고 CI preset 에도 고정하는 것이 다음 단계다.** 지금 추측으로 박으면
틀렸을 때 원인을 찾기 어렵다.

## `verify-dump` 를 CI 에서 실제로 돌리는 이유

빌드가 green 이어도 크래시 경로가 죽어 있을 수 있다. 그러면 "덤프 배관을 S1 에서 끝냈다"는
판정이 성립하지 않는다. Codex 리뷰가 P1 으로 지적한 부분이다.

CI 가 실제로 크래시를 내고, 덤프가 생기고, 짝 PDB 가 있는지를 exit code 로 판정한다.

## artifact 에 PDB 를 함께 올리는 이유

덤프에는 주소만 있다. 함수 이름으로 바꾸려면 **그 빌드의** PDB 가 필요하다.
덤프만 받아두면 나중에 열어도 심볼이 붙지 않는다.

```yaml
if-no-files-found: error
```

파일이 없으면 조용히 넘어가지 않고 실패한다. 빈 artifact 가 올라가면 "덤프가 생겼다"는
착각을 준다.

`if: always()` 라서 앞 단계가 실패해도 업로드를 시도한다 — 실패했을 때 증거가 더 필요하다.

## 경로 필터

```yaml
on:
  push:
    paths:
      - 'modern-iocp/**'
      - '.github/workflows/ci.yml'
```

문서만 고쳤을 때 빌드를 돌리지 않는다. 이 저장소에는 `seamless-world/` 도 있는데
그쪽은 아직 코드가 없다.

## 바꾸려면

- vcpkg 버전 올림 → `ref` 와 `modern-iocp/vcpkg.json` 의 `builtin-baseline` 을 **함께**
- 러너 변경 → `runs-on`. 그리고 툴체인 기록 로그를 다시 확인한다
- 새 검증 단계 추가 → `phase1.ps1` 에 서브커맨드를 만들고 여기서 부른다.
  워크플로에 명령을 직접 쓰지 않는다 — 로컬에서 같은 것을 돌릴 수 없게 된다
