#include "IOCPServer.h"


IOCPServer::IOCPServer()
	: 
	// CreateIoCompletionPort 는 실패 시 nullptr 을 반환한다(INVALID_HANDLE_VALUE 가 아니다).
	// 초기값과 실패값을 같은 것으로 맞춰야 Stop() 에서 한 가지만 검사하면 된다.
	m_iocpHandle(nullptr),
	m_listenSocket(INVALID_SOCKET),
	m_clientSocket(INVALID_SOCKET),
	m_serverPort(0),
	m_running(false)
{

}


IOCPServer::~IOCPServer()
{
	stop();
}

bool IOCPServer::initSocket(uint16_t port)
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup() error: %d\n", nRet);
		return false;
	}

	m_listenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_listenSocket)
	{
		printf("[ERROR] WSASocket() error: %d\n", WSAGetLastError());
		return false;
	}

	m_serverPort = port;

	printf("[INFO] Init success\n");
	return true;
}

bool IOCPServer::bindSocket()
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(m_serverPort);
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(m_listenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		printf("[ERROR] bind() error: %d\n", WSAGetLastError());
		return false;
	}

	// 포트 0 으로 bind 하면 OS 가 빈 포트 선택
	SOCKADDR_IN stBoundAddr = {};
	int nBoundAddrLen = sizeof(stBoundAddr);
	if (SOCKET_ERROR == getsockname(m_listenSocket, (SOCKADDR*)&stBoundAddr, &nBoundAddrLen))
	{
		printf("[ERROR] getsockname() error: %d\n", WSAGetLastError());
		return false;
	}
	m_serverPort = ntohs(stBoundAddr.sin_port);

	printf("[INFO] bind success (port %u)\n", m_serverPort);
	return true;
}

bool IOCPServer::startListen()
{
	int nRet = listen(m_listenSocket, 5);
	if (0 != nRet)
	{
		printf("[ERROR] listen() error: %d\n", WSAGetLastError());
		return false;
	}

	printf("[INFO] listen success\n");
	return true;
}

bool IOCPServer::start(uint16_t port)
{
	if (!initSocket(port))
		return false;

	if (!bindSocket())
		return false;

	if (!startListen())
		return false;

	m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, kWorkerThreadCount);
	if (nullptr == m_iocpHandle)
	{
		// GetLastError() 는 DWORD(unsigned long) 다. %d 로 찍으면 C4477 경고 -> /WX 로 빌드 실패.
		printf("[ERROR] CreateIoCompletionPort() error : %lu\n", GetLastError());
		return false;
	}

	m_running = true;

	for (int workerThreadCount = 0; workerThreadCount < kWorkerThreadCount; workerThreadCount++)
	{
		m_workerThreads.emplace_back([this]() { workerThread(); });
	}

	for (int acceptThreadCount = 0; acceptThreadCount < kAcceptThreadCount; acceptThreadCount++)
	{
		m_acceptThreads.emplace_back([this]() { acceptThread(); });
	}

	return true;
}


bool IOCPServer::postRecv()
{
	ZeroMemory(&m_recvContext.m_wsaOverlapped, sizeof(WSAOVERLAPPED));

	m_recvContext.m_ioOpt = IOOperation::RECV;
	m_recvContext.m_wsaBuf.buf = m_recvContext.m_szBuf;
	m_recvContext.m_wsaBuf.len = kMaxSockBuf;

	DWORD flags = 0;

	int nRet = WSARecv(m_clientSocket,
		&m_recvContext.m_wsaBuf,
		1,
		nullptr,
		&flags,
		&m_recvContext.m_wsaOverlapped,
		nullptr);

	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[ERROR] WSARecv() error : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}


bool IOCPServer::postSend(int bytes)
{
	ZeroMemory(&m_sendContext.m_wsaOverlapped, sizeof(WSAOVERLAPPED));

	m_sendContext.m_ioOpt = IOOperation::SEND;
	memcpy(m_sendContext.m_szBuf, m_recvContext.m_szBuf, bytes);
	m_sendContext.m_wsaBuf.buf = m_sendContext.m_szBuf;
	m_sendContext.m_wsaBuf.len = static_cast<ULONG>(bytes);

	// TODO(S3): 완료 시 bytes 가 요청보다 작을 수 있다. 송신 커서 필요 (레거시 결함 D1)

	int nRet = WSASend(m_clientSocket,
		&m_sendContext.m_wsaBuf,
		1,
		nullptr,
		0,
		&m_sendContext.m_wsaOverlapped,
		nullptr);

	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[ERROR] WSASend() error : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}


void IOCPServer::acceptThread()
{
	printf("[INFO] AcceptThread Start\n");

	while (m_running)
	{
		SOCKET clientSocket = accept(m_listenSocket, nullptr, nullptr);
		if (INVALID_SOCKET == clientSocket)
		{
			if (!m_running)		// stop() 이 listen 소켓을 닫아 깨어난 경우
				break;

			printf("[ERROR] accept() error: %d\n", WSAGetLastError());
			continue;
		}

		m_clientSocket = clientSocket;

		// 세 번째 인자가 GetQueuedCompletionStatus 의 completionKey 로 돌아온다.
		if (nullptr == CreateIoCompletionPort((HANDLE)clientSocket, m_iocpHandle, (ULONG_PTR)this, 0))
		{
			printf("[ERROR] CreateIoCompletionPort() error: %lu\n", GetLastError());
			closeClient();
			continue;
		}

		printf("[INFO] client accepted\n");

		if (!postRecv())
			closeClient();
	}

	printf("[INFO] AcceptThread End\n");
}


void IOCPServer::workerThread()
{
	printf("[INFO] WorkerThread Start\n");

	while (true)
	{
		ULONG_PTR key = 0;
		DWORD transferredBytes = 0;
		OVERLAPPED* pOverlapped = nullptr;

		BOOL bSuccess = GetQueuedCompletionStatus(m_iocpHandle,
			&transferredBytes,
			&key,
			&pOverlapped,
			INFINITE);

		// stop() 이 보낸 종료 신호
		if (bSuccess && 0 == transferredBytes && nullptr == pOverlapped)
			break;

		// 완료 포트가 닫힘
		if (!bSuccess && nullptr == pOverlapped)
			break;

		if (!bSuccess)
		{
			printf("[ERROR] GetQueuedCompletionStatus() error: %lu\n", GetLastError());
			closeClient();
			continue;
		}

		// 상대가 연결을 닫았다. 에러가 아니다
		if (0 == transferredBytes)
		{
			printf("[INFO] client disconnected\n");
			closeClient();
			continue;
		}

		stOverlappedEX* pContext = CONTAINING_RECORD(pOverlapped, stOverlappedEX, m_wsaOverlapped);

		switch (pContext->m_ioOpt)
		{
		case IOOperation::RECV:
			if (!postSend(static_cast<int>(transferredBytes)))
				closeClient();
			break;

		case IOOperation::SEND:
			if (!postRecv())
				closeClient();
			break;
		}
	}

	printf("[INFO] WorkerThread End\n");
}


void IOCPServer::stop()
{
	if (!m_running.exchange(false))
		return;

	// accept 스레드를 깨운다
	if (INVALID_SOCKET != m_listenSocket)
	{
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
	}

	// 한 항목은 한 스레드만 받는다. 워커 수만큼 보내야 전부 깨어난다
	for (size_t i = 0; i < m_workerThreads.size(); i++)
	{
		PostQueuedCompletionStatus(m_iocpHandle, 0, 0, nullptr);
	}

	for (std::thread& thread : m_acceptThreads)
	{
		if (thread.joinable())
			thread.join();
	}

	for (std::thread& thread : m_workerThreads)
	{
		if (thread.joinable())
			thread.join();
	}

	m_acceptThreads.clear();
	m_workerThreads.clear();

	// 스레드가 전부 끝난 뒤에 자원을 해제한다
	closeClient();

	if (nullptr != m_iocpHandle)
	{
		CloseHandle(m_iocpHandle);
		m_iocpHandle = nullptr;
	}

	WSACleanup();

	printf("[INFO] server stopped\n");
}


void IOCPServer::closeClient()
{
	if (INVALID_SOCKET != m_clientSocket)
	{
		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
	}
}