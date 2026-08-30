#pragma once

// exception 발생 시, minidump 생성
// 덤프 위치: 실행 파일 옆 dumps\<실행 파일 이름>_<pid>_<yyyyMMdd-HHmmss>.dmp
// 덤프를 쓰면 표준출력에 다음 한 줄을 남긴다. scripts\phase1.ps1 verify-dump 가 이 줄을
// 파싱해 파일 존재와 대응 PDB 를 판정한다. 형식을 바꾸면 그쪽도 고쳐야 한다.
//     [CRASH] dump written: <전체 경로>
void installCrashHandler();
