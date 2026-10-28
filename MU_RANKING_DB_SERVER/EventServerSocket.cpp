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

VOID CALLBACK WaitCallback(int nQueueIndex, int nWorkIndex, QNode* pNode);

CEventServerSocket::CEventServerSocket()
{
	WSADATA wsaData;
	int nRET = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRET == WSAEFAULT		||nRET == WSAEINPROGRESS		||nRET == WSAEPROCLIM ||
		nRET == WSASYSNOTREADY	||nRET == WSAVERNOTSUPPORTED)
	{
		WSACleanup();
		MessageBox(NULL, "failed to load ws2_32.dll", "error", 0);
		gLogProc.LogAddFunc("CEventServerSocket::CEventServerSocket - WSAStartup() error : %d", nRET);
		return;
	}

	if (LoadConfig(".\\svconfig.ini") == FALSE)
	{
		MessageBox(NULL, "server config file does not exist !!", "error", 0);
		ExitProcess(-1);
	}

	PER_SOCKET_CONTEXT * PerSocketCxt = new PER_SOCKET_CONTEXT;
	if (PerSocketCxt)
	{
		PerSocketCxt->m_nCount = 0;
		PerSocketCxt->m_nQueueNo = m_QueueNo;
		PerSocketCxt->m_lpCriticalSection = new CRITICAL_SECTION[m_QueueNo];
		PerSocketCxt->m_hEvent = new HANDLE[m_QueueNo];
		PerSocketCxt->m_lpQNode = new QNode[m_QueueNo];
		
		for (int i = 0; i < m_QueueNo; i++)
		{
			InitializeCriticalSection(&PerSocketCxt->m_lpCriticalSection[i]);
			PerSocketCxt->m_hEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
		}
	}

	m_lpPerSocketContext = PerSocketCxt;
	m_lpPerSocketContext->m_fnWaitCallBack = WaitCallback;

	for (UINT i = 0; i < m_lpPerSocketContext->m_nQueueNo; i++)
	{
		for (int n = 0; n < m_WorkerThreadNo; n++)
		{
			EVENT_THREAD_PARAM* pParam = new EVENT_THREAD_PARAM;

			pParam->lpPerSocketContext = m_lpPerSocketContext;
			pParam->nQueueIndex = i;
			pParam->nWorkIndex = n;

			unsigned int dwThreadID;
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall*)(void*))CEventServerSocket::WaithThread, (void*)pParam, 0, &dwThreadID);
			CloseHandle(hThread);
			Sleep(100);
		}
	}

	if (m_RankDB.Connect(0, NULL, NULL, NULL) == FALSE)
	{
		gLogProc.LogAddFunc("MAKE ODBC Connection Failed");
	}

	for(int i = 0 ; i < MAX_IOWORKING_THREAD ; i++)
		m_hIOWorkingThread[i] = INVALID_HANDLE_VALUE;

	m_hIOCP = INVALID_HANDLE_VALUE;
	InitializeCriticalSection(&m_SocketCritical);

	m_nServerCount = 0;

	CreateKeyPool();
}


//CEventServerSocket::~CEventServerSocket()
//{
//	closesocket(m_Socket);
//	WSACleanup();
//
//	for (DWORD i = 0; i < m_lpPerSocketContext->m_nQueueNo; i++)
//	{
//		DeleteCriticalSection(&m_lpPerSocketContext->m_lpCriticalSection[i]);
//		CloseHandle(&m_lpPerSocketContext->m_hEvent[i]);
//		
//	}
//	// #MEMORY LEAK_ Not delething thunks of VAR m_lpPerSocketContext
//
//	delete m_lpPerSocketContext;
//
//	for (int i = 0; i < MAX_IOWORKING_THREAD; i++)
//		CloseHandle(m_hIOWorkingThread[i]);
//
//	if ( m_hIOCP != INVALID_HANDLE_VALUE)
//		CloseHandle(m_hIOCP);
//
//	DeleteCriticalSection(&m_SocketCritical);
//}

CEventServerSocket::~CEventServerSocket()
{
    closesocket(m_Socket);
    WSACleanup();

    if (m_lpPerSocketContext)
    {
        for (DWORD i = 0; i < m_lpPerSocketContext->m_nQueueNo; i++)
        {
            DeleteCriticalSection(&m_lpPerSocketContext->m_lpCriticalSection[i]);

            if (m_lpPerSocketContext->m_hEvent[i])
            {
                CloseHandle(m_lpPerSocketContext->m_hEvent[i]);
            }
        }

        delete[] m_lpPerSocketContext->m_lpCriticalSection;
        delete[] m_lpPerSocketContext->m_hEvent;
        delete[] m_lpPerSocketContext->m_lpQNode;

        delete m_lpPerSocketContext;
        m_lpPerSocketContext = NULL;
    }

    for (int i = 0; i < MAX_IOWORKING_THREAD; i++)
    {
        if (m_hIOWorkingThread[i] &&
            m_hIOWorkingThread[i] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hIOWorkingThread[i]);
        }
    }

    if (m_hIOThread &&
        m_hIOThread != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hIOThread);
    }

    if (m_hIOCP &&
        m_hIOCP != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hIOCP);
    }

    DeleteCriticalSection(&m_SocketCritical);
}


BOOL CEventServerSocket::LoadConfig(char* lpszConfigFile)
{
	if (_access(lpszConfigFile, 0) != 0)
		return FALSE;

	char szData[64];

	if (GetPrivateProfileString("pim_setting", "queue_no", "", szData, _countof(szData), lpszConfigFile))
		m_QueueNo = atoi(szData);
	else
		m_QueueNo = DEF_RANK_QUEUE_NO;

	if (GetPrivateProfileString("pim_setting", "workerthread_no", "", szData, _countof(szData), lpszConfigFile))
		m_WorkerThreadNo = atoi(szData);
	else
		m_WorkerThreadNo = DEF_RANK_WORKERTHREAD_NO;

	if (GetPrivateProfileString("odbc_setting", "dbname", "", szData, _countof(szData), lpszConfigFile) == 0)
		return FALSE;
	else
		strcpy(m_RankDB.m_szDBName, szData);

	if (GetPrivateProfileString("odbc_setting", "odbc_dsn", "", szData, _countof(szData), lpszConfigFile) == 0)
		return FALSE;
	else
		strcpy(m_RankDB.m_szDSNName, szData);

	if (GetPrivateProfileString("odbc_setting", "odbc_uid", "", szData, _countof(szData), lpszConfigFile) == 0)
		return FALSE;
	else
		strcpy(m_RankDB.m_szUserID, szData);

	if (GetPrivateProfileString("odbc_setting", "odbc_pass", "", szData, _countof(szData), lpszConfigFile) == 0)
		return FALSE;
	else
		strcpy(m_RankDB.m_szUserPW, szData);

	if (GetPrivateProfileString("odbc_setting", "odbc_con_count", "", szData, _countof(szData), lpszConfigFile) == 0)
		m_RankDB.m_ConCount = DEF_RANK_CONN_COUNT;
	else
		m_RankDB.m_ConCount = atoi(szData);

	return TRUE;
}


VOID CALLBACK WaitCallback(int nQueueIndex, int nWorkIndex, QNode* pNode)
{
	// Stub - the original does nothing meaningful here.
	// The actual packet processing happens in ServerWorkerThread via IOCP.
}


BOOL CEventServerSocket::CreateIOCompletionPort()
{
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (m_hIOCP == INVALID_HANDLE_VALUE)
		return FALSE;

	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (m_Socket == INVALID_SOCKET)
		return FALSE;

	INT stream_size = MAX_PACKETSTREAM_SIZE;
	if (setsockopt(m_Socket, 0xFFFF, SO_RCVBUF, (char*)&stream_size, sizeof(stream_size)) == -1)
		return FALSE;

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	//for (DWORD dwCPU = 0; dwCPU < sysInfo.dwNumberOfProcessors; dwCPU++)
	//for (DWORD dwCPU = 0; dwCPU < sysInfo.dwNumberOfProcessors * 2; dwCPU++)
	DWORD dwMaxThreads = min(2 * sysInfo.dwNumberOfProcessors, MAX_IOWORKING_THREAD);// fix overflow on over 64 threads CPU
	for (DWORD dwCPU = 0; dwCPU < dwMaxThreads; dwCPU++)// 
	{
		if (CreateServerWorkerThread(dwCPU) == FALSE)
			return FALSE;
	}
	if (dwMaxThreads > MAX_IOWORKING_THREAD)
	{
		gLogProc.LogAddFunc(" **** Worker Thread Max Count Fix 64ea ****");
	}

	m_hIOThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall*)(void*))CEventServerSocket::IocpServerWorker, this, 0, (unsigned int*)&m_dwThreadID);
	if (m_hIOThread == INVALID_HANDLE_VALUE)
		return FALSE;

	gLogProc.LogAddFuncColor(4, "<<< Devil Squre - Ranking Server Start. (TCP) >>>");
	LOG_ERROR.Output("<<< Devil Squre - Ranking Server Start. (TCP) >>>");

	return TRUE;
}

BOOL CEventServerSocket::CreateServerWorkerThread(int nIndex)
{
	m_hIOWorkingThread[nIndex] = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall*)(void*))CEventServerSocket::ServerWorkerThread, this, 0, (unsigned int*)&m_dwIOWorkingThreadID[nIndex]);
	return m_hIOWorkingThread[nIndex] != INVALID_HANDLE_VALUE;
}

void CALLBACK CEventServerSocket::IocpServerWorker(CEventServerSocket* p)
{
	SOCKADDR_IN SockAddr;

	SockAddr.sin_family = AF_INET;
	SockAddr.sin_port = ntohs(DEFAULT_RANKSERVER_PORT);
	SockAddr.sin_addr.S_un.S_addr = ntohl(0);

	if (bind(p->m_Socket, (sockaddr*)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();
		gLogProc.LogAddFunc("bind() error : %d", dwLastError);
		LOG_ERROR.Output("bind() error : %d", dwLastError);
	}

	if (listen(p->m_Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();
		gLogProc.LogAddFunc("listen() error : %d", dwLastError);
		LOG_ERROR.Output("listen() error : %d", dwLastError);
	}

	int SockAddrLen = sizeof(SockAddr);
	while (true)
	{
		SOCKET s = WSAAccept(p->m_Socket, (sockaddr*)&SockAddr, &SockAddrLen, NULL, NULL);
		if (s == SOCKET_ERROR)
		{
			DWORD dwLastError = WSAGetLastError();
			gLogProc.LogAddFunc("WSAAccept() error : %d", dwLastError);
			LOG_ERROR.Output("WSAAccept() error : %d", dwLastError);
			continue;
		}
			
		int nIndex = -1;
		while (true)
		{
			nIndex = p->AddSocket(s);
			if (nIndex >= 0)
				break;
			Sleep(10);
		}

		p->m_IOStream1[nIndex].m_Socket = s;
		p->m_IOStream1[nIndex].IOOperation = 1;
		p->m_IOStream2[nIndex].m_Socket = s;
		p->m_IOStream2[nIndex].IOOperation = 2;

		p->m_hIOCP = CreateIoCompletionPort((HANDLE)s, p->m_hIOCP, nIndex, 0);
		if (p->m_hIOCP == NULL)
		{
			DWORD dwLastError = WSAGetLastError();
			gLogProc.LogAddFunc("CreateIoCompletionPort() error : %d", dwLastError);
			LOG_ERROR.Output("CreateIoCompletionPort() error : %d", dwLastError);
				
			while (p->m_hIOCP == NULL)
			{
				Sleep(100);
				p->m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

				if (p->m_hIOCP == NULL)
				{
					DWORD dwLastError = WSAGetLastError();
					gLogProc.LogAddFunc("CreateIoCompletionPort() error : %d", dwLastError);
					LOG_ERROR.Output("CreateIoCompletionPort() error : %d", dwLastError);
				}
			}
			p->DelSocket(nIndex);
		}

		p->ReadyRecvWork_TCP(s, &p->m_IOStream1[nIndex], nIndex);
	}
}


void CALLBACK CEventServerSocket::ServerWorkerThread(CEventServerSocket* p)
{
	while (true)
	{
		LPOVERLAPPED lpOverlapped;
		DWORD dwIoSize;
		int nIndex;
		
		if (GetQueuedCompletionStatus(p->m_hIOCP, &dwIoSize, (PULONG_PTR)&nIndex, &lpOverlapped, INFINITE) == FALSE)
		{
			if (lpOverlapped != NULL)
			{
				DWORD dwErrorCode = GetLastError();

				if (ERROR_NETNAME_DELETED != dwErrorCode /*&&
					ERROR_CONNECTION_ABORTED != errorcode &&
					ERROR_OPERATION_ABORTED != errorcode &&
					ERROR_SEM_TIMEOUT != errorcode*/)
				{
					gLogProc.LogAddFunc("GetQueuedCompletionStatus() error : %d", dwErrorCode);
					LOG_ERROR.Output("GetQueuedCompletionStatus() error : %d", dwErrorCode);
					continue;
				}
			}
		}

		if (dwIoSize == 0)
		{
			p->DelSocket(nIndex);
			continue;
		}

		CIOStream* lpIOContext = (CIOStream*)lpOverlapped;

		// Avoids crash when closing
		if (lpIOContext == NULL)
		{
			continue;
		}
		
		if (lpIOContext->IOOperation == 2)
		{
			p->UpdateCompletionStatus(p->m_Socket, &p->m_IOStream2[nIndex]);
		}
		else if (lpIOContext->IOOperation == 1)
		{
			DWORD dwRecvPacket = 0;	// Count Number
			BOOL bSuccess = TRUE;

			lpIOContext->nSentBytes += dwIoSize;

			while (true)
			{
				int nPacketSize = 0;
				switch (lpIOContext->m_byBuffer[0])
				{
				case 0xC1:
					if (dwIoSize <= 2) {	// Original was <
						bSuccess = FALSE;
						nPacketSize = 0;
					}
					else
						nPacketSize = lpIOContext->m_byBuffer[1];
					break;
				case 0xC2:
					if (dwIoSize <= 3) {	// Original was <
						bSuccess = FALSE;
						nPacketSize = 0;
					}
					else
						nPacketSize = MAKEWORD(lpIOContext->m_byBuffer[2], lpIOContext->m_byBuffer[1]);
					break;
				default:
					gLogProc.LogAddFunc("packet header is wrong");
					nPacketSize = 0;
				}

				if (bSuccess == FALSE)
				{
					lpIOContext->nSentBytes += dwIoSize;
					break;
				}
				else if (nPacketSize <= 0)
				{
					lpIOContext->nSentBytes = 0;
					break;
				}
				else
				{
					if ((DWORD)nPacketSize < lpIOContext->nSentBytes)
					{
						BYTE pDataBuffer[MAX_STREAM_SIZE];
						CIOStream* lpSendIOCtx = lpIOContext->m_lpSOCKETCLSS->GetSendIOContext(lpIOContext->m_nQueueIndex);

						memcpy(pDataBuffer, lpIOContext->m_byBuffer, nPacketSize);
						memmove(lpIOContext->m_byBuffer, &lpIOContext->m_byBuffer[nPacketSize], lpIOContext->nSentBytes - nPacketSize);
						lpIOContext->nSentBytes -= nPacketSize;
						dwIoSize -= nPacketSize;

						switch (pDataBuffer[0])
						{
						case 0xC1:
							p->m_RankDB.ProtocolCore(pDataBuffer[2], lpSendIOCtx, pDataBuffer);
							break;
						case 0xC2:
							p->m_RankDB.ProtocolCore(pDataBuffer[3], lpSendIOCtx, pDataBuffer);
							break;
						default:
							gLogProc.LogAddFunc("wrong protocol !! : %x", pDataBuffer[0]);
							LOG_ERROR.Output("wrong protocol !! : %x", pDataBuffer[0]);
						}

						if (lpIOContext->nSentBytes <= 0)
						{
							lpIOContext->nSentBytes = 0;
							break;
						}

						dwRecvPacket++;
						if (dwRecvPacket > 100000)
						{
							MessageBox(NULL, "error", "unlimited loop !!", 0);
							break;
						}
					}
					else
					{
						break;
					}
				}
			}	// while (true )
			p->ReadyRecvWork_TCP(lpIOContext->m_Socket, &p->m_IOStream1[nIndex], nIndex);
		}	// IOType
	}
}


BOOL CEventServerSocket::ReadyRecvWork_TCP(SOCKET s, CIOStream* lpIOContext, int nIndex)
{
	if (nIndex < 0 || nIndex >= MAX_IOSTREAM_COUNT)
	{
		gLogProc.LogAddFunc("ReadyRecvWork_TCP() Index Out of Bound : %d", nIndex);
		LOG_ERROR.Output("ReadyRecvWork_TCP() Index Out of Bound : %d", nIndex);
		return FALSE;
	}

	WSABUF WSABuf;
	WSABuf.len = lpIOContext->m_BufSize - lpIOContext->nSentBytes;
	WSABuf.buf = (CHAR*)(&m_IOStream1->m_byBuffer[lpIOContext->nSentBytes]);

	DWORD dwBytesRecv;
	DWORD Flags = 0;

	if (WSARecv(s, &WSABuf, 1, &dwBytesRecv, &Flags, &lpIOContext->Overlapped, NULL) == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();
		if (dwLastError != ERROR_IO_PENDING)
		{
			shutdown(s, SD_BOTH);
			closesocket(s);
			DelSocket(nIndex);
			gLogProc.LogAddFunc("WSARecv() error : %d", dwLastError);
			LOG_ERROR.Output("WSARecv() error : %d", dwLastError);

			return FALSE;
		}
	}
	return TRUE;
}


BOOL CEventServerSocket::UpdateCompletionStatus(SOCKET s, CIOStream* lpCXOL)
{
	EnterCriticalSection(&lpCXOL->m_CriticalSection);
	lpCXOL->m_nWaitIO = 0;
	LeaveCriticalSection(&lpCXOL->m_CriticalSection);
	return TRUE;
}


BOOL CEventServerSocket::Send(SOCKET s, CIOStream* lpCXOL, LPVOID lpBuffer, UINT nSize)
{
	if (s == INVALID_SOCKET)
		return FALSE;
	if (lpCXOL == NULL)
		return FALSE;

	EnterCriticalSection(&lpCXOL->m_CriticalSection);

	memcpy(lpCXOL->m_byBuffer, lpBuffer, nSize);

	WSABUF WSABuf;
	WSABuf.buf = (CHAR*)lpCXOL->m_byBuffer;
	WSABuf.len = nSize;

	DWORD dwBytesSent;
	if (WSASend(s, &WSABuf, 1, &dwBytesSent, 0, &lpCXOL->Overlapped, NULL) == SOCKET_ERROR)
	{
		DWORD dwLastError = WSAGetLastError();
		if (dwLastError != ERROR_IO_PENDING)
		{
			shutdown(s, SD_BOTH);
			closesocket(s);

			gLogProc.LogAddFunc("WSASend() error : %d", dwLastError);
			LOG_ERROR.Output("WSASend() error : %d", dwLastError);

			lpCXOL->m_nWaitIO = FALSE;
			LeaveCriticalSection(&lpCXOL->m_CriticalSection);
			return FALSE;
		}
	}

	lpCXOL->m_nWaitIO = TRUE;
	LeaveCriticalSection(&lpCXOL->m_CriticalSection);
	return TRUE;
}

////////////////////////
// Scalar destructor QNode First is Count, then array of qNode and QNodes
void CALLBACK CEventServerSocket::WaithThread(EVENT_THREAD_PARAM* lpParam)
{
	PER_SOCKET_CONTEXT* lpSOCKETCTX = lpParam->lpPerSocketContext;
	int nWorkIndex = lpParam->nWorkIndex;
	int nQueueIndex = lpParam->nQueueIndex;

	delete lpParam;

	while (true)
	{
		WaitForSingleObject(lpSOCKETCTX->m_hEvent[nQueueIndex], INFINITE);

		EnterCriticalSection(&lpSOCKETCTX->m_lpCriticalSection[nQueueIndex]);

		QNode* pNode1, *pNode2;
		QNode* nOut = NULL;
		
		pNode1 = &lpSOCKETCTX->m_lpQNode[nQueueIndex];

		if (pNode1->Prev == 0)
		{
			nOut = NULL;
		}
		else
		{
			pNode2 = pNode1->Prev->Next;
			pNode1->Prev = pNode2;
			if (pNode2 == NULL)
			{
				pNode1->Next = 0;
			}
			nOut = pNode1->Prev->Next;
			if (pNode1->Prev)
				delete pNode1->Prev;
			pNode1->Count--;
		}

		LeaveCriticalSection(&lpSOCKETCTX->m_lpCriticalSection[nQueueIndex]);

		if (nOut)
		{
			if (lpSOCKETCTX->m_lpQNode[nQueueIndex].Count)
				SetEvent(lpSOCKETCTX->m_hEvent[nQueueIndex]);
			lpSOCKETCTX->m_fnWaitCallBack(nQueueIndex, nWorkIndex, nOut);
		}
	}
}


void CEventServerSocket::CreateKeyPool()
{
	for (int i = 0; i < MAX_IOSTREAM_COUNT; i++)
	{
		EnterCriticalSection(&m_SocketCritical);
		m_KeyPool.push(i);
		LeaveCriticalSection(&m_SocketCritical);
	}
}


int CEventServerSocket::AddSocket(SOCKET s)
{
	int nKey = -1;
	__try
	{
		EnterCriticalSection(&m_SocketCritical);

		// Check if any key avaible
		if (m_KeyPool.size() == 0)
			__leave;

		nKey = m_KeyPool.front();
		m_KeyPool.pop();

		m_IOStream1[nKey].m_lpSOCKETCLSS = this;
		m_IOStream1[nKey].m_nQueueIndex = nKey;
		m_IOStream2[nKey].m_lpSOCKETCLSS = this;
		m_IOStream2[nKey].m_nQueueIndex = nKey;

		gLogProc.LogAddFuncColor(3, "Add Socket : %d, now empty buffer count : %d",
			s, m_KeyPool.size());
		m_nServerCount++;
	}
	__finally
	{
		LeaveCriticalSection(&m_SocketCritical);
	}
	return nKey;
}

BOOL CEventServerSocket::DelSocket(int nIndex)
{
	if (nIndex < 0 || nIndex >= MAX_IOSTREAM_COUNT)
		return FALSE;
	EnterCriticalSection(&m_SocketCritical);

	shutdown(m_IOStream1[nIndex].m_Socket, SD_BOTH | 0x01);
	closesocket(m_IOStream1[nIndex].m_Socket);

	// Reinster used key
	m_KeyPool.push(nIndex);

	gLogProc.LogAddFunc("Delete Socket:%d (%d), now empty buffer count : %d",
		m_IOStream1[nIndex].m_Socket, nIndex, m_KeyPool.size());

	m_IOStream1[nIndex].m_Socket = INVALID_SOCKET;
	m_IOStream2[nIndex].m_Socket = INVALID_SOCKET;

	m_nServerCount--;

	LeaveCriticalSection(&m_SocketCritical);

	return TRUE;
}


CIOStream* CEventServerSocket::GetRecvIOContext(int nIndex)
{
	EnterCriticalSection(&m_SocketCritical);
	CIOStream* pIOStream = &m_IOStream1[nIndex];
	LeaveCriticalSection(&m_SocketCritical);

	return pIOStream;
}


CIOStream* CEventServerSocket::GetSendIOContext(int nIndex)
{
	EnterCriticalSection(&m_SocketCritical);
	CIOStream* pIOStream = &m_IOStream2[nIndex];
	LeaveCriticalSection(&m_SocketCritical);

	return pIOStream;
}