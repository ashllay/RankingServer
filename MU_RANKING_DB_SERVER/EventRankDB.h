//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "CQuery.h"
#include "cb_protocol.h"
#include <queue>


#define MAX_NODE_BUFFER	0x1000


class CIOStream;

class CEventRankDB
{
public:
	CEventRankDB();
	~CEventRankDB();

	BOOL Connect(DWORD nConnCount, char* szConnStr, char* szUserID, char* szUserPW);

	void DevilSqureScore(LPPMSG_ANS_EVENTUSERSCORE lpMsg);
	void RankUpdate();
	void BloodCastleScore(LPPMSG_ANS_BLOODCASTLESCORE lpMsg);
	void BloodCastleScore3rd(LPPMSG_ANS_BLOODCASTLESCORE_3RD lpMsg);
	void BloodCastleScore4th(LPPMSG_ANS_BLOODCASTLESCORE_3RD lpMsg);
	void BloodCastleScore5th(LPPMSG_ANS_BLOODCASTLESCORE_5TH lpMsg);

	void EVDB_SendEnterCount_BC(CIOStream * lpCXOL, LPPMSG_REQ_BLOODCASTLE_ENTERCOUNT lpMsg);
	void EVDB_CheckEnterCount_BC(CIOStream* lpCXOL, LPPMSG_REQ_BLOODCASTLEENTERCHECK lpMsg);
	void EVDB_EnterCountOn_BC(CIOStream* lpCXOL, LPPMSG_REQ_BLOODCASTLECOUNTON lpMsg);

	void EVDB_CheckEnterCount_IllusionTemple(CIOStream* lpCXOL, LPPMSG_REQ_ILLUSION_TMPLE_ENTERCOUNTCHECK lpMsg);
	void EVDB_EnterCountOn_IllusionTemple(CIOStream* lpCXOL, LPPMSG_REQ_ILLUSION_TEMPLE_INC_COUNT lpMsg);
	void EVDB_SendEnterCount_IllusionTemple(CIOStream* lpCXOL, LPPMSG_REQ_ILLUSIONTEMPLE_ENTERCOUNT lpMsg);
#ifdef ADD_EVENT_MAP_ILLUSION_TEMPLE_20070328
	void IllusionTempleScore(LPPMSG_ANS_ILLUSION_TEMPLE_SCORE lpMsg);
#endif
public:
	void ProtocolCore(BYTE btHeadCode, CIOStream* lpCXOL, LPBYTE lpMsg);

public:
	char	m_szDBName[32];	// 0
	char	m_szDSNName[32];	// 20
	char	m_szUserID[32];	// 40
	char	m_szUserPW[32];	// 60
	DWORD	m_ConCount;	// 80
	int	unk84;
	//s_unk2	unk88;	// 88
	/*bool unk88;
	ListNode m_PrevListNode;	// 8C
	ListNode m_NextListNode;	// 9C
	s_unk1	* unkac;	// AC
	int unkb0;*/
	std::queue<CQuery*>	m_RankQuery;	// 88 
	//int		m_LNodeCount;	// B4
	CRITICAL_SECTION m_RankCritical;	// b8
};

