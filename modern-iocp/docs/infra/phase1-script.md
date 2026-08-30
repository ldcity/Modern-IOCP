# `scripts/phase1.ps1`

대상 파일: `scripts/phase1.ps1`

```powershell
.\scripts\phase1.ps1 <doctor|build|test|run|crash|verify-dump> [-Preset <name>]
```

## 왜 있는가

감사가 레거시에 대해 지적한 것 중 하나가 "리뷰어가 3분 안에 실행할 방법이 없다" 였다.
명령을 외우게 하는 대신 한 단어로 부르게 한다.

🔴 **이 스크립트는 cmake 호출을 감싸기만 한다.** 컴파일 플래그나 경로를 여기서 다시 정의하지
않는다. 빌드 설정의 정본은 `CMakePresets.json` 이다. 같은 값이 두 곳에 있으면 반드시 어긋난다.

## 서브커맨드

| 명령 | 하는 일 |
|---|---|
| `doctor` | 환경 검사. 빌드 전에 부족한 것을 이름으로 알려준다 |
| `build` | configure + build |
| `test` | ctest |
| `run` | 서버 실행 (뒤에 붙인 인자를 그대로 넘긴다) |
| `crash` | 의도적 크래시 |
| `verify-dump` | 덤프 배관을 exit code 로 판정 |

`-Preset` 은 `x64-release`(기본) 또는 `x64-release-ci`.

## `doctor` — 가장 중요한 명령

fresh clone 에서 빌드가 깨졌을 때 **"내 환경 문제인가, 저장소 문제인가"** 를 가르는 것이
목적이다. 이게 구분되지 않으면 원커맨드 목표가 무너진다.

검사 항목: cmake / Visual Studio / MSVC toolset / Windows SDK / `VCPKG_ROOT` / vcpkg baseline.

두 가지 원칙이 있다.

**① 뭉뚱그려 실패하지 않는다.** 무엇이 왜 없는지, 어떻게 고치는지까지 적는다.

```
[FAIL] MSVC toolset 이 14.44 다. CMakePresets.json 은 14.51 계열을 요구한다.
       VS Installer 에서 해당 toolset 을 설치하거나 preset 을 갱신하라.
```

**② 문제를 모아서 한 번에 보여준다.** 하나 고치고 다시 돌리면 다음 게 나오는 방식은
시간을 낭비한다.

### baseline 커밋 검사

```powershell
& git -C $env:VCPKG_ROOT cat-file -e "$baseline^{commit}"
```

`vcpkg.json` 의 `builtin-baseline` 커밋이 로컬 vcpkg 저장소에 **실제로 있는지** 확인한다.
없으면 vcpkg 가 조용히 다른 버전을 설치해 재현성이 깨진다. 이런 문제는 나중에
"왜 CI 랑 결과가 다르지" 로 나타나며 원인 찾기가 어렵다.

## `verify-dump` — S1 완료 조건의 기계적 근거

빌드 green 만으로는 크래시 경로가 살아 있다는 증거가 되지 않는다. 그래서 사람 눈이 아니라
exit code 로 판정한다.

1. `GameServer.exe --crash-test` 실행하고 표준출력을 받는다
2. `[CRASH] dump written: <경로>` 줄을 정규식으로 파싱한다
3. 그 파일이 존재하고 크기가 0 이 아닌지 확인한다
4. 실행 파일과 짝이 되는 `.pdb` 가 있는지 확인한다
5. 하나라도 실패하면 exit 1

🔴 **덤프 파일명을 스크립트가 추측하지 않는다.** 크래시 핸들러가 경로를 출력하고 스크립트가
그것을 읽는다. 파일명 규칙이 바뀌어도 안 깨진다. 대신 **출력 형식이 계약**이므로
`Core/CrashHandler.cpp` 와 이 스크립트를 함께 고쳐야 한다. 양쪽 주석에 적어 두었다.

### `exit 0` 이 왜 명시돼 있는가

크래시한 자식 프로세스의 종료 코드(`0xC0000005`)가 `$LASTEXITCODE` 에 남는다.
명시적으로 `exit 0` 을 하지 않으면 **CI 가 이 성공을 실패로 읽는다.**
`crash` 도 같은 이유로 `exit 0` 한다 — 크래시가 나는 것이 그 명령의 정상 동작이다.

### 실패 경로를 확인했다

PDB 를 잠시 다른 이름으로 바꾸고 돌려서 exit 1 과 원인 메시지가 나오는 것을 눈으로 확인했다
(`.ai/CONSTITUTION.md` 측정 규율: 새 검사를 채택하기 전에 결함을 되돌려 FAIL 을 확인한다).
항상 통과하는 검사는 검사가 아니다.

## 인코딩

이 파일은 **UTF-8 with BOM** 이어야 한다. Windows PowerShell 5.1 은 BOM 이 없으면
`.ps1` 을 cp949 로 읽어 한글이 깨진다. 실제로 처음 작성했을 때 깨졌다.

## 바꾸려면

- 서브커맨드 추가 → `ValidateSet` 에 이름 추가 + `function Invoke-Xxx` + 맨 아래 `switch` 에 등록
- 실행 파일 이름이 바뀜 → `$ServerExe`
- 요구 toolset·SDK 변경 → `$RequiredToolset` · `$RequiredSdk` (`CMakePresets.json` 과 함께)
- 덤프 출력 형식 변경 → `Invoke-VerifyDump` 의 정규식 (`Core/CrashHandler.cpp` 와 함께)
