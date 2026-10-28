//////////////////////////////////////////////////////////////////////////////////////////////
// MU_RANKING_DB_SERVER 
//  VERSION "20081007"
//  COUNTRY "INTERNATIONAL"
// Decompiled by Deathway 
// Date: 01/27/2021
// Status: Completed - Not Tested
//////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <process.h>
#include "EventRankDB.h"

#define DEF_RANK_QUEUE_NO			2
#define DEF_RANK_WORKERTHREAD_NO	2
#define DEF_RANK_CONN_COUNT			10
#define MAX_IOSTREAM_COUNT			1000
#define MAX_IOWORKING_THREAD		64
#define MAX_STREAM_SIZE				10240
#define MAX_PACKETSTREAM_SIZE		0x5000

struct QNode
{
	QNode()
	{
		Prev = NULL;
		Next = NULL;
		Count = 0;
	}

	~QNode()
	{
		QNode* pNode = this->Prev;
		while (pNode)
		{
			this->Prev = this->Next;
			if (pNode)
				delete pNode;
			pNode = this->Prev;
		}
	}

	QNode * Prev;	// 0
	QNode * Next;	// 4
	int Count;	// 8
};

typedef VOID(CALLBACK* fnPSC_CALLBACK)(int, int, QNode*);

struct PER_SOCKET_CONTEXT
{
	LPCRITICAL_SECTION	m_lpCriticalSection;	// 0
	DWORD				m_nQueueNo;	// 4
	int					m_nCount;	// 8
	QNode		*		m_lpQNode;	// C
	HANDLE		*		m_hEvent;	// 10
	fnPSC_CALLBACK		m_fnWaitCallBack;	// 14
};
// 18

struct EVENT_THREAD_PARAM
{
	PER_SOCKET_CONTEXT* lpPerSocketContext;	// 0
	int nWorkIndex;	// 4
	int nQueueIndex;	// 8
};

class CEventServerSocket;

class CIOStream
{
public:
	CIOStream()
	{
		this->m_BufSize = MAX_STREAM_SIZE;
		this->nSentBytes = 0;
		this->unk2828 = 0;
		this->Overlapped.hEvent = NULL;
		this->IOOperation = 0;

		InitializeCriticalSection(&m_CriticalSection);

		this->m_nWaitIO = 0;
		this->m_lpSOCKETCLSS = NULL;
	}
	virtual ~CIOStream()
	{
		DeleteCriticalSection(&m_CriticalSection);
		delete[] this->unk2828;
	}
public:
	WSAOVERLAPPED		Overlapped;
	int 				IOOperation;	// 18
	SOCKET				m_Socket;	// 1C
	BYTE				m_byBuffer[MAX_STREAM_SIZE];	// 20
	int					m_BufSize;	// 2820
	DWORD				nSentBytes;	// 2824
	LPVOID	unk2828;
	int					m_nWaitIO;	/*282c*/
	int					m_nQueueIndex;	// unk2830
	CEventServerSocket* m_lpSOCKETCLSS;	// 2834
	CRITICAL_SECTION	m_CriticalSection;	// 2838
};

class CEventServerSocket
{
public:
	CEventServerSocket();
	~CEventServerSocket();

	BOOL LoadConfig(char* lpszConfigFile);
	BOOL CreateIOCompletionPort();
	BOOL CreateServerWorkerThread(int nIndex);

	int AddSocket(SOCKET s);
	BOOL DelSocket(int nIndex);
	
	BOOL Send(SOCKET s, CIOStream* lpCXOL, LPVOID lpBuffer, UINT nSize);
	int GetServerCount() {		return m_nServerCount			;	}
public:
	static void CALLBACK WaithThread(EVENT_THREAD_PARAM* lpParam);
	static void CALLBACK IocpServerWorker(CEventServerSocket* p);
	static void CALLBACK ServerWorkerThread(CEventServerSocket* p);
private:
	// VOID CALLBACK WaitCallback(int nQueueIndex, int nWorkIndex, QNode* pNode);
private:
	void CreateKeyPool();
	BOOL UpdateCompletionStatus(SOCKET s, CIOStream* lpCXOL);
	BOOL ReadyRecvWork_TCP(SOCKET s, CIOStream* lpIOContext, int nIndex);
	CIOStream* GetSendIOContext(int nIndex);
	CIOStream* GetRecvIOContext(int nIndex);
private:
	SOCKET				m_Socket;	// 0
	char				unk4[260];	// 4
	PER_SOCKET_CONTEXT* m_lpPerSocketContext;	// 108
	int					m_QueueNo;	// 10C
	int					m_WorkerThreadNo;	// 110
	CEventRankDB		m_RankDB;	// 114
	HANDLE				m_hIOCP;	// 1E4
	HANDLE				m_hIOThread;	// 1E8
	HANDLE				m_hIOWorkingThread[MAX_IOWORKING_THREAD];	// 1EC
	DWORD				m_dwThreadID;	// 2EC
	DWORD				m_dwIOWorkingThreadID[MAX_IOWORKING_THREAD];	// 2F0
	CIOStream			m_IOStream1[MAX_IOSTREAM_COUNT];	// 3F0
	CIOStream			m_IOStream2[MAX_IOSTREAM_COUNT];	// 9d7c70
	std::queue<int>		m_KeyPool;	// 13aF4F0
	CRITICAL_SECTION	m_SocketCritical;	// 13af520
	int					m_nServerCount;	// unk13AF538
};

extern CEventServerSocket g_IOCPSocket;