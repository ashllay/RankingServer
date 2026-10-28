#include "stdafx.h"

#include "CQuery.h"
#include "assert.h"

CQuery::CQuery()
{
	AffectCount = -1;
	ret = SQL_SUCCESS;

	hStmt = NULL;
	hDbc = NULL;
	hEnv = NULL;

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
	SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
}

CQuery::~CQuery()
{
	if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	if (hDbc) SQLDisconnect(hDbc);
	if (hDbc) SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
	if (hEnv) SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}

// ¿¬°á ÇÚµéÀ» ÇÒ´çÇÏ°í ¿¬°áÇÑ ÈÄ ¸í·ÉÇÚµé±îÁö °°ÀÌ ÇÒ´çÇÑ´Ù.
// Type=1:ConStrÀº MDB ÆÄÀÏÀÇ °æ·Î¸¦ °¡Áø´Ù. °æ·Î »ı·«½Ã ÇöÀç µğ·ºÅä¸®¿¡¼­ MDB¸¦ Ã£´Â´Ù.
// Type=2:ConStrÀº SQL ¼­¹öÀÇ ¿¬°á Á¤º¸¸¦ °¡Áö´Â DSN ÆÄÀÏÀÇ °æ·Î¸¦ °¡Áø´Ù. 
//        °æ·Î´Â ¹İµå½Ã ¿ÏÀü °æ·Î·Î ÁöÁ¤ÇØ¾ß ÇÑ´Ù.
// Type=3:SQLConnect ÇÔ¼ö·Î DSN¿¡ Á÷Á¢ ¿¬°áÇÑ´Ù.
// ¿¬°á ¶Ç´Â ¸í·É ÇÚµé ÇÒ´ç¿¡ ½ÇÆĞÇÏ¸é FALSE¸¦ ¸®ÅÏÇÑ´Ù.

BOOL CQuery::Connect(int Type, char* ConStr, char* UID, char* PWD)
{
	SQLCHAR InCon[255];
	SQLCHAR OutCon[255];
	SQLSMALLINT cbOutCon;

	int ii = 1;
	SQLRETURN Ret;
	SQLINTEGER NativeError;
	SQLCHAR SqlState[6], Msg[255];
	SQLSMALLINT MsgLen;
	char str[256];

	m_Type = Type;
	strcpy(m_szConnect, ConStr);
	strcpy(m_Id, UID);
	strcpy(m_Pass, PWD);

	// ¿¬°á Å¸ÀÔ¿¡ µû¶ó MDB ¶Ç´Â SQL ¼­¹ö, ¶Ç´Â DSN¿¡ ¿¬°áÇÑ´Ù.
	SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
	switch (Type) {
	case 1:
		wsprintf((char*)InCon, "DRIVER={Microsoft Access Driver (*.mdb)};DBQ=%s;", ConStr);
		ret = SQLDriverConnect(hDbc, NULL, (SQLCHAR*)InCon, sizeof(InCon), OutCon,
			sizeof(OutCon), &cbOutCon, SQL_DRIVER_NOPROMPT);
		break;
	case 2:
		wsprintf((char*)InCon, "FileDsn=%s", ConStr);
		ret = SQLDriverConnect(hDbc, NULL, (SQLCHAR*)InCon, sizeof(InCon), OutCon,
			sizeof(OutCon), &cbOutCon, SQL_DRIVER_NOPROMPT);
		break;
	case 3:
		ret = SQLConnect(hDbc, (SQLCHAR*)ConStr, SQL_NTS, (SQLCHAR*)UID, SQL_NTS,
			(SQLCHAR*)PWD, SQL_NTS);
		break;
	}

	// Á¢¼Ó ¿¡·¯½Ã Áø´Ü Á¤º¸¸¦ º¸¿©ÁØ´Ù.
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		while (Ret = SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, ii, SqlState, &NativeError,
			Msg, sizeof(Msg), &MsgLen) != SQL_NO_DATA) {
			wsprintf(str, "SQLSTATE:%s, Áø?ÜÁ??:%s\n", (LPCTSTR)SqlState, (LPCTSTR)Msg);
			gLogProc.LogAddFunc(str);
			LOG_ERROR.Output(str);
			ii++;
		}
		return FALSE;
	}

	ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO)) {
		hStmt = 0;
		return FALSE;
	}
	return TRUE;
}

BOOL CQuery::ReConnect()
{
	return Connect(m_Type, m_szConnect, m_Id, m_Pass);
}


void CQuery::Close()
{
	if (hStmt) SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	if (hDbc) SQLDisconnect(hDbc);
	if (hDbc) SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
}

bool CQuery::SQLAllocStmt(SQLHDBC sqlHDBC)
{
	if (sqlHDBC == NULL)
		return false;

	SQLFreeStmt();
	SQLRETURN _rc = ::SQLAllocStmt(sqlHDBC, &hStmt);
	return true;
}

void CQuery::SQLFreeStmt()
{
	SQLRETURN _rc;
	if (hStmt != NULL)
		_rc = ::SQLFreeStmt(hStmt, SQL_HANDLE_STMT);
	hStmt = NULL;
}

BOOL CQuery::Exec(LPCTSTR szSQL)
{
	int c;
	//LogAddTD("%s", szSQL);
	//printf("%s\n", szSQL);

	// SQL¹®À» ½ÇÇàÇÑ´Ù. SQL_NO_DATA¸¦ ¸®ÅÏÇÑ °æ¿ìµµ ÀÏ´Ü ¼º°øÀ¸·Î Ãë±ŞÇÑ´Ù. 
	// ÀÌ °æ¿ì Fetch¿¡¼­ EOF¸¦ ¸®ÅÏÇÏµµ·Ï µÇ¾î ÀÖ±â ¶§¹®¿¡ Áø´Ü Á¤º¸¸¦ Ãâ·ÂÇÒ ÇÊ¿ä´Â ¾ø´Ù.

	//SQLAllocStmt();
	ret = SQLExecDirect(hStmt, (SQLCHAR*)szSQL, SQL_NTS);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO) && (ret != SQL_NO_DATA)) {
		PrintDiag();
		return FALSE;
	}

	// Update, Delete, Insert ¸í·É½Ã ¿µÇâ¹ŞÀº ·¹ÄÚµå °³¼ö¸¦ ±¸ÇØ ³õ´Â´Ù.
	SQLRowCount(hStmt, &AffectCount);
	SQLNumResultCols(hStmt, &nCol);
	if (nCol > MAXCOL) {
		//::MessageBox(NULL,"ÃÖ´ë ÄÃ·³ ¼ö¸¦ ÃÊ°úÇß½À´Ï´Ù","CQuery ¿¡·¯",MB_OK);
		//LogAdd("CQuery error :ÃÖ´ë ÄÃ·³ ¼ö¸¦ ÃÊ°úÇß½À´Ï´Ù");
		return FALSE;
	}

	// nColÀÌ 0ÀÎ °æ¿ì´Â Select¹® ÀÌ¿ÜÀÇ ´Ù¸¥ ¸í·ÉÀ» ½ÇÇàÇÑ °æ¿ìÀÌ¹Ç·Î 
	// ¹ÙÀÎµùÀ» ÇÒ ÇÊ¿ä°¡ ¾ø´Ù.
	if (nCol == 0) {
		Clear();
		return TRUE;
	}

	// ¸ğµç ÄÃ·³À» ¹®ÀÚ¿­·Î ¹ÙÀÎµùÇØ ³õ´Â´Ù. Col¹è¿­Àº zero base, 
	// ÄÃ·³ ¹øÈ£´Â one baseÀÓ¿¡ À¯ÀÇÇÒ °Í
	for (c = 0; c < nCol; c++) {
		SQLBindCol(hStmt, c + 1, SQL_C_CHAR, Col[c], 5000, &lCol[c]);
		SQLDescribeCol(hStmt, c + 1, ColName[c], 30, NULL, NULL, NULL, NULL, NULL);
	}
	return TRUE;
}


BOOL CQuery::Exec(SQLHDBC sqlHDBC, LPCTSTR szSQL)
{
	int c;
	//LogAddTD("%s", szSQL);
	//printf("%s\n", szSQL);

	// SQL??? ?ÇÇàÇ?? SQL_NO_DATA?? ??ÅÏÇÑ ??ìµ?ÀÏ????øÀ???Ãë?ŞÇ?? 
	// ÀÌ ???Fetch???? EOF?? ??ÅÏÇÏµµ??µÇ??ÀÖ???????? Áø?????? Ãâ?ÂÇ?ÇÊ??????

	SQLAllocStmt(sqlHDBC);
	ret = SQLExecDirect(hStmt, (SQLCHAR*)szSQL, SQL_NTS);
	if ((ret != SQL_SUCCESS) && (ret != SQL_SUCCESS_WITH_INFO) && (ret != SQL_NO_DATA)) {
		PrintDiag();
		return FALSE;
	}

	// Update, Delete, Insert ?????µÇ?ŞÀ???ÄÚµå ????? ??ÇØ ????
	SQLRowCount(hStmt, &AffectCount);

	SQLNumResultCols(hStmt, &nCol);
	if (nCol > MAXCOL) {
		//::MessageBox(NULL,"ÃÖ??ÄÃ?? ??? ÃÊ?úÇ????,"CQuery ????",MB_OK);
		gLogProc.LogAddFunc("CQuery error :ÃÖ??ÄÃ?? ??? ÃÊ?úÇ????n");
		return FALSE;
	}

	// nColÀÌ 0ÀÎ ????Select?? ÀÌ?ÜÀ???? ??ÉÀ? ?ÇÇàÇ???ìÀ???
	// ?ÙÀÎµùÀ? ÇÒ ÇÊ??? ???
	if (nCol == 0) {
		Clear();
		return TRUE;
	}

	// ?ğµ?ÄÃ??? ??ÀÚ?????ÙÀÎµùÇ????? Col???Àº zero base, 
	// ÄÃ?? ?øÈ???one baseÀÓ?? ?ÀÇÇÒ ??
	for (c = 0; c < nCol; c++) {
		SQLBindCol(hStmt, c + 1, SQL_C_CHAR, Col[c], 5000, &lCol[c]);
		SQLDescribeCol(hStmt, c + 1, ColName[c], 30, NULL, NULL, NULL, NULL, NULL);
	}
	return TRUE;
}

// SQL??? ?ÇÇàÇ?????ÂÀ?? ??????? ???ªÀ? ÀĞ????ÅÏÇØ ÁØ?? 
// ???? ?òÀ?ÈÄ ????îÁ?ÇØ ÁØ??
int CQuery::ExecGetInt(LPCTSTR szSQL)
{
	int i;

	gLogProc.LogAddFunc("%s", szSQL);
	//printf("%s\n", szSQL);

	if (Exec(szSQL) == FALSE)
		return CQUERYERROR;

	// ?ÀÌÅÍ?? ??????EOF?? ??ÅÏÇÑ??
	if (Fetch() == SQL_NO_DATA) {
		Clear();
		return CQUERYEOF;
	}
	i = GetInt(1);
	Clear();
	return i;
}

// SQL??? ?ÇÇàÇ?????ÂÀ?? ??????? ???ªÀ? ÀĞ????ÅÏÇØ ÁØ?? 
// ???? ?òÀ?ÈÄ ????îÁ?ÇØ ÁØ??
int CQuery::ExecGetInt(LPCTSTR szSQL, int* retvalue)
{
	int i;

	gLogProc.LogAddFunc("%s", szSQL);
	//printf("%s\n", szSQL);

	if (Exec(szSQL) == FALSE)
		return FALSE;

	// ?ÀÌÅÍ?? ??????EOF?? ??ÅÏÇÑ??
	if (Fetch() == SQL_NO_DATA) {
		Clear();
		return FALSE;
	}
	i = GetInt(1);
	Clear();
	*retvalue = i;

	return TRUE;
}


// SQL??? ?ÇÇàÇ?????ÂÀ?? ??????? ??ÀÚ??? ÀĞ????ÅÏÇØ ÁØ??
BOOL CQuery::ExecGetStr(LPCTSTR szSQL, char* buf)
{

	// SQL?? ?ÇÇàÁ??????? ??ıÇ??????ÀÚ?? ?öÆ?? ?????? ????????	
	if (Exec(szSQL) == FALSE) {
		//strcpy(buf, "CQUERYERROR");
		buf[0] = '\0';
		return FALSE;
	}

	// ?ÀÌÅÍ?? ??????EOF?? ??ÅÏÇÑ??
	if (Fetch() == SQL_NO_DATA) {
		Clear();
		//strcpy(buf,"EOF");
		buf[0] = '\0';
		return FALSE;
	}
	GetStr(1, buf);
	Clear();
	return TRUE;
}

// ??????? ÇÑ Çà? ??????
SQLRETURN CQuery::Fetch()
{
	ret = SQLFetch(hStmt);
	return ret;
}

// ????? ????ÙÀÎµ????? ÇØ?ÇÑ??
void CQuery::Clear()
{
	SQLCloseCursor(hStmt);
	//SQLFreeStmt();
	::SQLFreeStmt(hStmt, SQL_UNBIND);
}

// ÄÃ?? ÀÌ????ÎºÎÅ?ÄÃ?? ÀÎ???? ???? ?øÀ? ???-1? ??ÅÏÇÑ??
int CQuery::FindCol(char* name)
{
	int i;
	for (i = 0; i < nCol; i++) {
		if (_stricmp(name, (LPCTSTR)ColName[i]) == 0)
			return i + 1;
	}
	return -1;
}

// nColÀÇ ÄÃ???ªÀ? ????ÀĞ?îÁ?? NULLÀÏ ???CQUERYNULL? ??ÅÏÇÑ??
int CQuery::GetInt(int nCol)
{
	if (nCol > this->nCol)
		return CQUERYNOCOL;

	if (lCol[nCol - 1] == SQL_NULL_DATA) {
		return CQUERYNULL;
	}
	else {
		return atoi(Col[nCol - 1]);
	}
}

// sColÀÇ ÄÃ???ªÀ? ????ÀĞ?îÁ??
int CQuery::GetInt(char* sCol)
{
	int n;
	n = FindCol(sCol);
	if (n == -1) {
		return CQUERYNOCOL;
	}
	else {
		return GetInt(n);
	}
}


INT64 CQuery::GetInt64(int nCol)
{
	if (nCol > this->nCol)
		return CQUERYNOCOL;

	if (lCol[nCol - 1] == SQL_NULL_DATA) {
		return CQUERYNULL;
	}
	else {
		return atoi(Col[nCol - 1]);
	}
}

// sColÀÇ ÄÃ???ªÀ? ????ÀĞ?îÁ??
INT64 CQuery::GetInt64(char* sCol)
{
	int n;
	n = FindCol(sCol);
	if (n == -1) {
		return CQUERYNOCOL;
	}
	else {
		return GetInt(n);
	}
}

// nColÀÇ ÄÃ???ªÀ? ????ÀĞ?îÁ?? NULLÀÏ ???CQUERYNULL? ??ÅÏÇÑ??
float CQuery::GetFloat(int nCol)
{
	if (nCol > this->nCol)
		return CQUERYNOCOL;

	if (lCol[nCol - 1] == SQL_NULL_DATA) {
		return CQUERYNULL;
	}
	else {
		return (float)atof(Col[nCol - 1]);
	}
}

// sColÀÇ ÄÃ???ªÀ? ????ÀĞ?îÁ??
float CQuery::GetFloat(char* sCol)
{
	int n;
	n = FindCol(sCol);
	if (n == -1) {
		return CQUERYNOCOL;
	}
	else {
		return GetFloat(n);
	}
}


// nColÀÇ ÄÃ???ªÀ? ??ÀÚ????ÀĞ?îÁ?? NULLÀÏ ?????ÀÚ???? NULL? ??öÁ?? 
// bufÀÇ ?æÀ??ÃÖ?ÒÇ?256ÀÌ???ÇÏ???æÀ???ËÀ?ÇÏÁö ????
void CQuery::GetStr(int nCol, char* buf)
{
	if (nCol > this->nCol) {
		//strcpy(buf, "CQUERYNOCOL");
		buf[0] = '\0';
		return;
	}

	if (lCol[nCol - 1] == SQL_NULL_DATA)
	{
		buf[0] = '\0';
		//lstrcpy(buf,"NULL");
	}
	else {
		lstrcpy(buf, Col[nCol - 1]);
	}

}

// sColÀÇ ÄÃ???ªÀ? ??ÀÚ????ÀĞ?îÁ??
void CQuery::GetStr(char* sCol, char* buf)
{
	int n;
	n = FindCol(sCol);
	if (n == -1) {
		//lstrcpy(buf,"ERROR:Colume not found");
		buf[0] = '\0';
	}
	else {
		GetStr(n, buf);
	}
}

// ???? ????Áø?????? Ãâ?ÂÇ?ÁØ??
void CQuery::PrintDiag()
{
	int ii;
	SQLRETURN Ret;
	SQLINTEGER NativeError;
	SQLCHAR SqlState[6], Msg[255];
	SQLSMALLINT MsgLen;
	char str[256];

	ii = 1;
	while (Ret = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, ii, SqlState, &NativeError,
		Msg, sizeof(Msg), &MsgLen) != SQL_NO_DATA) {
		wsprintf(str, "SQLSTATE:%s, Áø?ÜÁ??:%s", (LPCTSTR)SqlState, (LPCTSTR)Msg);
		//::MessageBox(NULL,str,"Áø????",0);
		//g_Log.Log(str);
		//LogAddTD(str);
		printf("%s\n", str);
		ii++;
	}

	// ???? Åë???????. ?????ÓÇ????
	if (strcmp((LPCTSTR)SqlState, "08S01") == 0 ||
		strcmp((LPCTSTR)SqlState, "08001") == 0)
	{
		ReConnect();
	}

	//assert(0&&"?ÀÌÅÍ?ÀÌ??????");
	// ??? ºÒ???ÉÇ??????? ??ıÇ????ÇÁ?????? ??áÇ?? ??ÜÀ?????????? ºÒÇÊ?äÇ?
	// ????ÀÌ ÄÚµå?? ÁÖ?? ???ÇÏ???Àû?çÇ????? ?çÆ????????ÇÑ??
/*	if (ii > 1) {
		::MessageBox(NULL,"Áø?????? Ãâ?Âµ????? ÀÌ ???? Ãâ?Âµ?????????? ???? DB ????ÁßÁö µîÀÇ\r\n"
			"??? ºÒ???ÉÇ??????? ??ıÇ???ìÀ??ÇÁ????? ?ÇÇàÀ? ??ÓÇ????????\r\n"
			"?????? ?öÁ?ÇÏ??ÈÄ ÇÁ?????? ????ÇÇàÇ?ÁÖ????","Critical Error",MB_OK | MB_ICONERROR);

		// ?ÙÀ? µÑ Áß ÇÏ??? ??ÅÃÇÒ ??
		PostQuitMessage(0);
		// ExitProcess(0);
	}
*/
}

// BLOB ?ÀÌÅÍ?? buf?? ??öÁ?? ÀÌ?? buf??ÃæºĞÇÑ ??âÀ??????? ??? ÇÒ?çÇ?
// ????ÇÑ?? NULLÀÏ ???0? ??ÅÏÇÏ?????? ????-1? ??ÅÏÇÑ?? ????ÀĞÀº 
// ÃÑ ?ÙÀÌÆ? ??? ??ÅÏÇÑ?? szSQLÀº ÇÏ?ªÀ?BLOB ÇÊµå?? ÀĞ??Select SQL??ÀÌ???ÇÑ??
int CQuery::ReadBlob(LPCTSTR szSQL, void* buf)
{
	SQLCHAR BinaryPtr[BLOBBATCH];
	SQLINTEGER LenBin;
	char* p;
	int nGet;
	int TotalGet = 0;

	//LogAddTD("%s", szSQL);
	//printf("%s\n", szSQL);

	ret = SQLExecDirect(hStmt, (SQLCHAR*)szSQL, SQL_NTS);
	if (ret != SQL_SUCCESS) {
		PrintDiag();
		return -1;
	}

	while ((ret = SQLFetch(hStmt)) != SQL_NO_DATA)
	{
		p = (char*)buf;
		while ((ret = SQLGetData(hStmt, 1, SQL_C_BINARY, BinaryPtr, sizeof(BinaryPtr), &LenBin)) != SQL_NO_DATA)
		{
			if (LenBin == SQL_NULL_DATA)
			{
				Clear();
				return 0;
			}
			if (ret == SQL_SUCCESS)
				nGet = LenBin;
			else
				nGet = BLOBBATCH;
			TotalGet += nGet;
			memcpy(p, BinaryPtr, nGet);
			p += nGet;
		}
	}
	Clear();
	return TotalGet;
}

// bufÀÇ BLOB ?ÀÌÅÍ?? ÀúÀåÇÑ?? szSQLÀº ÇÏ?ªÀ?BLOB ?ÀÌÅÍ?? ÀúÀåÇÏ??Update, Insert
// SQL??ÀÌ???ÇÑ??
void CQuery::WriteBlob(LPCTSTR szSQL, void* buf, int size)
{
	SQLINTEGER cbBlob;
	char tmp[BLOBBATCH], * p;
	SQLPOINTER pToken;
	int nPut;

	gLogProc.LogAddFunc("%s", szSQL);
	//printf("%s\n", szSQL);

	cbBlob = SQL_LEN_DATA_AT_EXEC(size);
	SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY,
		size, 0, (SQLPOINTER)1, 0, &cbBlob);
	SQLExecDirect(hStmt, (SQLCHAR*)szSQL, SQL_NTS);
	ret = SQLParamData(hStmt, &pToken);
	while (ret == SQL_NEED_DATA)
	{
		if (ret == SQL_NEED_DATA)
		{
			if ((int)pToken == 1)
			{
				for (p = (char*)buf; p < (char*)buf + size; p += BLOBBATCH)
				{
					nPut = min(BLOBBATCH, (char*)buf + size - p);
					memcpy(tmp, p, nPut);
					SQLPutData(hStmt, (PTR)tmp, nPut);
				}
			}
		}
		ret = SQLParamData(hStmt, &pToken);
	}
	Clear();
}
