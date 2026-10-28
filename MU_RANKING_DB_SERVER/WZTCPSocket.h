//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once


class CWZTCPSocket
{
public:
	CWZTCPSocket();
	~CWZTCPSocket();

	BOOL Listen(HWND hWnd, WORD wPort);
private:
	SOCKET	m_Socket;
	int		unk4;
	HWND	m_hWnd;
private:
	static DWORD s_OldWndProc;
	static LRESULT CALLBACK CallBackProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
};


extern CWZTCPSocket g_RankServer;