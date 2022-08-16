#pragma once
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <sql.h>
#include <sqlext.h>
//#include <afxdb.h> //mfc odbc 헤더 
const int FiledLength = 20;

//ODBC로 데이터베이스 접속용 클래스
//사전에 사용하려는 디비의 ODBC커넥터 등을 설치하여 DSN설정을 해야 한다.
//당연히 디비가 실행 되어 있어야만 함.

//https://docs.microsoft.com/ko-kr/sql/connect/odbc/cpp-code-example-app-connect-access-sql-db?view=sql-server-ver15
//마이크로소프트 odbc 샘플 기반

typedef struct STR_BINDING {
	TCHAR* wszFieldName;    //colums name
	TCHAR* wszBuffer;   //display buffer
	SQLLEN recvSize;    //size or null
	struct STR_BINDING* pNext;  //linked list
} BINDING;


class SRDatabase
{
public:
	SRDatabase(const TCHAR* dsn, const TCHAR* id, const TCHAR* pwd)
	{
		Connect(dsn, id, pwd);
	};
	SRDatabase(const TCHAR* conStr) 
	{
		DriverConnect(conStr);
	};
	~SRDatabase() {};
public:
	BOOL Connect(const TCHAR* dsn, const TCHAR* id, const TCHAR* pwd)
	{
		//ODBC 기술을 사용하기 위한 환경 구성
		if (SQLAllocHandle(SQL_HANDLE_ENV, //핸들의 타입을 설정 env :환경설정, dbc :연결설정, stmt :명령전달, desc : 설명
			SQL_NULL_HANDLE, //입력에 사용될 핸들 dbc는 env 필요, stmt는 obc필요
			&hEnv)			//입력 받는 핸들
			!= SQL_SUCCESS) 
		{
			std::cout << "Unable to allocate an environment handle" << std::endl;
			return FALSE;
		}

		if (SQLSetEnvAttr(hEnv,
			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, //ODBC 버전
			SQL_IS_INTEGER) != SQL_SUCCESS)
		{
			std::cout << "Error SQLSetEnvAttr()" << std::endl;
			return FALSE;
		}

		//ODBC 함수를 사용하기 위한 정보 구성
		if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS)
		{
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_DBC)" << std::endl;
			return FALSE;
		}

		//DB 연결
		if (SQLConnect(hDbc, (SQLTCHAR*)dsn, SQL_NTS, (SQLTCHAR*)id, SQL_NTS, (SQLTCHAR*)pwd, SQL_NTS) != SQL_SUCCESS)
		{
			FreeHandle();
			std::cout << "DB Connect Fail" << std::endl;
			return FALSE;
		}
		
		//Query문을 위해 명령 핸들 할당
		if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_STMT)" << std::endl;

		return TRUE;

	};

	BOOL DriverConnect(const TCHAR* connectString) 
	{
		//ODBC 기술을 사용하기 위한 환경 구성
		if (SQLAllocHandle(SQL_HANDLE_ENV, //핸들의 타입을 설정 env :환경설정, dbc :연결설정, stmt :명령전달, desc : 설명
			SQL_NULL_HANDLE, //입력에 사용될 핸들 dbc는 env 필요, stmt는 obc필요
			&hEnv)			//입력 받는 핸들
			!= SQL_SUCCESS)
		{
			std::cout << "Unable to allocate an environment handle" << std::endl;
			return FALSE;
		}

		if (SQLSetEnvAttr(hEnv,
			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, //ODBC 버전
			SQL_IS_INTEGER) != SQL_SUCCESS)
		{
			std::cout << "Error SQLSetEnvAttr()" << std::endl;
			return FALSE;
		}

		//ODBC 함수를 사용하기 위한 정보 구성
		if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS) 
		{
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_DBC)" << std::endl;
			return FALSE;
		}

		if (SQLDriverConnect(hDbc, NULL,(SQLTCHAR*)connectString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) != SQL_SUCCESS)
		{
			FreeHandle();
			std::cout << "DB Connect Fail" << std::endl;
			return FALSE;
		}

		//Query문을 위해 명령 핸들 할당
		if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_STMT)" << std::endl;

		return TRUE;
	};

	//쿼리 실행
	BOOL Excute(const TCHAR* query)
	{
		if (SQLExecDirect(hStmt, (SQLTCHAR*)query, SQL_NTS) != SQL_SUCCESS)
		{
			std::cout << "Error Excute Query" << std::endl;
			return FALSE;
		};

		//쿼리문을 보내서 돌아온 응답의 열의 개수.
		SQLNumResultCols(hStmt, &numCols);

		return TRUE;
	}

	//DB 테이블의 colum과 연결
	BOOL BindingsCols() 
	{
		if (BindingsCols(hStmt, numCols, &pFirstBinding))
			return TRUE;

		return FALSE;
	}
	BOOL BindingsCols(HSTMT hstmt, SQLSMALLINT numcols, BINDING** ppBinding)
	{
		int indexCols;
		BINDING* pBinding = NULL;
		BINDING* pLastBinding = NULL;
		SQLLEN columSize;

		for (indexCols = 1; indexCols <= numcols; indexCols++)
		{
			pBinding = new BINDING;
			if (!pBinding)
			{
				return FALSE;
			}

			if (indexCols == 1)
				*ppBinding = pBinding;
			else
				pLastBinding->pNext = pBinding;

			pLastBinding = pBinding;

			if (SQLColAttribute(hstmt,
				indexCols,
				SQL_DESC_DISPLAY_SIZE, //열의 데이터를 표시 하는 데 필요한 최대 문자 수
				NULL,
				0,
				NULL,
				&columSize) != SQL_SUCCESS)
			{
				std::cout << "Error Get Columsize" << std::endl;
				return FALSE;
			};

			//열의 타입을 SQLColAttribute(3번째 인자 SQL_DESC_CONCISE_TYPE)으로 가져올 수 있으나 어차피
			//모두 TCHAR로 받을 예정이기 때문에 쓰지 않음.

			pBinding->pNext = NULL;

			pBinding->wszFieldName = new TCHAR[FiledLength + 1];

			if (SQLColAttribute(hstmt,
				indexCols,
				SQL_DESC_NAME,
				pBinding->wszFieldName,
				FiledLength + 1,
				NULL,
				NULL) != SQL_SUCCESS)
			{
				std::cout << "Error Get Filed Name" << std::endl;
			}

			pBinding->wszBuffer = new TCHAR[columSize + 1];

			if (!(pBinding->wszBuffer))
				return FALSE;

			if (SQLBindCol(hstmt,
				indexCols,
				SQL_C_TCHAR,
				pBinding->wszBuffer,
				(columSize + 1) * sizeof(TCHAR),
				&pBinding->recvSize) != SQL_SUCCESS)
			{
				std::cout << "Error SqlBindCol" << std::endl;
				return FALSE;
			}
		}

	}

	BOOL Get(const TCHAR* fieldName, TCHAR* fieldValue, size_t fieldSize) 
	{
		for (pBinding = pFirstBinding; pBinding; pBinding = pBinding->pNext)
		{
			if (_tcscmp(fieldName, pBinding->wszFieldName) == 0)
			{
				if (fieldSize < pBinding->recvSize + 1) 
					return FALSE;
				
				ZeroMemory(fieldValue, fieldSize);
				memcpy(fieldValue, pBinding->wszBuffer, pBinding->recvSize + 1);
				return TRUE;
			}
		}

		return FALSE;
	}

	//row(레코드)를 한줄 씩 바인딩한 컬럼 버퍼에 받아온다.
	BOOL FetchNext() 
	{
		if (SQLFetchScroll(hStmt, SQL_FETCH_NEXT, 0) != SQL_SUCCESS)
		{
			return FALSE;
		}

		return TRUE;
	}
	BOOL FetchPREV() 
	{
		if (SQLFetchScroll(hStmt, SQL_FETCH_PRIOR, 0) != SQL_SUCCESS) 
		{
			return FALSE;
		}

		return TRUE;
	}
	BOOL FetchFirst()
	{
		if (SQLFetchScroll(hStmt, SQL_FETCH_FIRST, 0) != SQL_SUCCESS)
		{
			return FALSE;
		}

		return TRUE;
	}
	BOOL FetchLast() 
	{
		if (SQLFetchScroll(hStmt, SQL_FETCH_LAST, 0) != SQL_SUCCESS)
		{
			return FALSE;
		}

		return TRUE;
	}

	BOOL IsFetchLast() 
	{
		if (SQLFetch(hStmt) == SQL_NO_DATA) 
		{
			FetchPREV();
			return TRUE;
		}

		FetchPREV();
		return FALSE;
	}

	//쿼리에 대한 값을 Get한 뒤에 풀어준다. 다음 쿼리 보내기 전까지는 해줘야 함.
	void FreeBuffer() 
	{
		SQLFreeStmt(hStmt, SQL_CLOSE);

		while (pFirstBinding)
		{
			pBinding = pFirstBinding->pNext;
			delete[] pFirstBinding->wszFieldName;
			delete[] pFirstBinding->wszBuffer;
			delete pFirstBinding;
			pFirstBinding = pBinding;
		}
	}

	//연결 종료 시
	BOOL DisConnect() 
	{
		if (SQLDisconnect(hDbc) != SQL_SUCCESS) 
		{
			std::cout << "Error Disconnect" << std::endl;
			return FALSE;
		};

		return TRUE;
	}

	void FreeHandle()
	{
		if (hDbc != SQL_NULL_HDBC)
			SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
		if (hEnv != SQL_NULL_HENV)
			SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
		if (hStmt != SQL_NULL_HSTMT)
			SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
	}

public:
	SQLHENV hEnv;
	SQLHDBC hDbc;
	SQLHSTMT hStmt;
	SQLSMALLINT numCols;

	BINDING* pBinding;
	BINDING* pFirstBinding;
};

//사용법
/*
int main() 
{
	SRDatabase db(L"maria", L"root", L"root");
	//SRDatabase db(L"Driver={MariaDB ODBC 3.1 Driver};SERVER=localhost;USER=root;PASSWORD=root;DATABASE=defaultdb;PORT=3306");

	db.Excute(L"select * from defaulttable");
	db.BindingsCols();
	db.FetchFirst();

	while (1)
	{
		for (db.pBinding = db.pFirstBinding; db.pBinding; db.pBinding = db.pBinding->pNext)
		{
			if (db.pBinding->recvSize != SQL_NULL_DATA)
			{
				std::wcout << db.pBinding->wszBuffer;
			}
			else
			{
				std::wcout << "NULL";
			}
			std::wcout << " ";
		}

		if (db.IsFetchLast())
			break;

		db.FetchNext();
	}

	TCHAR id[20];
	db.Get(L"USERID", id, sizeof(id));

	db.DisConnect();
	db.FreeBuffer();
	db.FreeHandle();
}
*/