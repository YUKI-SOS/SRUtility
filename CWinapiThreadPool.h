#pragma once
#include <Windows.h>

class CWinapiThreadPool
{
public:
	CWinapiThreadPool(DWORD threadCntMin, DWORD threadCntMax)
	{
		rollback = 0;

		InitializeThreadpoolEnvironment(&callbackEnviron);
		
		pool = CreateThreadpool(NULL);
		if (pool == NULL) 
		{
			OutputDebugString(L"createThreadPool fail");
			goto CLEANUP;
		}

		rollback = 1;

		SetThreadpoolThreadMaximum(pool, threadCntMax);
		if (SetThreadpoolThreadMinimum(pool, threadCntMin) == FALSE) 
		{
			OutputDebugString(L"SetThreadpoolThreadMinimum fail");
			goto CLEANUP;
		};

		cleanupgroup = CreateThreadpoolCleanupGroup();
		if (cleanupgroup == NULL) 
		{
			OutputDebugString(L"CreateThreadpoolCleanupGroup fail");
			goto CLEANUP;
		}

		rollback = 2;

		SetThreadpoolCallbackPool(&callbackEnviron, pool);

		SetThreadpoolCallbackCleanupGroup(&callbackEnviron, cleanupgroup, NULL);

		rollback = 0;

	CLEANUP:
		CleanUp();

	};
	~CWinapiThreadPool() 
	{
		CleanUp();
	};

public:
	void AddWorkCallback(PTP_WORK_CALLBACK callback, PVOID pv) 
	{
		PTP_WORK work = CreateThreadpoolWork(callback, pv, &callbackEnviron);
		if (work == NULL)
		{
			OutputDebugString(L"CreateThreadpoolWork fail");
			CleanUp();
			return;
		}

		rollback = 3;

		SubmitThreadpoolWork(work);
	};
	
	void AddTimerCallback(PTP_TIMER_CALLBACK callback, PVOID pv, DWORD dueTime, DWORD period, DWORD msWinLength)
	{
		PTP_TIMER timer = CreateThreadpoolTimer(callback, pv, &callbackEnviron);
		if (timer == NULL) 
		{
			OutputDebugString(L"CreateThreadpoolTimer fail");
			CleanUp();
			return;
		}

		rollback = 3;

		ULARGE_INTEGER ulDueTime;
		FILETIME fileDueTime;

		//FILETIME Ÿ���� 64��Ʈ ������.
		//1601�� 1�� 1�� 0�� 0�� 0�ʸ� 0���� �����ϸ� 1�ʿ� 10,000,000�� �þ��.
		//100�������� �ػ�.
		//����ð��� ���� ���� 32��Ʈ ������ �ΰ��� ������� �ʰ� ULARGE_INTEGER���� ������ �� QuadPart�� 64��Ʈ ���� ������ �̿��Ѵ�. 
		ulDueTime.QuadPart = (ULONGLONG)-(dueTime * 10LL * 1000LL);
		if (dueTime == -1) 
		{
			ulDueTime.QuadPart = (ULONGLONG)-(1LL);
		}
		fileDueTime.dwHighDateTime = ulDueTime.HighPart;
		fileDueTime.dwLowDateTime = ulDueTime.LowPart;


		SetThreadpoolTimer(timer, //CreateThreadpoolTimer�� ��ȯ��
			&fileDueTime, //�ݹ��Լ��� ���ʷ� �����ؾ� �Ǵ� �ð�. ������ ������� �ð�(�и���...��µ� ����Ÿ���� õ���� ����?)  dwLowDateTime�� -1�̸� �ٷ� ����.
			period, //�ݹ��Լ��� �� ���� ȣ��Ǳ� ���Ѵٸ� 0�� ����. �ֱ������� ȣ���ϱ� ���Ѵٸ� �и��� ������ ����.
			msWinLength //pftDueTime���� ���� msWindowLength���� ���� �ð� ���� �� ������ ���Ƿ� ȣ��. ����� �ֱ��� Ÿ�̸Ӱ� ���� ��� ���� ������ �߻��ϰų� �������� ó������ �ʰ� �ϳ��� �����尡 ó���� �� �ֵ��� ����ϴ� ����� ����.
		);
	};

	//�̺�Ʈ ����ȭ ��ü �̿��ϴ� ����ε� ������ ȣ���ϴ� �Լ����� �޶� ������ ��������. 
	/*
	void AddWaitCallback()
	{
	
	};
	*/

	void CleanUp()
	{
		//break���� �� Ÿ�� ��.
		switch (rollback)
		{
		case 3:
			CloseThreadpoolCleanupGroupMembers(cleanupgroup, FALSE, NULL);
		case 2:
			CloseThreadpoolCleanupGroup(cleanupgroup);
		case 1:
			CloseThreadpool(pool);
		default:
			rollback = 0;
			break;
		}
	};
public:
	PTP_POOL pool = NULL;
	TP_CALLBACK_ENVIRON callbackEnviron;
	PTP_CLEANUP_GROUP cleanupgroup = NULL;
	UINT rollback;
};

//�ݹ� �Լ� ����
/*
VOID CALLBACK MyWaitCallback(
	PTP_CALLBACK_INSTANCE Instance,
	PVOID                 Parameter,
	PTP_WAIT              Wait,
	TP_WAIT_RESULT        WaitResult
)
{
	// Instance, Parameter, Wait, and WaitResult not used in this example.
	UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(Parameter);
	UNREFERENCED_PARAMETER(Wait);
	UNREFERENCED_PARAMETER(WaitResult);

	//
	// Do something when the wait is over.
	//
	_tprintf(_T("MyWaitCallback: wait is over.\n"));
}


//
// Thread pool timer callback function template
//
VOID CALLBACK MyTimerCallback(
	PTP_CALLBACK_INSTANCE Instance,
	PVOID                 Parameter,
	PTP_TIMER             Timer
)
{
	// Instance, Parameter, and Timer not used in this example.
	UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(Parameter);
	UNREFERENCED_PARAMETER(Timer);

	//
	// Do something when the timer fires.
	//
	_tprintf(_T("MyTimerCallback: timer has fired.\n"));

}
CWinapiThreadPool winThreadPool(1, 1);
winThreadPool.AddWorkCallback(MyWorkCallback, NULL);
winThreadPool.AddTimerCallback(MyTimerCallback, NULL, -1, 1000, 0);
//���ٽ��� ĸó�� ���� �ʾ��� ��쿡�� �Լ������Ϳ� ���Ե� �� ������ ������ Ŭ������ü�� �Լ������Ͱ� �ƴϱ⶧���� ĸ���� ��쿡�� �Լ������ͷ� ��ȯ�� �� ����.
winThreadPool.AddWorkCallback([](PTP_CALLBACK_INSTANCE, PVOID, PTP_WORK){_tprintf(_T("lambda work callback.\n")); }, NULL);
//���ٸ� ���鼭 ĸ������ �������� PVOID�� void*�� ���ڸ� �־ ĸ�Ĵ�� ����.
//������� Ŭ���� ������� �� ��쿡 PVOID�� this�� �ѱ�ٴ��� �ؼ� ����Լ��� ���� �ǰڴ�.
int count = 0;
int* pCount = &count;
 winThreadPool.AddWorkCallback([](PTP_CALLBACK_INSTANCE, PVOID vp, PTP_WORK)
		{
			int* count = (int*)vp;
			(*count)++;
			_tprintf(_T("count: %d\n"), *count);
		}, pCount);

*/