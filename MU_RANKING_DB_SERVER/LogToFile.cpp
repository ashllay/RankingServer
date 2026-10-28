//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "LogToFile.h"



CLogToFile::CLogToFile(char LogFileName[MAX_PATH] , char LogDirectoryName[MAX_PATH] , BOOL bWithDate )
{
	if (strcmp(LogFileName, "") == 0)
	{
		strcpy(this->m_szLogFileName, DEF_LOG_FILE_NAME);
	}
	else
	{
		strcpy(this->m_szLogFileName, LogFileName);
	}


	if (!strcmp(LogDirectoryName, ""))
	{
		strcpy(this->m_szLogDirectoryName, DEF_LOG_DIRECTORY_NAME);
	}
	else
	{
		strcpy(this->m_szLogDirectoryName, LogDirectoryName);
	}

	this->m_bWithDate = bWithDate;

	CreateDirectory(LogDirectoryName, NULL);

	InitializeCriticalSection(&this->m_critLogToFile);

}


CLogToFile::~CLogToFile()
{
	DeleteCriticalSection(&this->m_critLogToFile);
}


void CLogToFile::Output(char* fmt, ...)
{
	va_list argptr;
	int iChrWritten;
	char szLogFileName[MAX_PATH]; // Is StringZero Terminated

	SYSTEMTIME strSystime;

	EnterCriticalSection(&this->m_critLogToFile);

	va_start(argptr, fmt);
	iChrWritten = vsprintf(this->m_cBUFFER, fmt, argptr);
	va_end(argptr);

	GetLocalTime(&strSystime);

	wsprintf(szLogFileName, "%s\\%s %d-%d-%d.txt",
		m_szLogDirectoryName,
		m_szLogFileName,
		strSystime.wYear,
		strSystime.wMonth,
		strSystime.wDay
	);

	if ((this->m_fLogFile = fopen(szLogFileName, "a+")) == 0)	//flipe +'0
	{
		LeaveCriticalSection(&this->m_critLogToFile);
	}
	else
	{
		if (this->m_bWithDate == 0)
		{
			fprintf(this->m_fLogFile, "%s\n", this->m_cBUFFER);
		}
		else
		{
			fprintf(this->m_fLogFile, "%d:%d:%d  %s\n", strSystime.wHour, strSystime.wMinute, strSystime.wSecond, this->m_cBUFFER);
		}
		fclose(this->m_fLogFile);

		LeaveCriticalSection(&this->m_critLogToFile);
	}
}





