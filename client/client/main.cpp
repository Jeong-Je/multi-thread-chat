#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

// ���Ź��� �޽����� �ֿܼ� ����ϴ� ������ �Լ�
DWORD WINAPI ThreadReceive(LPVOID pParam);

int _tmain() {
	//���� �ʱ�ȭ
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		puts("���� �ʱ�ȭ ����");
		return 0;
	}

	//���� ����
	SOCKET hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
	{
		puts("���� ���� ����");
		return 0;
	}

	//��Ʈ ���ε�
	SOCKADDR_IN	svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(32132);
	svraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (::connect(hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR)
	{
		puts("ERROR: ������ ������ �� �����ϴ�.");
		return 0;
	}

	// �޽��� ���� ������ ����
	DWORD dwThreadID = 0;
	HANDLE hThread = ::CreateThread(NULL, 0, ThreadReceive, (LPVOID)hSocket, 0, &dwThreadID);
	::CloseHandle(hThread);

	// �޽��� �۽�
	char szBuffer[128];
	puts("�޽��� �Է��ϼ���");
	while (1) {
		memset(szBuffer, 0, sizeof(szBuffer));
		gets_s(szBuffer);
		if (strcmp(szBuffer, "EXIT") == 0) break;

		::send(hSocket, szBuffer, strlen(szBuffer) + 1, 0);
	}

	::closesocket(hSocket); // ���� �ݱ�
	::Sleep(100); // ������ �Լ� ����Ǳ� ��ٸ��� ����
	::WSACleanup(); // ���� ����
	return 0;
}

DWORD WINAPI ThreadReceive(LPVOID pParam) {
	SOCKET hSocket = (SOCKET)pParam;
	char szBuffer[128] = { 0 };
	while (::recv(hSocket, szBuffer, sizeof(szBuffer), 0) > 0) {
		printf("-> %s\n", szBuffer);
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	puts("���� ������ ����");
	return 0;
}