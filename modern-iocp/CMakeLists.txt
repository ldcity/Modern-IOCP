cmake_minimum_required(VERSION 3.28)

project(modern_iocp LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "modern-iocp 는 Windows 전용이다 (raw Win32 IOCP).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release 에서도 심볼을 남긴다. S1 의 덤프 plumbing 이 이것에 의존한다.
# /Zi 로 PDB 를 만들고, 링커 /DEBUG 가 꺼버리는 최적화를 /OPT:REF /OPT:ICF 로 되돌린다.
add_compile_options(/W4 /WX /permissive- /utf-8 /EHsc)
add_compile_options($<$<CONFIG:Release>:/Zi>)
add_link_options($<$<CONFIG:Release>:/DEBUG>)
add_link_options($<$<CONFIG:Release>:/OPT:REF>)
add_link_options($<$<CONFIG:Release>:/OPT:ICF>)

# --- 네트워크 코어 (사용자 작성) -------------------------------------------
# 파일이 늘어나면 여기에 명시적으로 추가한다. glob 을 쓰지 않는다.
# 파일명 규칙은 레거시(`NetServer.cpp`·`Session.h`)와 같은 PascalCase 를 따른다.
#
# 헤더는 컴파일 단위가 아니라서 빌드에는 필요 없다. 그래도 여기 적는 이유는
# Visual Studio 솔루션 탐색기에 보이게 하기 위해서다. 안 적으면 IDE 에서 열 수 없다.
set(CORE_SOURCES
    Core/IOCPServer.cpp
    Core/IOCPServer.h
    Core/CrashHandler.cpp
    Core/CrashHandler.h
)

add_library(Core STATIC ${CORE_SOURCES})
target_include_directories(Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# ws2_32: 소켓. dbghelp: MiniDumpWriteDump.
target_link_libraries(Core PUBLIC ws2_32 dbghelp)

# --- 서버 실행 파일 (사용자 작성) ------------------------------------------
set(GAME_SERVER_SOURCES GameServer/main.cpp)
add_executable(GameServer ${GAME_SERVER_SOURCES})
target_link_libraries(GameServer PRIVATE Core)

# --- 테스트 ----------------------------------------------------------------
enable_testing()
find_package(GTest CONFIG REQUIRED)

set(TESTS_SOURCES Tests/SmokeEchoRoundtrip.cpp)
add_executable(Tests ${TESTS_SOURCES})
target_link_libraries(Tests PRIVATE Core GTest::gtest GTest::gtest_main)

# --- IDE 표시 -------------------------------------------------------------
# 기본값은 모든 파일이 "Source Files" 한 폴더에 평평하게 들어간다.
# TREE 를 주면 디스크 구조(Core/, GameServer/, Tests/)가 솔루션 탐색기에 그대로 보인다.
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${CORE_SOURCES}
    ${GAME_SERVER_SOURCES}
    ${TESTS_SOURCES}
)

include(GoogleTest)
# TEST_PREFIX 없이 등록하면 CTest 이름이 `smoke.echo_roundtrip` 형태가 된다
# (GTest suite `smoke`, case `echo_roundtrip`). 완료 조건이 이 이름을 그대로 쓴다.
gtest_discover_tests(Tests
    PROPERTIES LABELS "smoke"
)
