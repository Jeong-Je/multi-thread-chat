#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

// 수신받은 메시지를 콘솔에 출력하는 스레드 함수
DWORD WINAPI ThreadReceive(LPVOID pParam);

int _tmain() {
	//윈속 초기화
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		puts("윈속 초기화 실패");
		return 0;
	}

	//소켓 생성
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		puts("소켓 생성 실패");
		return 0;
	}

	//포트 바인딩
	SOCKADDR_IN	svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(32132);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		puts("ERROR: 서버에 연결할 수 없습니다.");
		return 0;
	}

	// 메시지 수신 스레드 생성
	DWORD dwThreadID = 0;
	HANDLE hThread = ::CreateThread(NULL, 0, ThreadReceive, (LPVOID)hSocket, 0, &dwThreadID);
	::CloseHandle(hThread);

	// 메시지 송신
	char szBuffer[128];
	puts("메시지 입력하세요");
	while (1) {
		memset(szBuffer, 0, sizeof(szBuffer));
		gets_s(szBuffer);
		if (strcmp(szBuffer, "EXIT") == 0) break;

		::send(hSocket, szBuffer, strlen(szBuffer) + 1, 0);
	}

	::closesocket(hSocket); // 소켓 닫기
	::Sleep(100); // 스레드 함수 종료되길 기다리는 목적
	::WSACleanup(); // 윈속 해제
	return 0;
}

DWORD WINAPI ThreadReceive(LPVOID pParam) {
	SOCKET hSocket = (SOCKET)pParam;
	char szBuffer[128] = { 0 };
	while (::recv(hSocket, szBuffer, sizeof(szBuffer), 0) > 0) {
		printf("-> %s\n", szBuffer);
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	puts("수신 스레드 종료");
	return 0;
}