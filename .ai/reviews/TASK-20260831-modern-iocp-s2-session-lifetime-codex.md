# Review: TASK-20260831-modern-iocp-s2-session-lifetime

1. **P1 — `smoke.start_failure_releases_resources` 는 L2를 판정하지 못한다.**  
증거: [.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md](/D:/GameProjects/Portfolio/.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md):`Acceptance criteria`, `Problem evidence`; [modern-iocp/Core/IOCPServer.cpp](/D:/GameProjects/Portfolio/modern-iocp/Core/IOCPServer.cpp):`IOCPServer::start`, `IOCPServer::initSocket`, `IOCPServer::stop`.  
현재 코드의 L2는 `start()` 중간 실패 시 `m_running` 이 아직 `false` 여서 `stop()` 이 조기 반환하고, 그 결과 `WSACleanup()` 과 `closesocket()` 이 생략되는 경로다. 그런데 문서의 판정은 "같은 프로세스에서 다시 `start()` 성공"뿐이라 가짜 PASS가 가능하다. Winsock은 `WSAStartup`/`WSACleanup` 를 프로세스 내부 참조 카운트로 관리하므로, `WSACleanup()` 누락이 있어도 같은 프로세스의 다음 `WSAStartup()` 이 성공할 수 있다. 포트 충돌로 실패한 두 번째 `start()` 가 잡고 있던 소켓도 bind 전 실패면 이후 재시도를 막지 않는다. 즉 이 테스트는 L2의 핵심 결함을 놓칠 수 있다. 이 항목은 "재시도 성공" 대신 RAII 래퍼의 acquire/release 수지, 혹은 실패 주입점별로 `WSAStartup↔WSACleanup`, `WSASocketW↔closesocket` 짝이 맞는지를 기계적으로 검증하는 방향으로 바꿔야 한다. 참고: Microsoft Learn의 [WSAStartup](https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup), [WSACleanup](https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup).

2. **P1 — `세션 수지 + 크래시 없음` 만으로는 문서의 수명 목표를 증명하지 못한다.**  
증거: [.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md](/D:/GameProjects/Portfolio/.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md):`Goal`, `판정 장치 — 경합을 무엇으로 판정하는가`, `Acceptance criteria`의 `smoke.close_race`.  
문서의 Goal은 "워커가 완료를 처리하는 도중에 다른 스레드가 그 세션을 닫아도 해제된 메모리를 만지지 않는다"인데, 완료 조건은 "크래시 없이 종료 + 생성 수 == 파괴 수"다. 이 조합은 누수와 이중 해제는 잡아도, 이미 해제된 `Session*` 를 워커가 읽었지만 운 좋게 안 터진 경우를 막지 못한다. 또한 `smoke.close_race` 는 "겹치게 만든다"는 보장이 없어 반복 루프만 돌다 PASS 할 수 있다. 이 문서는 목표를 낮추거나, 더 강한 기계 장치를 추가해야 한다. 현실적인 대안은 테스트 훅으로 `GQCS dequeue 직후` 와 `세션 destructor 직전` 을 동기화해, close 요청이 들어와도 destructor 가 outstanding IO ref 해제 전에는 실행되지 않는다는 순서를 결정론적으로 검증하는 것이다.

3. **P1 — AcceptEx 취소·회수는 핵심 난점이라고 적어 놓고도 완료 조건이 없다.**  
증거: [.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md](/D:/GameProjects/Portfolio/.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md):`Invariants and risks`, `Plan`, `Acceptance criteria`; [modern-iocp/Core/IOCPServer.cpp](/D:/GameProjects/Portfolio/modern-iocp/Core/IOCPServer.cpp):`IOCPServer::acceptThread`, `IOCPServer::stop`.  
문서는 스스로 "미완료 AcceptEx 를 어떻게 취소·회수할지가 이 단계의 진짜 어려운 지점"이라고 적고, 그 검증을 `smoke.session_balance` 가 대신한다고 주장한다. 이 논증은 성립하지 않는다. 아직 세션으로 승격되지 않은 pending accept 는 세션 생성/파괴 카운터에 들어가지 않으므로, accept 소켓/OVERLAPPED 누수나 pending accept 회수 실패는 수지가 맞아도 통과할 수 있다. 적어도 "클라이언트 없이 `start()` 후 곧바로 `stop()` 해도 반환한다"와 "pending accept/accept socket 수지 0"을 별도 smoke 로 고정해야 한다. 필요하면 [CancelIoEx](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex) 같은 취소 경로를 문서에서 직접 판정 대상으로 올려야 한다.

4. **P2 — `CLAUDE.md` 참조 경로가 틀려 구현 규범 앵커가 어긋난다.**  
증거: [.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md](/D:/GameProjects/Portfolio/.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md):`Invariants and risks`; [D:\GameProjects\CLAUDE.md](/D:/GameProjects/CLAUDE.md):`§5.3 상태 소유권을 명시한다`.  
문서는 "락을 쓰면 획득 순서를 `Portfolio/CLAUDE.md` 에 `## 락 순서` 로 적는다"고 썼지만, 현재 규범 문서는 저장소 안의 `Portfolio/CLAUDE.md` 가 아니라 루트의 `D:\GameProjects\CLAUDE.md` 다. 지금 상태로는 구현자가 존재하지 않는 파일을 갱신 대상으로 읽을 수 있다. 규범 경로를 실제 파일로 고쳐야 한다.

5. **P3 — grep 기반 완료 조건은 지나치게 형식적이고 우회 가능하다.**  
증거: [.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md](/D:/GameProjects/Portfolio/.ai/tasks/TASK-20260831-modern-iocp-s2-session-lifetime.md):`Acceptance criteria`; [modern-iocp/Core/IOCPServer.h](/D:/GameProjects/Portfolio/modern-iocp/Core/IOCPServer.h):`m_recvContext`, `m_sendContext`; [modern-iocp/Core/IOCPServer.cpp](/D:/GameProjects/Portfolio/modern-iocp/Core/IOCPServer.cpp):`IOCPServer::acceptThread`.  
`grep -n "\baccept(" Core/IOCPServer.cpp` 공백은 블로킹 accept 가 "그 파일에서만" 사라졌음을 뜻할 뿐이고, 다른 파일/헬퍼로 옮겨도 PASS다. `m_recvContext`/`m_sendContext` 부재도 이름만 바꾸면 통과한다. 이 둘은 강한 판정이 아니라 약한 스타일 체크다. `accept` 기준은 최소한 `Core/**` 범위의 negative check 와 `AcceptEx` positive check 를 같이 두는 편이 낫고, context 기준은 현재의 `smoke.multi_session_echo` 가 더 실질적이므로 삭제하거나 구조적 invariant 로 바꾸는 편이 낫다.

추가 확인 사항: L1~L6의 사실 주장은 현재 S1 코드와 대체로 일치한다. 다만 L2는 실제 코드보다 좁다. [modern-iocp/Core/IOCPServer.cpp](/D:/GameProjects/Portfolio/modern-iocp/Core/IOCPServer.cpp):`IOCPServer::start` 에서는 `CreateIoCompletionPort` 실패도 같은 자원 누수 패턴으로 빠진다. Non-goals 는 S3·S4 경계를 대체로 잘 지키고 있고, Phase 1의 D3 계약과도 충돌하지 않는다.

**Verdict: Request changes**