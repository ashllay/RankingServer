//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#define MAX_RANK_WAITTIME	30 * 1000

class CRankingLock
{
public:
	CRankingLock();
	~CRankingLock();

	void Lock();
	void UnLock();
	void StartTimer();
	BOOL CheckTimer();
private:
	DWORD				m_dwTickCount;
	CRITICAL_SECTION	m_Critical;
};

