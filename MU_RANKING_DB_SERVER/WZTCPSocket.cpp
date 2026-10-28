//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "WZTCPSocket.h"

DWORD CWZTCPSocket::s_OldWndProc = NULL;




CWZTCPSocket::CWZTCPSocket()
{
	m_Socket = INVALID_SOCKET;
	m_hWnd = NULL;

	s_OldWndProc = 0;

	WSADATA wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
}


CWZTCPSocket::~CWZTCPSocket()
{
	if (m_Socket != INVALID_SOCKET)
		closesocket(m_Socket);
}


BOOL CWZTCPSocket::Listen(HWND hWnd, WORD wPort)
{
	if (hWnd == NULL)
		return FALSE;

	if (!IsWindow(hWnd))
		return FALSE;

	m_hWnd = hWnd;
	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (m_Socket == INVALID_SOCKET)
		return FALSE;

	SOCKADDR_IN SockAddr;
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.S_un.S_addr = 0;
	SockAddr.sin_port = ntohs(wPort);

	if (bind(m_Socket, (sockaddr*)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR)
	{
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
		return FALSE;
	}

	if ( listen(m_Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
		return FALSE;
	}

	if (WSAAsyncSelect(m_Socket, hWnd, WM_LISTEN_MSG, FD_ACCEPT ) == SOCKET_ERROR)
	{
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
		return FALSE;
	}

	s_OldWndProc = SetWindowLong(hWnd, GWLP_WNDPROC, (LONG)CallBackProc);
	return TRUE;
}


LRESULT CALLBACK CWZTCPSocket::CallBackProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg != WM_LISTEN_MSG)
	{
		return CallWindowProc((WNDPROC)s_OldWndProc, hWnd, nMsg, wParam, lParam);
	}

	switch (LOWORD(lParam))
	{
	case FD_READ:
		{
			char buf[256];
			if (recv(LOWORD(wParam), buf, sizeof(buf), 0) <= 0)
			{
				closesocket(LOWORD(wParam));
				return 0;
			}
		}
		break;
	case FD_CLOSE:
		{
			closesocket(LOWORD(wParam));
		}
		break;
	case FD_ACCEPT:
		{
			SOCKADDR_IN SockAddr;
			int SockAddrLen = sizeof(SockAddr);
			SOCKET s = accept(LOWORD(wParam), (sockaddr*)&SockAddr, &SockAddrLen);
			if ( s == INVALID_SOCKET)
				return 0;
		
			if (WSAAsyncSelect(s, hWnd, WM_LISTEN_MSG, FD_READ | FD_WRITE | FD_CLOSE) == -1)
			{
				closesocket(s);
				return 0;
			}
		}
		break;
	}
	return 0;
}