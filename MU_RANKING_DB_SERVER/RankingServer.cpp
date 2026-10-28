//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RankingServer.h"
#include "WZTCPSocket.h"
#include "EventServerSocket.h"
#include "RankingLock.h"

HINSTANCE hInst;
CLogProc gLogProc(20, 20, NULL);
CLogToFile LOG_POINT("LOG_POINT", ".\\LOG_POINT");
CLogToFile LOG_ERROR("LOG_ERROR", ".\\LOG_ERROR");
CEventServerSocket g_IOCPSocket;
CWZTCPSocket g_RankServer;
CRankingLock g_RankLOCK;
HWND hAboutBoxDlg;

BOOL CALLBACK	RankingDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void InitRankingServer(HWND);

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	hInst = hInstance;
	return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_RANKINGSERVER_DIALOG), 0, (DLGPROC)RankingDlgProc, 0);
}

BOOL CALLBACK RankingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:  // 0x110
        InitRankingServer(hDlg);

        if (!g_IOCPSocket.CreateIOCompletionPort())
        {
            gLogProc.LogAddFunc("------------- Starting Event Server failed -------------");
            LOG_ERROR.Output("------------- Starting Event Server failed -------------");
        }

        SetTimer(hDlg, 1, 1000, NULL);
        g_RankServer.Listen(hDlg, DEFAULT_RANKSERVER_PORT2);  // 0xADA9 = 44457
        return TRUE;

    case WM_DESTROY:  // 2
        return TRUE;

    case WM_PAINT:  // 0xF
        gLogProc.LogTextPaintProc(hDlg);
        return FALSE;  // Let DefDlgProc handle it too

    case WM_CLOSE:  // 0x10
        if (MessageBoxA(hDlg, "Are You Sure to Terminate Event Server ?", "Ask Terminate Server", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
            EndDialog(hDlg, 0);
        return TRUE;

    case WM_COMMAND:  // 0x111 (273)
        switch (wParam)
        {
        case IDCANCEL:  // 2
            return TRUE;
        case IDOK:  // 1000
            return TRUE;
        case IDM_ABOUT:  // IDM_ABOUT
            hAboutBoxDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About, 0);
            return TRUE;
        default:
            return FALSE;
        }

    case WM_TIMER:  // 0x113 (275)
        if (wParam == 1)
        {
            int uValue = g_IOCPSocket.GetServerCount();
            SetDlgItemInt(hDlg, 1001, uValue, FALSE);
        }
        return TRUE;

    default:
        return FALSE;
    }
}

// Message handler for about box.
BOOL CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
		{
			char szVersion[256];
			wsprintf(szVersion, "Version : %s", RANKING_VERSION);

			HWND hVersion = GetDlgItem(hDlg, IDE_VERSION);
			SetWindowText(hVersion, szVersion);

			hVersion = GetDlgItem(hDlg, IDE_COUNTRY);
			SetWindowText(hVersion, "Country : " RANKING_COUNTRY);
		}
        return (BOOL)TRUE;

	case WM_DESTROY:
		return TRUE;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);
			HDC hcdc = CreateCompatibleDC(hdc);
			HBITMAP hWzLogo = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP_LOGO));
			HGDIOBJ hObj = SelectObject(hcdc, hWzLogo);
			TransparentBlt(hdc, 123, 52, 140, 80, hcdc, 0, 0, 106, 68, TRANSPARENT);
			SelectObject(hcdc, hObj);
			DeleteObject(hWzLogo);
			DeleteDC(hcdc);
			EndPaint(hDlg, &ps);
		}
		return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam) )
        {
		case IDOK:
			return TRUE;
		case IDCANCEL:
			DestroyWindow(hDlg);
			return TRUE;
		default:
			return TRUE;
        }
        return TRUE;

	case WM_LBUTTONDOWN:
		DestroyWindow(hDlg);
		return TRUE;
    }
    return (BOOL)FALSE;
}


void InitRankingServer(HWND hWnd)
{
    char szBuffStr[256];
    SYSTEMTIME sysTime;

    // Set the icon
    SendMessage(hWnd, WM_SETICON, 0, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_RANKINGSERVER)));

    // Create the logo bitmap (this also sets m_hWnd)
    gLogProc.CreateBitmap(hWnd);
    gLogProc.IncDisplaySize(-20, -70);
    gLogProc.Init(TRUE);

    // Build the window title
    GetWindowText(hWnd, szBuffStr, _countof(szBuffStr));
    GetLocalTime(&sysTime);

    wsprintf(szBuffStr, "%s (Version : %s, Start Date : %d-%d-%d %02d:%02d:%02d.%03d) ",
        szBuffStr, RANKING_VERSION,
        sysTime.wYear, sysTime.wMonth, sysTime.wDay,
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);

    strcat(szBuffStr, " [DEVIL SQUARE]");
    strcat(szBuffStr, " [BLOOD CASTLE]");

    SetWindowText(hWnd, szBuffStr);
}