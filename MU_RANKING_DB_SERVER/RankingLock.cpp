//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RankingLock.h"



CRankingLock::CRankingLock()
{
	m_dwTickCount = 0;
	InitializeCriticalSection(&m_Critical);
}


CRankingLock::~CRankingLock()
{
	DeleteCriticalSection(&m_Critical);
}


BOOL CRankingLock::CheckTimer()
{
	__try
	{
		Lock();
	}
	__except ((GetTickCount() - m_dwTickCount) > MAX_RANK_WAITTIME, EXCEPTION_EXECUTE_HANDLER)
	{
		UnLock();
	}

	UnLock();
	return FALSE;
}


void CRankingLock::Lock()
{
	EnterCriticalSection(&m_Critical);
}

void CRankingLock::UnLock()
{
	LeaveCriticalSection(&m_Critical);
}


void CRankingLock::StartTimer()
{
	Lock();
	m_dwTickCount = GetTickCount();
	UnLock();
}