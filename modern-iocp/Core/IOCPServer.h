#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <winSock2.h>
#include <Ws2tcpip.h>
#include <thread>
#include <atomic>
#include <vector>


constexpr int kMaxSockBuf        = 1024;	// buffer size
constexpr int kWorkerThreadCount = 1;		// worker thread count
constexpr int kAcceptThreadCount = 1;		// accept thread count

enum class IOOperation
{
	RECV,
	SEND
};


// WSAOVERLAPPED 구조체 확장
struct stOverlappedEX
{
	WSAOVERLAPPED m_wsaOverlapped;		// Overlapped I/O 구조체
	IOOperation   m_ioOpt;				// 작업 동작 종류
	WSABUF		  m_wsaBuf;				// Overlapped I/O 작업 버퍼
	char          m_szBuf[kMaxSockBuf]; // data buffer
};


class IOCPServer
{
public:
	IOCPServer();
	~IOCPServer();

	IOCPServer(const IOCPServer&) = delete;
	IOCPServer& operator=(const IOCPServer&) = delete;

public:
	bool start(uint16_t port = 0);	// port 0을 주면 OS가 빈 포트를 고름
	void stop();					// blocking. worker가 다 끝나고 반환

	// bind 이후 실제로 배정된 포트.
	// Start() 전에는 0.
	uint16_t getServerPort() const { return m_serverPort; }

private:
	bool initSocket(uint16_t port);
	bool bindSocket();		
	bool startListen();

	void acceptThread();
	void workerThread();

	bool postRecv();
	bool postSend(int bytes);

	void closeClient();

private:
	HANDLE                   m_iocpHandle;

	SOCKET                   m_listenSocket;
	uint16_t                 m_serverPort;

	SOCKET					 m_clientSocket;

	stOverlappedEX			 m_recvContext;
	stOverlappedEX			 m_sendContext;

	std::vector<std::thread> m_workerThreads;
	std::vector<std::thread> m_acceptThreads;

	std::atomic<bool>        m_running;
};