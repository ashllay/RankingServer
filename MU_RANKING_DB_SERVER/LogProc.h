//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once



#define MAXLOGTEXT		25
#define MAXLOGTEXTC		80

class CLogProc
{
public:
	CLogProc(int cX, int cY, HWND hWnd);
	virtual ~CLogProc();

	void Close();
	void Init(BOOL logprint);
	void Clear();
	void LogDataSet();
	BOOL LogDateChange();
	void MsgBox(char* szlog, ...);

	void LogAddFunc(char* szLog, ...);
	void LogAddFuncColor(BYTE Color, char* szLog, ...);
	void LogTextPaintProc(HWND hWnd);
	void LogAddHeadHexFunc(int Type, char* data, int len);
	void LogAddTimeDateFunc(char* szLog, ...);
	void LogTextPaintClear();
	void CreateBitmap(HWND hWnd);
	char* GetFileName();
	void SetDisplayCoord(int x, int y);
	void SetDisplayRect(int x, int y);
	void SetDisplaySize(int width, int heigh);
	void IncDisplaySize(int width, int heigh);
private:
	void CreateBitmap();
	void LogTextAdd(BYTE type, char* msg, int len);
private:
	char m_szLogFileName[256];	// 4
	FILE* logfp;	// 104
	int m_cline;	// 108
	char  LogText[MAXLOGTEXT][MAXLOGTEXTC];	// 10C
	short LogTextLength[MAXLOGTEXT];	// 8dC
	BYTE  LogTextViewType[MAXLOGTEXT];	// 90E
	BOOL m_bPrint;	//928
	BOOL m_bLogFile;	//  92C
	int unk930;
	int m_LogMYear;	// 934
	int m_LogMonth; // 938
	int m_LogMDay; // 93C
	int m_DisplayX;	// 940
	int m_DisplayY;	// 944
	RECT m_rtDisplay;	// 948
	HWND m_hWnd;	// 958
	HBITMAP m_hWebzenLogo;	// 95C
};


extern CLogProc gLogProc;

//#define LogAdd		gLogProc.LogAddFunc
//#define LogAddC		gLogProc.LogAddFuncColor
//#define LogAddTD	gLogProc.LogAddTimeDateFunc