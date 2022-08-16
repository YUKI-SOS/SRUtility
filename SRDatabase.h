#pragma once
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <sql.h>
#include <sqlext.h>
//#include <afxdb.h> //mfc odbc ��� 
const int FiledLength = 20;

//ODBC�� �����ͺ��̽� ���ӿ� Ŭ����
//������ ����Ϸ��� ����� ODBCĿ���� ���� ��ġ�Ͽ� DSN������ �ؾ� �Ѵ�.
//�翬�� ��� ���� �Ǿ� �־�߸� ��.

//https://docs.microsoft.com/ko-kr/sql/connect/odbc/cpp-code-example-app-connect-access-sql-db?view=sql-server-ver15
//����ũ�μ���Ʈ odbc ���� ���

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
		//ODBC ����� ����ϱ� ���� ȯ�� ����
		if (SQLAllocHandle(SQL_HANDLE_ENV, //�ڵ��� Ÿ���� ���� env :ȯ�漳��, dbc :���ἳ��, stmt :�������, desc : ����
			SQL_NULL_HANDLE, //�Է¿� ���� �ڵ� dbc�� env �ʿ�, stmt�� obc�ʿ�
			&hEnv)			//�Է� �޴� �ڵ�
			!= SQL_SUCCESS) 
		{
			std::cout << "Unable to allocate an environment handle" << std::endl;
			return FALSE;
		}

		if (SQLSetEnvAttr(hEnv,
			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, //ODBC ����
			SQL_IS_INTEGER) != SQL_SUCCESS)
		{
			std::cout << "Error SQLSetEnvAttr()" << std::endl;
			return FALSE;
		}

		//ODBC �Լ��� ����ϱ� ���� ���� ����
		if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS)
		{
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_DBC)" << std::endl;
			return FALSE;
		}

		//DB ����
		if (SQLConnect(hDbc, (SQLTCHAR*)dsn, SQL_NTS, (SQLTCHAR*)id, SQL_NTS, (SQLTCHAR*)pwd, SQL_NTS) != SQL_SUCCESS)
		{
			FreeHandle();
			std::cout << "DB Connect Fail" << std::endl;
			return FALSE;
		}
		
		//Query���� ���� ��� �ڵ� �Ҵ�
		if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_STMT)" << std::endl;

		return TRUE;

	};

	BOOL DriverConnect(const TCHAR* connectString) 
	{
		//ODBC ����� ����ϱ� ���� ȯ�� ����
		if (SQLAllocHandle(SQL_HANDLE_ENV, //�ڵ��� Ÿ���� ���� env :ȯ�漳��, dbc :���ἳ��, stmt :�������, desc : ����
			SQL_NULL_HANDLE, //�Է¿� ���� �ڵ� dbc�� env �ʿ�, stmt�� obc�ʿ�
			&hEnv)			//�Է� �޴� �ڵ�
			!= SQL_SUCCESS)
		{
			std::cout << "Unable to allocate an environment handle" << std::endl;
			return FALSE;
		}

		if (SQLSetEnvAttr(hEnv,
			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3, //ODBC ����
			SQL_IS_INTEGER) != SQL_SUCCESS)
		{
			std::cout << "Error SQLSetEnvAttr()" << std::endl;
			return FALSE;
		}

		//ODBC �Լ��� ����ϱ� ���� ���� ����
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

		//Query���� ���� ��� �ڵ� �Ҵ�
		if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
			std::cout << "Error SQLAllocHandle(SQL_HANDLE_STMT)" << std::endl;

		return TRUE;
	};

	//���� ����
	BOOL Excute(const TCHAR* query)
	{
		if (SQLExecDirect(hStmt, (SQLTCHAR*)query, SQL_NTS) != SQL_SUCCESS)
		{
			std::cout << "Error Excute Query" << std::endl;
			return FALSE;
		};

		//�������� ������ ���ƿ� ������ ���� ����.
		SQLNumResultCols(hStmt, &numCols);

		return TRUE;
	}

	//DB ���̺��� colum�� ����
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
				SQL_DESC_DISPLAY_SIZE, //���� �����͸� ǥ�� �ϴ� �� �ʿ��� �ִ� ���� ��
				NULL,
				0,
				NULL,
				&columSize) != SQL_SUCCESS)
			{
				std::cout << "Error Get Columsize" << std::endl;
				return FALSE;
			};

			//���� Ÿ���� SQLColAttribute(3��° ���� SQL_DESC_CONCISE_TYPE)���� ������ �� ������ ������
			//��� TCHAR�� ���� �����̱� ������ ���� ����.

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

	//row(���ڵ�)�� ���� �� ���ε��� �÷� ���ۿ� �޾ƿ´�.
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

	//������ ���� ���� Get�� �ڿ� Ǯ���ش�. ���� ���� ������ �������� ����� ��.
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

	//���� ���� ��
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

//����
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