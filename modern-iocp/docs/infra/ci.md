# CI

대상 파일: `../../.github/workflows/ci.yml` (저장소 루트)

🔴 **워크플로 파일은 저장소 루트의 `.github/workflows/` 에 있어야 한다.** GitHub 은 다른
위치를 인식하지 않는다. 이 저장소의 루트는 `Portfolio/` 이므로 `modern-iocp/` 아래가 아니다.

## 상태

**green** — 고정된 preset 으로 도는 run #2 (`2f430c0`, 2026-08-31):
[actions/runs/33321582508](https://github.com/ldcity/Modern-IOCP/actions/runs/33321582508).
빌드·테스트·`verify-dump`·artifact 업로드까지 전 스텝 성공했다.
(고정 전 run #1 은 [33319684014](https://github.com/ldcity/Modern-IOCP/actions/runs/33319684014).)

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

러너가 실제로 무엇으로 빌드했는지를 매 실행 로그에 남긴다 — VS displayName·installationVersion,
`Microsoft.VCToolsVersion.default.txt`, 설치된 Windows SDK 목록.

실측값(SDK 행은 목록 출력을 추가한 run #2 부터):

| 항목 | `windows-2025` 러너 | 로컬 |
|---|---|---|
| Visual Studio | Enterprise 2026 / 18.9.12112.369 | Community 2026 |
| MSVC toolset | 14.51.36231 | 14.51.36231 |
| CMake | 4.4.2 | 4.3.3 |
| 설치된 Windows SDK | 10.0.26100.0 | 10.0.26100.0 |

**toolset 이 로컬과 같은 값이었다.** 러너에 VS 2026 이 없을 것이라는 전제가 틀렸으므로,
CI preset 은 고정을 포기하지 않고 `x64-release` 를 상속하는 쪽으로 바꿨다.
무엇이 어느 강도로 고정되는지는 `docs/infra/build-system.md` 에 있다 — 여기서 반복하지 않는다.

CMake 버전은 고정하지 않는다 — `CMakeLists.txt` 의 최소 버전만 만족하면 된다.

이 단계는 preset 고정 이후에도 남긴다. 러너 이미지가 바뀌어 configure 가 실패할 때
**무엇이 사라졌는지**를 이 로그에서만 알 수 있다.

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

`modern-iocp/` 밖(`.ai/`·`seamless-world/`·루트 문서)만 고쳤을 때 빌드를 돌리지 않는다.
`modern-iocp/docs/**` 만 고쳐도 빌드는 돈다 — 필터를 더 잘게 자르면 "코드인데 안 돌았다"가
생길 수 있고, 한 번이 2분이라 그대로 둔다.

## 바꾸려면

- vcpkg 버전 올림 → `ref` 와 `modern-iocp/vcpkg.json` 의 `builtin-baseline` 을 **함께**
- 러너 변경 → `runs-on`. 그리고 툴체인 기록 로그를 다시 확인한다
- 새 검증 단계 추가 → `phase1.ps1` 에 서브커맨드를 만들고 여기서 부른다.
  워크플로에 명령을 직접 쓰지 않는다 — 로컬에서 같은 것을 돌릴 수 없게 된다
