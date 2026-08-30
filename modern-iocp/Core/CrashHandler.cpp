#include "CrashHandler.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <dbghelp.h>
#include <cstdio>


namespace
{
	bool splitModulePath(wchar_t* outDir, size_t outDirCount, wchar_t* outStem, size_t outStemCount)
	{
		wchar_t modulePath[MAX_PATH] = {};
		const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		if (0 == length || MAX_PATH == length)
		{
			return false;
		}

		wchar_t* lastSeparator = wcsrchr(modulePath, L'\\');
		if (nullptr == lastSeparator)
		{
			return false;
		}
		*lastSeparator = L'\0';

		if (0 != wcscpy_s(outStem, outStemCount, lastSeparator + 1))
		{
			return false;
		}
		wchar_t* extension = wcsrchr(outStem, L'.');
		if (nullptr != extension)
		{
			*extension = L'\0';
		}

		if (0 != wcscpy_s(outDir, outDirCount, modulePath))
		{
			return false;
		}

		return true;
	}

	bool buildDumpDirectory(wchar_t* outDir, size_t outDirCount, wchar_t* outStem, size_t outStemCount)
	{
		wchar_t moduleDir[MAX_PATH] = {};
		if (!splitModulePath(moduleDir, MAX_PATH, outStem, outStemCount))
		{
			return false;
		}

		if (swprintf_s(outDir, outDirCount, L"%s\\dumps", moduleDir) < 0)
		{
			return false;
		}

		if (!CreateDirectoryW(outDir, nullptr) && ERROR_ALREADY_EXISTS != GetLastError())
		{
			return false;
		}

		return true;
	}

	// dumps\<실행 파일 이름>_<pid>_<yyyyMMdd-HHmmss>.dmp
	bool buildDumpPath(wchar_t* outPath, size_t outPathCount)
	{
		wchar_t dumpDir[MAX_PATH] = {};
		wchar_t moduleStem[MAX_PATH] = {};
		if (!buildDumpDirectory(dumpDir, MAX_PATH, moduleStem, MAX_PATH))
		{
			return false;
		}

		SYSTEMTIME now = {};
		GetLocalTime(&now);

		const int written = swprintf_s(
			outPath, outPathCount,
			L"%s\\%s_%lu_%04u%02u%02u-%02u%02u%02u.dmp",
			dumpDir,
			moduleStem,
			GetCurrentProcessId(),
			static_cast<unsigned>(now.wYear), static_cast<unsigned>(now.wMonth), static_cast<unsigned>(now.wDay),
			static_cast<unsigned>(now.wHour), static_cast<unsigned>(now.wMinute), static_cast<unsigned>(now.wSecond));

		return written > 0;
	}

	bool writeMiniDump(EXCEPTION_POINTERS* exceptionInfo, const wchar_t* dumpPath)
	{
		const HANDLE dumpFile = CreateFileW(
			dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (INVALID_HANDLE_VALUE == dumpFile)
		{
			return false;
		}

		MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo = {};
		dumpExceptionInfo.ThreadId = GetCurrentThreadId();
		dumpExceptionInfo.ExceptionPointers = exceptionInfo;
		dumpExceptionInfo.ClientPointers = FALSE;

		// MiniDumpNormal: 예외 정보 + 모든 스레드의 콜스택 + 모듈 목록.
		const BOOL ok = MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			dumpFile,
			MiniDumpNormal,
			&dumpExceptionInfo,
			nullptr,
			nullptr);

		CloseHandle(dumpFile);
		return TRUE == ok;
	}

	LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* exceptionInfo)
	{
		wchar_t dumpPath[MAX_PATH] = {};
		if (buildDumpPath(dumpPath, MAX_PATH) && writeMiniDump(exceptionInfo, dumpPath))
			wprintf(L"[CRASH] dump written: %s\n", dumpPath);
		else
			wprintf(L"[CRASH] dump write failed (GetLastError=%lu)\n", GetLastError());

		fflush(stdout);

		return EXCEPTION_EXECUTE_HANDLER;
	}
}

void installCrashHandler()
{
	SetUnhandledExceptionFilter(onUnhandledException);
}
