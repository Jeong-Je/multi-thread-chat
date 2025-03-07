#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#include <windows.h>
#include <list>
#include <iterator>


CRITICAL_SECTION g_cs; // ������ ����ȭ ��ü
SOCKET g_hSocket; // ���� ���� ����
std::list<SOCKET> g_listClient;

BOOL AddUser(SOCKET hSocket);
BOOL CtrlHandler(DWORD dwType);
void SendMessage(char* pszParam);
DWORD WINAPI ThreadFunc(LPVOID pParam);

int _tmain() {
	WSADATA wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		puts("���� �ʱ�ȭ ����");
		return 0;
	}

	// �Ӱ迵�� ��ü ����
	::InitializeCriticalSection(&g_cs);

	//Ctrl+C �̺�Ʈ�� �����ϴ� �Լ� ���
	if (::SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE) {
		puts("Ctrl+C ó���� ��� ����");
	}

	// ���� ��� ���� ����
	g_hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (g_hSocket == INVALID_SOCKET) {
		puts("���� ��� ���� ���� ����");
		return 0;
	}

	// ��Ʈ ���ε�
	SOCKADDR_IN svraddr = { 0 };
	svraddr.sin_family = AF_INET;
	svraddr.sin_port = htons(32132);
	svraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(g_hSocket, (SOCKADDR*)&svraddr, sizeof(svraddr)) == SOCKET_ERROR) {
		puts("���Ͽ� IP�� ��Ʈ ���ε� ����");
		return 0;
	}

	// ���� ����
	if (::listen(g_hSocket, SOMAXCONN) == SOCKET_ERROR) {
		puts("���� ����");
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
			puts("�� �̻� Ŭ���̾�Ʈ ������ ó�� �Ұ�");
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

// Ctrl+C �̺�Ʈ�� ���� �� ���α׷� ����
BOOL CtrlHandler(DWORD dwType) {
	if (dwType == CTRL_C_EVENT) {
		std::list<SOCKET>::iterator it;

		// ����� ��� Ŭ���̾�Ʈ �� ���� ������ �ݰ� ���α׷� ����
		::shutdown(g_hSocket, SD_BOTH);

		::EnterCriticalSection(&g_cs);
		for (it = g_listClient.begin(); it != g_listClient.end(); it++) {
			::closesocket(*it);
		}
		g_listClient.clear();
		::LeaveCriticalSection(&g_cs);

		puts("��� Ŭ���̾�Ʈ�� ���� ����");
		::Sleep(100); // �����尡 ����Ǳ⸦ ��ٸ��� �ð�
		::DeleteCriticalSection(&g_cs);
		::closesocket(g_hSocket);

		//���� ����
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

	puts("�� Ŭ���̾�Ʈ ����");

	while ((nReceive = ::recv(hClient, szBuffer, sizeof(szBuffer), 0)) > 0) {
		puts(szBuffer);
		SendMessage(szBuffer);
		memset(szBuffer, 0, sizeof(szBuffer));
	}

	puts("Ŭ���̾�Ʈ ���� ������");
	
	::EnterCriticalSection(&g_cs);
	g_listClient.remove(hClient);
	::LeaveCriticalSection(&g_cs);

	::closesocket(hClient);
	return 0;
}