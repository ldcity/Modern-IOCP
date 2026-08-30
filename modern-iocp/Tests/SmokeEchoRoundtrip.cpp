#include <gtest/gtest.h>

#include "Core/IOCPServer.h"

#include <string>

// S1 의 완료 조건 테스트. 서버를 인프로세스로 띄우고 loopback 으로 왕복시킨다.
//
// 별도 프로세스를 띄우지 않는 이유: 프로세스 기동 대기, 포트 전달, 좀비 프로세스 정리가
// 전부 불안정 요소가 된다. 스레드로 띄우면 그 셋이 없어진다.
//
// 이 테스트는 IOCPServer 의 수신·송신 경로가 완성되기 전까지 실패한다. 그게 정상이다.
// "이 테스트를 통과시키는 것" 이 S1 코어 구현의 완료 정의다.
namespace
{
	constexpr int kIoTimeoutMs = 2000;

	// 지정한 바이트 수를 다 받거나 타임아웃까지 반복한다.
	// TCP 는 스트림이라 send 한 덩어리가 recv 한 번에 다 온다는 보장이 없다.
	bool recvExactly(SOCKET sock, char* buffer, int expectedBytes)
	{
		int received = 0;
		while (received < expectedBytes)
		{
			const int n = recv(sock, buffer + received, expectedBytes - received, 0);
			if (n <= 0)
			{
				return false;
			}
			received += n;
		}
		return true;
	}
}

TEST(smoke, echo_roundtrip)
{
	WSADATA wsaData = {};
	ASSERT_EQ(0, WSAStartup(MAKEWORD(2, 2), &wsaData));

	IOCPServer server;
	ASSERT_TRUE(server.start(0)) << "서버가 뜨지 않았다";

	const uint16_t port = server.getServerPort();
	ASSERT_NE(0, port) << "포트 0 으로 bind 했는데 배정된 포트를 못 읽었다";

	const SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT_NE(INVALID_SOCKET, client);

	// 서버가 응답하지 않을 때 테스트가 영원히 멈추지 않도록 타임아웃을 건다.
	DWORD timeout = kIoTimeoutMs;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

	SOCKADDR_IN serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	ASSERT_EQ(1, InetPtonW(AF_INET, L"127.0.0.1", &serverAddr.sin_addr));

	ASSERT_EQ(0, connect(client, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
		<< "connect 실패: " << WSAGetLastError();

	const std::string sent = "hello modern-iocp";
	const int sentBytes = static_cast<int>(sent.size());
	ASSERT_EQ(sentBytes, send(client, sent.data(), sentBytes, 0));

	std::string received(sent.size(), '\0');
	ASSERT_TRUE(recvExactly(client, received.data(), sentBytes))
		<< "echo 응답을 받지 못했다: " << WSAGetLastError();

	EXPECT_EQ(sent, received);

	closesocket(client);
	server.stop();
	WSACleanup();
}
