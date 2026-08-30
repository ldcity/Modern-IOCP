# 빌드 시스템

대상 파일: `CMakeLists.txt` · `CMakePresets.json` · `vcpkg.json`

셋은 한 세트다. CMakeLists 가 무엇을 빌드할지, Presets 가 어떤 도구로 빌드할지,
vcpkg.json 이 어떤 외부 라이브러리를 쓸지를 각각 담당한다.

## 왜 CMake 인가

레거시 5종은 Visual Studio `.vcxproj`(XML)가 정본이었다. 그래서 필요한 toolset·SDK 버전이
어디에도 적혀 있지 않았고, 감사 시점에 `v143` 이 없는 PC 에서 **재빌드가 불가능**했다.

CMake 는 `.sln`/`.vcxproj` 를 **생성**한다. 그래서 Visual Studio 로도 열리고 CI 에서도 돈다.
그리고 필요한 버전을 텍스트 파일에 적을 수 있다.

🔴 **`.sln` 과 `.vcxproj` 는 생성물이다.** `build/` 안에 있고 `.gitignore` 로 걸러진다.
VS 에서 `추가 > 새 항목` 으로 파일을 넣으면 다음 CMake 실행 때 사라진다.
파일을 추가할 때는 `CMakeLists.txt` 의 소스 목록에 이름을 적는다.

## `vcpkg.json` — 의존성

```json
"builtin-baseline": "114d9fe62faf35856b45cf55cb93b57028a45d63",
"dependencies": [ "gtest" ]
```

vcpkg 는 C++ 패키지 매니저다. 레거시가 라이브러리 소스와 `.lib` 를 저장소에 통째로 넣어
"어디까지가 내 코드인지" 흐려졌던 문제를 없앤다.

**baseline 이 핵심이다.** vcpkg 저장소의 특정 커밋을 가리키고, 그 시점의 라이브러리 버전
전체가 고정된다. 1년 뒤 clone 해도 같은 gtest 가 설치된다.

개별 의존성에 버전을 따로 적지 않는 이유는 baseline 이 이미 전부 고정하기 때문이다.
두 곳에 적으면 어긋난다.

**gtest 를 고른 이유**는 테스트 이름 규칙이다. `TEST(smoke, echo_roundtrip)` 이 CTest 에
`smoke.echo_roundtrip` 으로 등록되는데, 이 이름이 S1 완료 조건에 그대로 쓰인다.

### 바꾸려면

- 라이브러리 추가 → `dependencies` 배열에 포트 이름 추가
- 라이브러리 버전을 통째로 올리기 → `builtin-baseline` 을 새 커밋으로.
  이때 `.github/workflows/ci.yml` 의 vcpkg 체크아웃 `ref` 도 **같이** 바꿔야 한다

## `CMakePresets.json` — 도구 고정

`cmake` 명령에 옵션을 길게 붙이는 대신 이름으로 부르게 한다. `cmake --preset x64-release`.

preset 은 둘이다.

| | `x64-release` (로컬) | `x64-release-ci` |
|---|---|---|
| 제너레이터 | `Visual Studio 18 2026` | 고정 안 함 |
| toolset | `v145, version=14.51` | 고정 안 함 |
| Windows SDK | `10.0.26100.0` | 고정 안 함 |
| vcpkg baseline | `114d9fe...` | 동일 |
| triplet | `x64-windows-static-md` | 동일 |

🔴 **CI preset 이 toolset 을 고정하지 않는 이유**: GitHub Actions 러너에 VS 2026 이 없을
가능성이 높다. 로컬 preset 을 그대로 쓰면 CI 가 시작조차 못 한다. **의존성은 같고 toolset 만
다르다** — 재현성을 일부 포기한 것이므로 README 에 그 차이를 적는다.

CI 워크플로에 "툴체인 기록" 단계를 넣어 러너의 VS·toolset 을 로그에 남긴다.
그 값을 확인한 뒤 CI preset 에도 고정하는 것이 다음 단계다.

### `$env{VCPKG_ROOT}`

```json
"CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
```

vcpkg 위치를 환경변수로 받는다. 특정 PC 경로를 파일에 박으면 다른 사람이 쓸 수 없다.
`scripts\phase1.ps1 doctor` 가 이 변수를 검사한다.

### `x64-windows-static-md`

vcpkg triplet. gtest 를 **정적 링크**하되 C 런타임은 동적으로 쓴다. 정적이라 실행 파일 옆에
DLL 을 복사할 필요가 없어 테스트와 CI 가 단순해진다.

### 바꾸려면

- VS 를 새 버전으로 올림 → `generator` 와 `toolset` 을 함께 바꾸고,
  `scripts/phase1.ps1` 의 `$RequiredToolset` 도 같이 바꾼다
- Windows SDK 변경 → `CMAKE_SYSTEM_VERSION` 과 `$RequiredSdk`

## `CMakeLists.txt` — 무엇을 빌드하는가

### 타깃 3개

```cmake
add_library(Core STATIC ${CORE_SOURCES})      # Core.lib
add_executable(GameServer ${GAME_SERVER_SOURCES})
add_executable(Tests ${TESTS_SOURCES})
```

`Core` 가 정적 라이브러리이고 나머지가 그것을 링크한다. 실무에서 Core·Data·Packet 을 lib 로
두고 서버들이 쓰는 구조와 같다.

```cmake
target_include_directories(Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(Core PUBLIC ws2_32 dbghelp)
target_link_libraries(GameServer PRIVATE Core)
```

**`PUBLIC` / `PRIVATE` 이 전파를 결정한다.** `Core` 의 `ws2_32` 가 `PUBLIC` 이므로
`Core` 를 링크하는 쪽은 Winsock 을 다시 적을 필요가 없다. include 경로도 같이 따라간다.
`GameServer` 의 `Core` 가 `PRIVATE` 인 것은 최종 산출물이라 더 전파할 곳이 없기 때문이다.

include 루트가 저장소 루트라서 프로젝트를 넘나드는 include 는
`#include "Core/IOCPServer.h"` 형태가 된다. 어느 라이브러리 것인지 코드에서 드러난다.

- `ws2_32` — Winsock2. 소켓과 IOCP
- `dbghelp` — `MiniDumpWriteDump`

### 컴파일 플래그

```cmake
add_compile_options(/W4 /WX /permissive- /utf-8 /EHsc)
```

| 플래그 | 뜻 |
|---|---|
| `/W4` | 경고 수준 4 |
| `/WX` | **경고를 에러로** |
| `/permissive-` | 표준 C++ 강제 |
| `/utf-8` | 소스를 UTF-8 로 읽는다 (레거시의 CP949 주석 깨짐 방지) |
| `/EHsc` | C++ 예외 모델 |

🔴 **`/WX` 는 이 프로젝트의 논지다.** 레거시 결함 D2(`SendPacket()` 의 `return true` 누락)는
MSVC 가 C4715 로 경고하는 항목이다. `/W4 /WX` 가 켜져 있었으면 그 버그는 커밋되지 못했다.
실제로 이 프로젝트를 만들면서 같은 실수(`bindSocket()` 의 반환 누락)를 했고, 빌드가 막혔다.

### Release 에서도 심볼을 남긴다

```cmake
add_compile_options($<$<CONFIG:Release>:/Zi>)
add_link_options($<$<CONFIG:Release>:/DEBUG>)
add_link_options($<$<CONFIG:Release>:/OPT:REF>)
add_link_options($<$<CONFIG:Release>:/OPT:ICF>)
```

- `/Zi` — PDB 생성
- `/DEBUG` — 링커가 PDB 를 연결
- `/OPT:REF` `/OPT:ICF` — 🔴 **`/DEBUG` 를 켜면 링커가 이 둘을 자동으로 끈다.**
  되돌리지 않으면 "Release 인데 최적화가 덜 된 바이너리"가 되고, 나중 성능 측정이 거짓이 된다

덤프 배관 전체가 이 네 줄에 의존한다. 자세한 것은 `../dump-analysis-00-plumbing.md`.

`$<$<CONFIG:Release>:...>` 는 generator expression 으로 "Release 구성일 때만" 이라는 뜻이다.
Visual Studio 는 한 `.sln` 에 Debug/Release 를 함께 담으므로 이런 조건부 표기가 필요하다.

### 소스 목록을 변수로 뽑은 이유

```cmake
set(CORE_SOURCES
    Core/IOCPServer.cpp
    Core/IOCPServer.h
    ...
)
```

**glob(와일드카드)을 쓰지 않는다.** 자동 수집은 편하지만 파일을 추가해도 CMake 가 눈치채지
못해 "왜 내 코드가 안 들어가지" 로 시간을 쓴다.

**헤더를 목록에 넣는 이유**는 빌드가 아니라 IDE 다. 헤더는 컴파일 단위가 아니라서 빌드에는
필요 없지만, 적지 않으면 솔루션 탐색기에 보이지 않는다.

```cmake
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ...)
```

이게 없으면 모든 파일이 "Source Files" 한 폴더에 평평하게 들어간다.
`TREE` 를 주면 디스크 구조(`Core/`, `GameServer/`, `Tests/`)가 그대로 보인다.

### 바꾸려면

- 소스 파일 추가 → 해당 `set(..._SOURCES ...)` 목록에 `.cpp` 와 `.h` 를 함께 추가.
  저장하면 `ZERO_CHECK` 프로젝트가 감지해 `.sln` 을 갱신한다
- 새 라이브러리(예 `Data`) 추가 → `set(DATA_SOURCES ...)` + `add_library` +
  `target_include_directories` + 쓰는 쪽에 `target_link_libraries` 한 줄 +
  `source_group` 의 `FILES` 에 추가
