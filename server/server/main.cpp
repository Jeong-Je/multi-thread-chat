#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <windows.h>
#include <list>
#include <iterator>


CRITICAL_SECTION g_cs; // 스레드 동기화 객체
SOCKET g_hSocket; // 서버 리슨 소켓
std::list<SOCKET> g_listClient;

BOOL AddUser(SOCKET hSocket);
BOOL CtrlHandler(DWORD dwType);
void SendMessage(char* pszParam);
DWORD WINAPI ThreadFunc(LPVOID pParam);

int _tmain() {
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("윈속 초기화 오류");
		return 0;
	}

	// 임계영역 객체 생성
	::InitializeCriticalSection(&g_cs);

	//Ctrl+C 이벤트를 감지하는 함수 등록
	if (::SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE) {
		puts("Ctrl+C 처리기 등록 실패");
	}

	// 접속 대기 소켓 생성
	g_hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (g_hSocket == INVALID_SOCKET) {
		puts("접속 대기 소켓 생성 실패");
		return 0;
	}

	// 포트 바인딩
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(32132);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(g_hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR) {
		puts("소켓에 IP와 포트 바인드 실패");
		return 0;
	}

	// 리슨 상태
	if (::listen(g_hSocket, SOMAXCONN) == SOCKET_ERROR) {
		puts("리슨 실패");
		return 0;
	}

	puts("waiting for client...");

	SOCKADDR_IN clientaddr = { 0 };
	int nAddrLen = sizeof(clientaddr);
	SOCKET hClient = 0;
	DWORD dwThreadID = 0;
	HANDLE hThread;

	while ((hClient = ::accept(g_hSocket, (SOCKADDR*)&clientaddr, &nAddrLen)) != INVALID_SOCKET) {
		if (AddUser(hClient) == FALSE) {
			puts("더 이상 클라이언트 연결을 처리 불가");
			CtrlHandler(CTRL_C_EVENT);
			break;
		}

		hThread = ::CreateThread(NULL, 0, ThreadFunc, (LPVOID)hClient, 0, &dwThreadID);

		::CloseHandle(hThread);
	}
}

BOOL AddUser(SOCKET hSocket) {
	::EnterCriticalSection(&g_cs);
	g_listClient.push_back(hSocket);
	::LeaveCriticalSection(&g_cs);

	return true;
}

// Ctrl+C 이벤트를 감지 후 프로그램 종료
BOOL CtrlHandler(DWORD dwType) {
	if (dwType == CTRL_C_EVENT) {
		std::list<SOCKET>::iterator it;

		// 연결된 모든 클라이언트 및 리슨 소켓을 닫고 프로그램 종료
		::shutdown(g_hSocket, SD_BOTH);

		::EnterCriticalSection(&g_cs);
		for (it = g_listClient.begin(); it != g_listClient.end(); it++) {
			::closesocket(*it);
		}
		g_listClient.clear();
		::LeaveCriticalSection(&g_cs);

		puts("모든 클라이언트와 연결 종료");
		::Sleep(100); // 스레드가 종료되기를 기다리는 시간
		::DeleteCriticalSection(&g_cs);
		::closesocket(g_hSocket);

		//윈속 해제
		::WSACleanup();
		exit(0);
		return TRUE;
	}
	return FALSE;
}

void SendMessage(char* pszParam) {
	int nLength = strlen(pszParam);
	std::list<SOCKET>::iterator it;

	::EnterCriticalSection(&g_cs);
	for (it = g_listClient.begin(); it != g_listClient.end(); it++) {
		::send(*it, pszParam, sizeof(char) * (nLength + 1), 0);
	}
	::LeaveCriticalSection(&g_cs);
}

DWORD WINAPI ThreadFunc(LPVOID pParam) {
	char szBuffer[128] = { 0 };
	int nReceive = 0;
	SOCKET hClient = (SOCKET)pParam;

	puts("새 클라이언트 연결");

	while ((nReceive = ::recv(hClient, szBuffer, sizeof(szBuffer), 0)) > 0) {
		puts(szBuffer);
		SendMessage(szBuffer);
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	puts("클라이언트 연결 끊어짐");
	
	::EnterCriticalSection(&g_cs);
	g_listClient.remove(hClient);
	::LeaveCriticalSection(&g_cs);

	::closesocket(hClient);
	return 0;
}