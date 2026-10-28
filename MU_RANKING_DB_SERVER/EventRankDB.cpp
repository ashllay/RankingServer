//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "EventServerSocket.h"
#include "EventRankDB.h"
#include "CQuery.h"
#include "RankingLock.h"
#include "RankingServer.h"



CEventRankDB::CEventRankDB()
{
	InitializeCriticalSection(& m_RankCritical );

	m_ConCount = 0;
}


CEventRankDB::~CEventRankDB()
{
	DeleteCriticalSection(&m_RankCritical);

	while (!m_RankQuery.empty())
	{
		CQuery* pRankDB  = m_RankQuery.front();
		if (pRankDB != NULL)
			delete pRankDB;

		m_RankQuery.pop();
	}
}


BOOL CEventRankDB::Connect(DWORD nConnCount, char * szConnStr, char * szUserID, char * szUserPW)
{
	if (nConnCount <= 0)
	{
		if (m_ConCount <= 0)
			nConnCount = m_ConCount = 16;
		else
			nConnCount = m_ConCount;
	}

	if (szConnStr == NULL)	szConnStr = m_szDSNName;
	if (szUserID == NULL)	szUserID = m_szUserID;
	if (szUserPW == NULL)	szUserPW = m_szUserPW;

	for (DWORD i = 0; i < nConnCount; i++)
	{
		CQuery * pRankDB = new CQuery;
		if (pRankDB->Connect(3, szConnStr, szUserID, szUserPW) == FALSE)
		{
			gLogProc.LogAddFuncColor(3, "ODBC CONNECTION FAILED");
			return FALSE;
		}

		m_RankQuery.push(pRankDB);
	}
	return TRUE;
}


void CEventRankDB::DevilSqureScore(LPPMSG_ANS_EVENTUSERSCORE lpMsg)
{
	// Get Any free Query from the memory
	CQuery * pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size() )
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char temp[512];
		wsprintf(temp, "[DEVIL] SERVER:%d-%d, SQUARE:%d, ACCNT:%s, CHAR:%s, CLASS:%d, SCORE:%d",
			lpMsg->ServerCode / 20 + 1, lpMsg->ServerCode % 20 + 1, lpMsg->SquareNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score
		);
		gLogProc.LogAddFunc(temp);
		LOG_POINT.Output(temp);

		char sql[512];
		wsprintf(sql, "EXEC SP_POINT_ACCUMULATION '%d', '%d', '%s', '%s', '%d', '%d'",
			lpMsg->ServerCode / 20 + 1, lpMsg->SquareNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score
		);
		if (pRankDB->Exec(sql) == FALSE)
			LOG_ERROR.Output(sql);

		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}


void CEventRankDB::RankUpdate()
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	if (g_RankLOCK.CheckTimer())
		return;

	g_RankLOCK.StartTimer();
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		gLogProc.LogAddFunc("------------- Mu Ranking Update !! -------------");
		LOG_POINT.Output("------------- Mu Ranking Update !! -------------");

		char sql[512];
		wsprintf(sql, "EXEC SP_RANKING");

		if (pRankDB->Exec(sql) )
		{
			SQLRETURN nRET = pRankDB->Fetch();
			pRankDB->Clear();
			if (nRET == SQL_ERROR)
			{
				pRankDB->Close();
				pRankDB->ReConnect();
			}
		}
		else
		{
			pRankDB->Clear();
		}
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}



void CEventRankDB::BloodCastleScore(LPPMSG_ANS_BLOODCASTLESCORE lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char temp[512];
		wsprintf(temp, "[BLOOD] SERVER:%d-%d, BRIDGE:%d, ACCNT:%s, CHAR:%s, CLASS:%d, SCORE:%d, PCROOM GUID:%d",
			lpMsg->ServerCode / 20 + 1, lpMsg->ServerCode % 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iPCRoomGUID
		);
		gLogProc.LogAddFunc(temp);
		LOG_POINT.Output(temp);

		char sql[512];
		wsprintf(sql, "EXEC SP_POINT_ACCM_BC '%d', '%d', '%s', '%s', '%d', '%d', '%d'",
			lpMsg->ServerCode / 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iPCRoomGUID
		);
		if (pRankDB->Exec(sql) == FALSE)
			LOG_ERROR.Output(sql);

		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}




void CEventRankDB::BloodCastleScore3rd(LPPMSG_ANS_BLOODCASTLESCORE_3RD lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char temp[512];
		wsprintf(temp, "[BLOOD] SERVER:%d-%d, BRIDGE:%d, ACCNT:%s, CHAR:%s, CLASS:%d, SCORE:%d, PCROOM GUID:%d, LEFTTIME:%d",
			lpMsg->ServerCode / 20 + 1, lpMsg->ServerCode % 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iPCRoomGUID, lpMsg->iLeftTime
		);
		gLogProc.LogAddFunc(temp);
		LOG_POINT.Output(temp);

		char sql[512];
		wsprintf(sql, "EXEC SP_POINT_ACCM_BC_3RD '%d', '%d', '%s', '%s', '%d', '%d', '%d', '%d'",
			lpMsg->ServerCode / 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iPCRoomGUID, lpMsg->iLeftTime
		);
		if (pRankDB->Exec(sql) == FALSE)
			LOG_ERROR.Output(sql);

		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}




void CEventRankDB::BloodCastleScore4th(LPPMSG_ANS_BLOODCASTLESCORE_3RD lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char temp[512];
		wsprintf(temp, "[BLOOD] SERVER:%d-%d, BRIDGE:%d, ACCNT:%s, CHAR:%s, CLASS:%d, SCORE:%d, PCROOM GUID:%d, LEFTTIME:%d",
			lpMsg->ServerCode / 20 + 1, lpMsg->ServerCode % 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iPCRoomGUID, lpMsg->iLeftTime
		);
		gLogProc.LogAddFunc(temp);
		LOG_POINT.Output(temp);

		char sql[512];
		wsprintf(sql, "EXEC SP_POINT_ACCM_BC_4TH '%d', '%d', '%s', '%s', '%d', '%d', '%d', '%d'",
			lpMsg->ServerCode / 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iPCRoomGUID, lpMsg->iLeftTime
		);
		if (pRankDB->Exec(sql) == FALSE)
			LOG_ERROR.Output(sql);

		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}



void CEventRankDB::BloodCastleScore5th(LPPMSG_ANS_BLOODCASTLESCORE_5TH lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char temp[512];
		wsprintf(temp, "[BLOOD] SERVER:%d-%d, BRIDGE:%d, ACCNT:%s, CHAR:%s, CLASS:%d, SCORE:%d, LEFTTIME:%d, ALIVEPARTY:%d",
			lpMsg->ServerCode / 20 + 1, lpMsg->ServerCode % 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iLeftTime, lpMsg->iAlivePartyCount
		);
		gLogProc.LogAddFunc(temp);
		LOG_POINT.Output(temp);

		char sql[512];
		wsprintf(sql, "EXEC SP_POINT_ACCM_BC_5TH '%d', '%d', '%s', '%s', '%d', '%d', '%d', '%d'",
			lpMsg->ServerCode / 20 + 1, lpMsg->BridgeNum,
			szAccountID, szGameID, lpMsg->Class, lpMsg->Score, lpMsg->iLeftTime, lpMsg->iAlivePartyCount
		);
		if (pRankDB->Exec(sql) == FALSE)
			LOG_ERROR.Output(sql);

		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}


void CEventRankDB::EVDB_SendEnterCount_BC(CIOStream* lpCXOL, LPPMSG_REQ_BLOODCASTLE_ENTERCOUNT lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	if (lpCXOL == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_SendEnterCount_BC ERROR : lpCXOL == NULL");
		return;
	}

	if (lpCXOL->m_lpSOCKETCLSS == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_SendEnterCount_BC ERROR : lpCXOL->m_lpSOCKETCLSS == NULL");
		return;
	}

	__try
	{
		SOCKET s = lpCXOL->m_Socket;

		PMSG_ANS_BLOODCASTLE_ENTERCOUNT pMsg;

		pMsg.h.c = 0xC1;
		pMsg.h.headcode = 0x0B;
		pMsg.h.size = sizeof(pMsg);
		memcpy(pMsg.AccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(pMsg.GameID, lpMsg->GameID, MAX_IDSTRING);
		pMsg.ServerCode = lpMsg->ServerCode;
		pMsg.iObjIndex = lpMsg->iObjIndex;

		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql, "EXEC SP_LEFT_ENTERCOUNT_BC '%s', '%s', '%d'",
			szAccountID, szGameID, lpMsg->ServerCode
		);
		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
			pRankDB->Clear();
			__leave;
		}

		SQLRETURN nRET = pRankDB->Fetch();
		if (nRET != SQL_NO_DATA && nRET != SQL_ERROR)
			pMsg.iLeftCount = pRankDB->GetInt("LeftEnterCount");
		else
			pMsg.iLeftCount = -1;

		pRankDB->Clear();

		if (lpCXOL->m_lpSOCKETCLSS->Send(s, lpCXOL, &pMsg, sizeof pMsg) == FALSE)
		{
			LOG_ERROR.Output("CEventRankDB::EVDB_SendEnterCount_BC ERROR : SendPacket_TCP FAILED");
		}
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}



void CEventRankDB::EVDB_CheckEnterCount_BC(CIOStream* lpCXOL, LPPMSG_REQ_BLOODCASTLEENTERCHECK lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	if (lpCXOL == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_CheckEnterCount_BC ERROR : lpCXOL == NULL");
		return;
	}

	if (lpCXOL->m_lpSOCKETCLSS == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_CheckEnterCount_BC ERROR : lpCXOL->m_lpSOCKETCLSS == NULL");
		return;
	}

	__try
	{
		SOCKET s = lpCXOL->m_Socket;

		PMSG_ANS_BLOODCASTLEENTERCHECK pMsg;

		pMsg.h.c = 0xC1;
		pMsg.h.headcode = 0x0B;
		pMsg.h.size = sizeof(pMsg);
		memcpy(pMsg.AccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(pMsg.GameID, lpMsg->GameID, MAX_IDSTRING);
		pMsg.ServerCode = lpMsg->ServerCode;
		pMsg.iObjIndex = lpMsg->iObjIndex;
		pMsg.iItemLevel = lpMsg->iItemLevel;
		pMsg.iItemPos =  lpMsg->iItemPos;
		pMsg.iBridgeNumber = lpMsg->iBridgeNumber;
		pMsg.bEnterCheck = TRUE;

		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql, "EXEC SP_CHECK_BC '%s', '%s', '%d'",
			szAccountID, szGameID, lpMsg->ServerCode / 20 + 1
		);
		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
			pRankDB->Clear();
			__leave;
		}

		SQLRETURN nRET = pRankDB->Fetch();
		if (nRET != SQL_NO_DATA && nRET != SQL_ERROR)
			pMsg.bEnterCheck = pRankDB->GetInt("EnterResult");
		else
			pMsg.bEnterCheck = TRUE;

		pRankDB->Clear();

		if (lpCXOL->m_lpSOCKETCLSS->Send(s, lpCXOL, &pMsg, sizeof pMsg) == FALSE)
		{
			LOG_ERROR.Output("CEventRankDB::EVDB_CheckEnterCount_BC ERROR : SendPacket_TCP FAILED");
		}
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}




void CEventRankDB::EVDB_EnterCountOn_BC(CIOStream* lpCXOL, LPPMSG_REQ_BLOODCASTLECOUNTON lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	if (lpCXOL == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_EnterCountOn_BC ERROR : lpCXOL == NULL");
		return;
	}

	if (lpCXOL->m_lpSOCKETCLSS == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_EnterCountOn_BC ERROR : lpCXOL->m_lpSOCKETCLSS == NULL");
		return;
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql, "EXEC SP_ENTER_BC '%s', '%s', '%d'",
			szAccountID, szGameID, lpMsg->ServerCode / 20 + 1
		);
		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
			pRankDB->Clear();
			__leave;
		}
		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}


//////////////////////////////////////////////////////////////////
/// Ranking LOCK Functiosn goes here -> Spliteted inyo  RLOG



void CEventRankDB::EVDB_CheckEnterCount_IllusionTemple(CIOStream* lpCXOL, LPPMSG_REQ_ILLUSION_TMPLE_ENTERCOUNTCHECK lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	if (lpCXOL == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_CheckEnterCount_DS ERROR : lpCXOL == NULL");	// #error on EXE, change DS to IllsuionTemplke
		return;
	}

	if (lpCXOL->m_lpSOCKETCLSS == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_CheckEnterCount_DS ERROR : lpCXOL->m_lpSOCKETCLSS == NULL");// #error on EXE, change DS to IllsuionTemplke
		return;
	}

	__try
	{
		SOCKET s = lpCXOL->m_Socket;

		PMSG_ANS_ILLUSION_TEMPLE_ENTERCOUNTCHECK pMsg;

		pMsg.h.c = 0xC1;
		pMsg.h.headcode = 0x12;
		pMsg.h.size = sizeof(pMsg);
		memcpy(pMsg.AccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(pMsg.GameID, lpMsg->GameID, MAX_IDSTRING);
		pMsg.ServerCode = lpMsg->ServerCode;
		pMsg.iObjIndex = lpMsg->iObjIndex;
		pMsg.iItemLevel = lpMsg->iItemLevel;
		pMsg.iItemPos = lpMsg->iItemPos;
		pMsg.iTempleNumber = lpMsg->iTempleNumber;
		pMsg.bEnterCheck = TRUE;
		
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql, "EXEC SP_CHECK_ILLUSION_TEMPLE '%s', '%s', '%d'",
			szAccountID, szGameID, lpMsg->ServerCode / 20 + 1
		);
		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
			pRankDB->Clear();
			__leave;
		}

		SQLRETURN nRET = pRankDB->Fetch();
		if (nRET != SQL_NO_DATA && nRET != SQL_ERROR)
			pMsg.bEnterCheck = pRankDB->GetInt("EnterResult");
		else
			pMsg.bEnterCheck = TRUE;

		pRankDB->Clear();

		if (lpCXOL->m_lpSOCKETCLSS->Send(s, lpCXOL, &pMsg, sizeof pMsg) == FALSE)
		{
			LOG_ERROR.Output("CEventRankDB::EVDB_CheckEnterCount_DS ERROR : SendPacket_TCP FAILED");// #error on EXE, change DS to IllsuionTemplke
		}
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}



void CEventRankDB::EVDB_EnterCountOn_IllusionTemple(CIOStream* lpCXOL, LPPMSG_REQ_ILLUSION_TEMPLE_INC_COUNT lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql, "EXEC SP_ENTER_ILLUSION_TEMPLE '%s', '%s', '%d'",
			szAccountID, szGameID, lpMsg->ServerCode / 20 + 1
		);
		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
			pRankDB->Clear();
			__leave;
		}

		pRankDB->Clear();
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}




void CEventRankDB::EVDB_SendEnterCount_IllusionTemple(CIOStream* lpCXOL, LPPMSG_REQ_ILLUSIONTEMPLE_ENTERCOUNT lpMsg)
{
	// Get Any free Query from the memory
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	if (lpCXOL == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_SendEnterCount_IllusionTemple ERROR : lpCXOL == NULL");
		return;
	}

	if (lpCXOL->m_lpSOCKETCLSS == NULL)
	{
		LOG_ERROR.Output("CEventRankDB::EVDB_SendEnterCount_IllusionTemple ERROR : lpCXOL->m_lpSOCKETCLSS == NULL");
		return;
	}

	__try
	{
		SOCKET s = lpCXOL->m_Socket;

		PMSG_ANS_ILLUSIONTEMPLE_ENTERCOUNT pMsg;

		pMsg.h.c = 0xC1;
		pMsg.h.headcode = 0x0B;
		pMsg.h.size = sizeof(pMsg);
		memcpy(pMsg.AccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(pMsg.GameID, lpMsg->GameID, MAX_IDSTRING);
		pMsg.ServerCode = lpMsg->ServerCode;
		pMsg.iObjIndex = lpMsg->iObjIndex;

		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->AccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->GameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql, "EXEC SP_LEFT_ENTERCOUNT_ILLUSIONTEMPLE '%s', '%s', '%d'",
			szAccountID, szGameID, lpMsg->ServerCode
		);
		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
			pRankDB->Clear();
			__leave;
		}

		SQLRETURN nRET = pRankDB->Fetch();
		if (nRET != SQL_NO_DATA && nRET != SQL_ERROR)
			pMsg.iLeftCount = pRankDB->GetInt("LeftEnterCount");
		else
			pMsg.iLeftCount = -1;

		pRankDB->Clear();

		if (lpCXOL->m_lpSOCKETCLSS->Send(s, lpCXOL, &pMsg, sizeof pMsg) == FALSE)
		{
			LOG_ERROR.Output("CEventRankDB::EVDB_SendEnterCount_IllusionTemple ERROR : SendPacket_TCP FAILED");
		}
	}
	__finally
	{
		// Reinster into avaible queue once SQL queryr class is free
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}

void CEventRankDB::IllusionTempleScore(LPPMSG_ANS_ILLUSION_TEMPLE_SCORE lpMsg)
{
	// Get a free query from the pool
	CQuery* pRankDB = NULL;
	while (true)
	{
		EnterCriticalSection(&m_RankCritical);

		if (m_RankQuery.size())
		{
			pRankDB = m_RankQuery.front();
			m_RankQuery.pop();
		}

		LeaveCriticalSection(&m_RankCritical);

		if (pRankDB != NULL)
			break;

		Sleep(100);
	}

	__try
	{
		// Process the packet
		char szAccountID[MAX_IDSTRING + 1];
		char szGameID[MAX_IDSTRING + 1];

		memcpy(szAccountID, lpMsg->szAccountID, MAX_IDSTRING);
		memcpy(szGameID, lpMsg->szGameID, MAX_IDSTRING);
		szAccountID[MAX_IDSTRING] = '\0';
		szGameID[MAX_IDSTRING] = '\0';

		char sql[512];
		wsprintf(sql,
			"EXEC SP_REG_ILLUSION_TEMPLE_RANKPOINT '%s', '%s', %d, %d, %d, %d, %d, %d, %d",
			szAccountID,
			szGameID,
			lpMsg->wServerCode,
			lpMsg->nTempleNumber,
			lpMsg->nClass,
			lpMsg->nLevel,
			lpMsg->btWin,
			lpMsg->nPCBangGuid,
			lpMsg->nRankPoint);

		if (pRankDB->Exec(sql) == FALSE)
		{
			LOG_ERROR.Output(sql);
		}

		pRankDB->Clear();
	}
	__finally
	{
		// Return the query to the available pool
		EnterCriticalSection(&m_RankCritical);
		m_RankQuery.push(pRankDB);
		LeaveCriticalSection(&m_RankCritical);
	}
}

void CEventRankDB::ProtocolCore(BYTE btHeadCode, CIOStream* lpCXOL, LPBYTE lpMsg)
{
	switch (btHeadCode)
	{
	case 0x01:
		DevilSqureScore((LPPMSG_ANS_EVENTUSERSCORE)lpMsg);
		break;
	case 0x02:
		RankUpdate();
		break;
	case 0x07:
		BloodCastleScore((LPPMSG_ANS_BLOODCASTLESCORE)lpMsg);
		break;
	case 0x08:
		EVDB_CheckEnterCount_BC(lpCXOL, (LPPMSG_REQ_BLOODCASTLEENTERCHECK)lpMsg);
		break;
	case 0x09:
		EVDB_EnterCountOn_BC(lpCXOL, (LPPMSG_REQ_BLOODCASTLECOUNTON)lpMsg);
		break;
	case 0x0A:
		BloodCastleScore3rd((LPPMSG_ANS_BLOODCASTLESCORE_3RD)lpMsg);
		break;
	case 0x0B:
		EVDB_SendEnterCount_BC(lpCXOL, (LPPMSG_REQ_BLOODCASTLE_ENTERCOUNT)lpMsg);
		break;
	case 0x0C:
		BloodCastleScore4th((LPPMSG_ANS_BLOODCASTLESCORE_3RD)lpMsg);
		break;
	case 0x0D:
		BloodCastleScore5th((LPPMSG_ANS_BLOODCASTLESCORE_5TH)lpMsg);
		break;
	case 0x12:
		EVDB_CheckEnterCount_IllusionTemple(lpCXOL, (LPPMSG_REQ_ILLUSION_TMPLE_ENTERCOUNTCHECK)lpMsg);
		break;
	case 0x13:
		EVDB_EnterCountOn_IllusionTemple(lpCXOL, (LPPMSG_REQ_ILLUSION_TEMPLE_INC_COUNT)lpMsg);
		break;
	case 0x14:
		EVDB_SendEnterCount_IllusionTemple(lpCXOL, (LPPMSG_REQ_ILLUSIONTEMPLE_ENTERCOUNT)lpMsg);
		break;
#ifdef ADD_EVENT_MAP_ILLUSION_TEMPLE_20070328
	//case 0x26:
	//	IllusionTempleScore((LPPMSG_ANS_ILLUSION_TEMPLE_SCORE)lpMsg);//This is on the EventServer
	//	break;
#endif
	default:
		gLogProc.LogAddFunc("wrong protocol !! : %x", btHeadCode);
		LOG_ERROR.Output("wrong protocol !! : %x", btHeadCode);
		break;
	}
}