//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "LogProc.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


CLogProc::CLogProc(int cX, int cY, HWND hWnd)
	: m_DisplayX(cX) ,	m_DisplayY(cY), m_hWnd(hWnd)
{
	this->logfp = NULL;
	this->m_bLogFile = FALSE;
	this->unk930 = 0;
	this->m_cline = 0;
	this->m_rtDisplay.left = 10;
	this->m_rtDisplay.top = 40;
	this->m_rtDisplay.right = 0;
	this->m_rtDisplay.bottom = 0;
	
	this->m_hWebzenLogo = (HBITMAP)0x0;
	if (hWnd != 0) {
		CreateBitmap();
	}
}

CLogProc::~CLogProc()
{
	Close();

	if (m_hWebzenLogo)
		DeleteObject(m_hWebzenLogo);
}

void CLogProc::Init(BOOL logprint)
{
	m_bPrint = logprint;

	for (int n = 0; n < MAXLOGTEXT; n++)
	{
		memset(LogText[n], 0, MAXLOGTEXTC);
		LogTextLength[n] = 0;
		LogTextViewType[n] = 0;
	}

	if (m_bLogFile)
		LogDataSet();
}


void CLogProc::Close()
{
	if (logfp)
	{
		fclose(logfp);
		logfp = NULL;
	}
}


void CLogProc::CreateBitmap(HWND hWnd)
{
	m_hWnd = hWnd;

	// Call the private version to create the logo bitmap
	CreateBitmap();

	// Add window client size to display rectangle
	RECT Rect;
	GetClientRect(hWnd, &Rect);
	m_rtDisplay.right += Rect.right;
	m_rtDisplay.bottom += Rect.bottom;
}

void CLogProc::SetDisplayCoord(int x, int y)
{
	m_DisplayX = x;
	m_DisplayY = y;
}

void CLogProc::SetDisplayRect(int x, int y)
{
	m_rtDisplay.left = x;
	m_rtDisplay.top = y;
}


void CLogProc::SetDisplaySize(int width, int heigh)
{
	m_rtDisplay.right = width;
	m_rtDisplay.bottom = heigh;
}


void CLogProc::IncDisplaySize(int width, int heigh)
{
	m_rtDisplay.right += width;
	m_rtDisplay.bottom += heigh;
}

char* CLogProc::GetFileName()
{
	return m_szLogFileName;
}


void CLogProc::LogTextAdd(BYTE type, char* msg, int len)
{
	if (len > MAXLOGTEXTC - 1)
		len = MAXLOGTEXTC - 1;

	if (m_cline > MAXLOGTEXT - 1)
		m_cline = 0;

	LogText[m_cline][0] = 0;
	memcpy(&LogText[m_cline], msg, len);
	LogText[m_cline][1 + len] = 0;
	LogText[m_cline][len] = 0;
	LogTextLength[m_cline] = len;
	LogTextViewType[m_cline] = type;
	m_cline++;
}


void CLogProc::Clear()
{
	for (int n = 0; n < MAXLOGTEXT; n++)
	{
		memset(LogText[n], 0, MAXLOGTEXTC);
		LogTextLength[n] = 0;
		LogTextViewType[n] = 0;
	}
}


void CLogProc::LogDataSet()
{
	char szTemp[250];

	tm* today;
	time_t ltime;

	time(&ltime);
	today = localtime(&ltime);

	today->tm_year = today->tm_year + 1900;
	m_LogMYear = today->tm_year;
	m_LogMonth = today->tm_mon + 1;
	m_LogMDay = today->tm_mday;

	wsprintf(szTemp, "log\\%02d%02d%02d.log", m_LogMYear, m_LogMonth, m_LogMDay);

	if (logfp != 0)
	{
		fclose(logfp);
		logfp = 0;
	}

	logfp = fopen(szTemp, "a+t");
	if (logfp == 0)
	{
		LogAddFunc("error-L1 : Log write error.");
	}
	strcpy(m_szLogFileName, szTemp);

}


BOOL CLogProc::LogDateChange()
{
	char szTemp[250];

	tm* today;
	time_t ltime;
	time(&ltime);

	today = localtime(&ltime);
	today->tm_year = today->tm_year + 1900;
	today->tm_mon = today->tm_mon + 1;

	if (today->tm_year <= m_LogMYear)
	{
		if (today->tm_mon <= m_LogMonth)
		{
			if (today->tm_mday <= m_LogMDay)
			{
				return FALSE;
			}
		}
	}

	m_LogMYear = today->tm_year;
	m_LogMonth = today->tm_mon;
	m_LogMDay = today->tm_mday;

	wsprintf(szTemp, "log\\%02d%02d%02d.log", m_LogMYear, m_LogMonth, m_LogMDay);

	if (logfp != 0)
	{
		fclose(logfp);
		logfp = 0;
	}
	logfp = fopen(szTemp, "a+t");
	if (logfp == NULL)
	{
		MsgBox("error-L1 : Log write error.");
		return FALSE;
	}
	strcpy(m_szLogFileName, szTemp);

	return TRUE;
}


void CLogProc::MsgBox(char* szlog, ...)
{
	char szBuffer[512] = "";
	va_list pArguments;
	va_start(pArguments, szlog);
	vsprintf(szBuffer, szlog, pArguments);
	va_end(pArguments);
	MessageBox(NULL, szBuffer, "error", MB_OK | MB_APPLMODAL);
}


void CLogProc::LogAddFunc(char* szLog, ...)
{
	char szBuffer[512] = "";

	va_list pArguments;

	va_start(pArguments, szLog);	// review
	vsprintf(szBuffer, szLog, pArguments);
	va_end(pArguments);
	LogTextAdd(0, szBuffer, strlen(szBuffer));


	if (m_hWnd ) {
		LogTextPaintProc(m_hWnd);
		return;
	}

	if (m_bLogFile && logfp)
		fprintf(logfp, "%s\n", szBuffer);
}


void CLogProc::LogAddFuncColor(BYTE Color, char* szLog, ...)
{
	char szBuffer[1024] = "";
	va_list pArguments;

	va_start(pArguments, szLog);
	vsprintf(szBuffer, szLog, pArguments);
	va_end(pArguments);
	LogTextAdd(Color, szBuffer, strlen(szBuffer));

	if (m_hWnd) {
		LogTextPaintProc(m_hWnd);
		return;
	}

	if (m_bLogFile && logfp)
		fprintf(logfp, "%s\n", szBuffer);
}


void CLogProc::LogAddHeadHexFunc(int Type, char* data, int len)
{
	if (!m_bPrint)
		return;

	LogTextAdd(Type, data, len);

	if (!m_bLogFile)
		return;
	
	if (logfp == 0) {
		return;
	}

	switch (Type)
	{
	case 2:
		fprintf(logfp, "Send:[%d] : (", len);
		break;
	case 3:
		fprintf(logfp, "FDWrite:[%d] : (", len);
		break;
	case 4:
		fprintf(logfp, "Recv:[%d] : (", len);
		break;
	default:
		fprintf(logfp, "%d[%d] : (", Type, len);
		break;
	}

	for (int n = 0; n < len; n++)
	{
		fprintf(logfp, "0x%02x ", *(BYTE*)(data + n));
	}

	fprintf(logfp, ")\n");
}


void CLogProc::LogTextPaintProc(HWND hWnd)
{
	if (!m_bPrint)
		return;

	// Clear the background (no parameter - uses m_hWnd internally)
	LogTextPaintClear();

	HDC hdc = GetDC(hWnd);

	int total = MAXLOGTEXT;    // 25 lines
	int n = m_cline - 1;   // Start from the most recent line

	HDC hdcSrc;
	HGDIOBJ hObj1 = NULL, hObj2 = NULL;
	HBRUSH hBrush = NULL;
	RECT Rect;

	// Logo path: create offscreen DC with white background
	if (m_hWebzenLogo)
	{
		GetClientRect(m_hWnd, &Rect);

		hdcSrc = CreateCompatibleDC(hdc);
		hObj1 = SelectObject(hdcSrc, (HGDIOBJ)m_hWebzenLogo);
		hBrush = CreateSolidBrush(RGB(255, 255, 255));
		hObj2 = SelectObject(hdcSrc, hBrush);
		PatBlt(hdcSrc, 0, 0, Rect.right, Rect.bottom, PATCOPY);
	}
	else
	{
		hdcSrc = hdc;  // No logo - draw directly on window DC
	}

	// Draw each log line
	while (total-- != 0)
	{
		// Set text color based on log type
		switch (LogTextViewType[n])
		{
		case 2:
			SetTextColor(hdcSrc, RGB(255, 0, 0));     // 0xFF
			break;
		case 3:
			SetTextColor(hdcSrc, RGB(0, 100, 0));      // 0x6400
			break;
		case 4:
			SetTextColor(hdcSrc, RGB(0, 0, 255));      // 0xFF0000 (blue in RGB)
			break;
		case 5:
			SetTextColor(hdcSrc, RGB(155, 0, 0));      // 0x9B
			break;
		case 6:
			SetTextColor(hdcSrc, RGB(0, 0, 100));      // 0x640000
			break;
		default:
			SetTextColor(hdcSrc, RGB(0, 0, 0));        // Black
			break;
		}

		// Draw the text if it has content
		if (strlen(LogText[n]) > 1)
		{
			int len = strlen(LogText[n]);
			TextOutA(hdcSrc, m_DisplayX, 15 * total + m_DisplayY, LogText[n], len);
		}

		// Move to previous line (older log entry)
		if (--n < 0)
			n = MAXLOGTEXT - 1;
	}

	// If logo, copy the offscreen DC to the window
	if (m_hWebzenLogo)
	{
		// Use custom display rectangle size if set
		if (m_rtDisplay.right > 0 && m_rtDisplay.bottom > 0)
		{
			Rect.right = m_rtDisplay.right;
			Rect.bottom = m_rtDisplay.bottom;
		}

		BitBlt(hdc, m_rtDisplay.left, m_rtDisplay.top, Rect.right, Rect.bottom,
			hdcSrc, 0, 0, SRCCOPY);

		// Cleanup
		SelectObject(hdcSrc, hObj1);
		SelectObject(hdcSrc, hObj2);
		DeleteObject(hBrush);
		DeleteDC(hdcSrc);
	}

	ReleaseDC(hWnd, hdc);
}


void CLogProc::LogAddTimeDateFunc(char* szLog, ...)
{
	if (!m_bPrint)
		return;

	char szBuffer[1024] = "";
	va_list pArguments;
	tm* today;
	time_t ltime;
	char tmpbuf[512];


	time(&ltime);
	today = localtime(&ltime);

	char noon[2]="";
	if (today->tm_hour > 11)
		strcpy(noon, "p");

	if (today->tm_hour == 0)
		today->tm_hour = 12;

	wsprintf(tmpbuf, "%.8s%s ", asctime(today) + 11, noon);

	va_start(pArguments, szLog);
	vsprintf(szBuffer, szLog, pArguments);
	va_end(pArguments);

	strcat(tmpbuf, szBuffer);

	LogTextAdd(0, tmpbuf, strlen(tmpbuf));

	if (m_bLogFile && logfp)
		fprintf(logfp, "%s\n", &tmpbuf);
}


void CLogProc::LogTextPaintClear()
{
	if (m_hWebzenLogo)
		return;

	if (!m_hWnd)
		return;

	RECT Rect;
	GetClientRect(m_hWnd, &Rect);

	HDC hDC = GetDC(m_hWnd);
	HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
	FillRect(hDC, &Rect, hbr);
	DeleteObject(hbr);
	ReleaseDC(m_hWnd, hDC);
}


void CLogProc::CreateBitmap()
{
	if (!m_hWnd)
		return;

	RECT rt;
	HDC hDC = GetDC(m_hWnd);
	GetClientRect(m_hWnd, &rt);

	if (m_rtDisplay.right > 0 && m_rtDisplay.bottom)
	{
		rt.right = m_rtDisplay.right;
		rt.bottom = m_rtDisplay.bottom;
	}

	m_hWebzenLogo = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
	ReleaseDC(m_hWnd, hDC);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////