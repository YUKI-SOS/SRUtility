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

		//FILETIME 타입은 64비트 정수형.
		//1601년 1월 1일 0시 0분 0초를 0으로 지정하며 1초에 10,000,000이 늘어난다.
		//100나노초의 해상도.
		//경과시간을 구할 때는 32비트 정수형 두개를 사용하지 않고 ULARGE_INTEGER형에 대입한 후 QuadPart를 64비트 정수 연산을 이용한다. 
		ulDueTime.QuadPart = (ULONGLONG)-(dueTime * 10LL * 1000LL);
		if (dueTime == -1) 
		{
			ulDueTime.QuadPart = (ULONGLONG)-(1LL);
		}
		fileDueTime.dwHighDateTime = ulDueTime.HighPart;
		fileDueTime.dwLowDateTime = ulDueTime.LowPart;


		SetThreadpoolTimer(timer, //CreateThreadpoolTimer의 반환값
			&fileDueTime, //콜백함수를 최초로 실행해야 되는 시간. 음수면 상대적인 시간(밀리초...라는데 파일타임은 천만에 일초?)  dwLowDateTime이 -1이면 바로 실행.
			period, //콜백함수가 한 번만 호출되길 원한다면 0을 전달. 주기적으로 호출하길 원한다면 밀리초 단위로 설정.
			msWinLength //pftDueTime으로 부터 msWindowLength값을 더한 시간 범위 값 내에서 임의로 호출. 비슷한 주기의 타이머가 있을 경우 여러 통지가 발생하거나 슬립으로 처리하지 않고 하나의 스레드가 처리할 수 있도록 사용하는 방법이 있음.
		);
	};

	//이벤트 동기화 객체 이용하는 경우인데 삭제나 호출하는 함수등이 달라서 별도로 구현하자. 
	/*
	void AddWaitCallback()
	{
	
	};
	*/

	void CleanUp()
	{
		//break없이 쭉 타고 들어감.
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

//콜백 함수 정의
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
//람다식이 캡처를 하지 않았을 경우에는 함수포인터에 대입될 수 있지만 엄밀히 클로져객체는 함수포인터가 아니기때문에 캡쳐할 경우에는 함수포인터로 변환할 수 없다.
winThreadPool.AddWorkCallback([](PTP_CALLBACK_INSTANCE, PVOID, PTP_WORK){_tprintf(_T("lambda work callback.\n")); }, NULL);
//람다를 쓰면서 캡쳐하지 못함으로 PVOID에 void*형 인자를 넣어서 캡쳐대신 쓰자.
//예를들어 클래스 멤버에서 쓸 경우에 PVOID에 this를 넘긴다던가 해서 멤버함수를 쓰면 되겠다.
int count = 0;
int* pCount = &count;
 winThreadPool.AddWorkCallback([](PTP_CALLBACK_INSTANCE, PVOID vp, PTP_WORK)
		{
			int* count = (int*)vp;
			(*count)++;
			_tprintf(_T("count: %d\n"), *count);
		}, pCount);

*/