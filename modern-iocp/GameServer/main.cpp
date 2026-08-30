#include "Core/IOCPServer.h"
#include "Core/CrashHandler.h"

#include <cstring>

namespace
{
	// 최적화 빌드에서 UB 가 제거될 수 있어 null 역참조 대신 RaiseException 사용
	[[noreturn]] void triggerIntentionalCrash()
	{
		RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
		std::abort();
	}
}

int main(int argc, char* argv[])
{
	installCrashHandler();

	bool crashTest = false;
	uint16_t port = 0;

	for (int i = 1; i < argc; ++i)
	{
		if (0 == std::strcmp(argv[i], "--crash-test"))
		{
			crashTest = true;
		}
		else if (0 == std::strcmp(argv[i], "--port") && i + 1 < argc)
		{
			port = static_cast<uint16_t>(std::atoi(argv[++i]));
		}
	}

	if (crashTest)
	{
		printf("[INFO] triggering intentional crash\n");
		fflush(stdout);
		triggerIntentionalCrash();
	}

	IOCPServer server;
	if (!server.start(port))
	{
		printf("[ERROR] server start failed\n");
		return 1;
	}

	printf("[INFO] listening on port %u. press Enter to stop.\n", server.getServerPort());
	fflush(stdout);
	(void)getchar();

	server.stop();
	return 0;
}
