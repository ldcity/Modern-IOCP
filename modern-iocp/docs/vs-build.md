# Visual Studio 빌드·디버그

Visual Studio 2026 (toolset v145 / 14.51.36231), Windows SDK 10.0.26100.0 기준.

명령줄 빌드는 `..\scripts\phase1.ps1 build`. 이 문서는 IDE 로 여는 방법만 다룬다.

## 사전 조건

```powershell
.\scripts\phase1.ps1 doctor
```

`VCPKG_ROOT` 환경변수를 읽지 못하면 CMake 가 vcpkg 툴체인을 찾지 못한다.
환경변수는 프로세스 시작 시점에 읽히므로, 설정한 뒤 **Visual Studio 를 재시작**해야 한다.

## 방법 1 — 폴더 열기 (권장)

`파일 > 열기 > 폴더` → `D:\GameProjects\Portfolio\modern-iocp`

Visual Studio 가 `CMakePresets.json` 을 읽어 구성 드롭다운에 `x64-release` 를 표시한다.
`CMakeLists.txt` 를 저장하면 자동으로 재구성된다.

이쪽을 권하는 이유는 `CMakeLists.txt` 가 정본이라는 사실이 흐려지지 않기 때문이다.

## 방법 2 — 생성된 솔루션 열기

`.\scripts\phase1.ps1 build` 를 한 번 실행하면 솔루션이 생성된다.

```
build\x64-release\modern_iocp.slnx
```

🔴 **확장자가 `.slnx` 다.** Visual Studio 2026 이 쓰는 XML 기반 새 솔루션 형식이며,
CMake 가 이 형식으로 생성한다. 예전 `.sln` 을 찾으면 없다.

솔루션 탐색기에 다음이 보인다.

| 프로젝트 | 산출물 |
|---|---|
| `Core` | `Core.lib` (정적 라이브러리) |
| `GameServer` | `GameServer.exe` — `Core.lib` 링크 |
| `Tests` | `Tests.exe` — `Core.lib` + gtest 링크 |
| `ALL_BUILD` | CMake 가 만드는 "전부 빌드" 가상 프로젝트 |
| `ZERO_CHECK` | `CMakeLists.txt` 변경을 감지해 솔루션을 재생성 |

파일은 `Core` / `GameServer` / `Tests` 폴더로 나뉘어 보인다.
`CMakeLists.txt` 의 `source_group(TREE ...)` 가 디스크 구조를 그대로 반영한 결과다.

🔴 **`.vcxproj` 를 직접 수정하지 않는다.** CMake 가 생성하는 파일이라 다음 실행 때
덮어써진다. VS 의 `추가 > 새 항목` 으로 넣은 파일은 사라진다.
파일을 추가할 때는 `CMakeLists.txt` 의 소스 목록에 이름을 적는다. 저장하면 `ZERO_CHECK` 가
감지해 솔루션을 갱신한다.

## 빌드와 디버그

- 전체 빌드: `Ctrl+Shift+B`
- 서버 디버그: `GameServer` 우클릭 → 시작 프로젝트로 설정 → `F5`
- 테스트 디버그: `Tests` 를 시작 프로젝트로 설정 → `F5`

구성은 `Release` 를 쓴다. `/Zi` 와 링커 `/DEBUG` 를 걸어 Release 에서도 PDB 가 나오므로
중단점과 호출 스택이 동작한다. 단 최적화가 켜져 있어 인라인된 함수는 단계 실행이
건너뛰어지고 일부 지역 변수는 "최적화되어 사용할 수 없습니다" 로 표시된다.

## 🔴 `--crash-test` 는 VS 에서 확인하지 않는다

디버거가 붙어 있으면 예외를 **디버거가 먼저 가로챈다.**
`SetUnhandledExceptionFilter` 로 등록한 핸들러가 호출되지 않고 덤프 파일도 생기지 않는다.

크래시 경로는 명령줄에서 확인한다.

```powershell
.\scripts\phase1.ps1 crash        # 덤프 생성
.\scripts\phase1.ps1 verify-dump  # 덤프 + 짝 PDB 를 exit code 로 판정
```

생성된 덤프를 여는 방법은 `dump-analysis-00-plumbing.md` 를 참조한다.

## 확인 기록

2026-08-30, Visual Studio 2026 / `x64-release` preset / Release 구성에서
`GameServer` 를 시작 프로젝트로 두고 중단점이 걸리는 것을 확인했다.
Release 빌드에 `/Zi` 와 링커 `/DEBUG` 를 건 결과다.
