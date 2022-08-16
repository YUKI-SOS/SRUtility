#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <future>
//#include <type_traits>


//https://docs.microsoft.com/en-us/windows/win32/sync/using-condition-variables msdn에 winapi버전 조건변수condition variable
//std::condition_variable클래스의 wait나 notify_one 등을 타고 올라가면 결국 winapi SleepConditionVariableCS() WakeConditionVariable()이 나온다.

//https://modoocode.com/285 두번째 코드 분석.

class CThreadPool 
{
public:
	CThreadPool(size_t cnt): threadNum(cnt), stopAll(false)
	{
		workerThreads.reserve(threadNum);
		for (size_t i = 0; i < threadNum; i++) 
		{
			//emplace_back 은 인자를 넘겨서 객체의 생성자로 받아 벡터에 원소를 추가하는 함수.
			//std::thread([this](){this->WorkerThread();}) 형태로 스레드 생성 후 실행까지.
			workerThreads.emplace_back([this]() {this->WorkerThread(); });
		}
	};
	~CThreadPool() 
	{
		stopAll = true;
		cv.notify_all(); //wait했던 스레드를 전부 깨운다.
		for (auto& elements : workerThreads)
			elements.join(); //스레드들이 다 종료했을 때까지 대기.
	};
public:
	
	//생산자-소비자 패턴으로 생산자는 할 일들을 큐에 추가하고 소비자는 큐에 무언가 있으면 잠들어 있다가 깨어나서 일을 처리한다.
	void WorkerThread() 
	{
		while (true)
		{
			//EnqueueJob에서 std::lock_guard와는 달리 std::unique_lock을 쓰는 이유는 기본적으로 스코프를 벗어날 때
			//unlock되는 건 같지만 중간에 lock unlock을 추가로 할 수 있어서.
			//그리고 여기서 락을 거는 이유는 wait함수가 인자로 락을 필요로 하기 때문.
			std::unique_lock<std::mutex> lock(mt);
			//wait함수는 인자로 넘긴 조건에 해당할 때 까지 스레드를 재우고, unlock상태에 들어간다.
			//조건을 만족하면 반환하면서 다시 lock을 건다.
			//할 일을 모아놓은 큐가 비어있지 않으면 대기를 빠져나와야 한다.
			cv.wait(lock, [this]() {return !this->jobs.empty() || stopAll; });
			if (stopAll && jobs.empty()) return;

			//함수포인터라 역시 우측값으로 바꾸는게 큰 의미는 없어 보인다.
			std::function<void()> job = std::move(jobs.front());
			jobs.pop();
			lock.unlock();
			//큐에서 얻은 함수포인터 실행
			job();
		}
	};

	template<typename T, typename... Args>
	std::future<typename std::result_of<T(Args...)>::type> EnqueueJob(T pFunc, Args... args) 
	{
		//if (stopAll) return;

		//넘어오는 함수가 int work(int, int) int타입 인자를 두 개 받아 int를 return해주는 함수일 때.
		//std::result_of<T(Args...)>::type은 리턴받은 int형. retType은 int형을 나타낸다.
		using retType = typename std::result_of<T(Args...)>::type;
		
		//shared_ptr과 packaged_task가 어떻게 선언되는지 확인용
		//std::packaged_task<int(int)> task(some_task);
		//std::shared_ptr<A> p1(new A());
		//std::shared_ptr<A> p1 = std::make_shared<A>();
		

		//std::packaged_task<retType()> job(std::bind(pFunc, args...));
		//std::future<retType> jobProcFuture = job.get_future();
		//위와 같이 쓰면job이 지역변수이기 때문에 future에 대해 promise가 set_value를 해주기 이전(workthread에서 job을 수행하기 전에)에
		//이미 EnqueueJob을 빠져나가 future객체는 파괴되었을 것이다.
		//이를 해결하기 위해 shared_ptr을 이용해서 packaged_task가 사용되는 동안 객체가 파괴되지 않도록 한다.
		//std::packaged_task<retType()>>은 retType이 int로 추론되었으며 ()인자가 없는 int타입의 인자없는 함수를 받는다.
		//std::shared_ptr<std::packaged_task<retType()>> job = new std::packaged_task<retType()>(std::bind(pFunc, args...));
		//풀어쓰면 이런 식이 될 것으로 보이며, std::bind(pFunc, args...)를 통해 int 타입을 리턴하는 인자가 없는 callable이 되고 이를
		//std::packaged_task<retType()>>이 int타입의 인자가 없는 호출하는 형태에 맞춰진다.
		auto job = std::make_shared<std::packaged_task<retType()>>(std::bind(pFunc, args...));
		//future와 promise 연결
		std::future<retType> jobProcFuture = job->get_future();
		
		//스코프를 벗어나면 락이 해제되면서 예외발생시에도 안전하게 락을 풀어주는 기능을 한다.
		//할 일을 큐에 넣는 부분까지만 락을 걸기 위한 {}스코프. 의미가 있는 중괄호임.
		{
			std::lock_guard<std::mutex> lock(mt);
			//packaged_task를 수행하는 람다. (*job)해서 전달받은 pFunc함수포인터를 나타내고 그를 수행하는 callable을 큐에 추가.
			//큐는 std::function<void()> 타입이었기 때문에 람다로 싸서 타입을 맞춰준 것.
			jobs.push([job]() {(*job)();});
		}
		//wait한 스레드 중 하나를 깨운다.
		cv.notify_one();

		return jobProcFuture;
	};

public:
	size_t threadNum;
	std::condition_variable cv; //조건변수
	std::mutex mt; //뮤텍스
	std::vector<std::thread> workerThreads; //워커 스레드 보관 자료구조
	std::queue<std::function<void()>> jobs; //함수단위 할 일 들을 보관할 큐
	bool stopAll; //스레드 종료 플래그
};

//version1
//https://modoocode.com/285 첫번째 코드 분석.
//리턴타입이 있는 함수를 전달한 경우 리턴값을 받을 수 없는 문제.
//void타입 함수만 큐에 넣을 수 있어 다른 타입은 람다로 감싸서 전달해야만 하는 문제.
//
/*
class CThreadPool
{
public:
	CThreadPool(size_t cnt): threadNum(cnt), stopAll(false)
	{
		workerThreads.reserve(threadNum);
		for (size_t i = 0; i < threadNum; i++)
		{
			//emplace_back 은 인자를 넘겨서 객체의 생성자로 받아 벡터에 원소를 추가하는 함수.
			//std::thread([this](){this->WorkerThread();}) 형태로 스레드 생성 후 실행까지.
			workerThreads.emplace_back([this]() {this->WorkerThread(); });
		}
	};
	~CThreadPool()
	{
		stopAll = true;
		cv.notify_all(); //wait했던 스레드를 전부 깨운다.
		for (auto& elements : workerThreads)
			elements.join(); //스레드들이 다 종료했을 때까지 대기.
	};
public:

	//생산자-소비자 패턴으로 생산자는 할 일들을 큐에 추가하고 소비자는 큐에 무언가 있으면 잠들어 있다가 깨어나서 일을 처리한다.
	void WorkerThread()
	{
		while (true)
		{
			//EnqueueJob에서 std::lock_guard와는 달리 std::unique_lock을 쓰는 이유는 기본적으로 스코프를 벗어날 때
			//unlock되는 건 같지만 중간에 lock unlock을 추가로 할 수 있어서.
			//그리고 여기서 락을 거는 이유는 wait함수가 인자로 락을 필요로 하기 때문.
			std::unique_lock<std::mutex> lock(mt);
			//wait함수는 인자로 넘긴 조건에 해당할 때 까지 스레드를 재우고, unlock상태에 들어간다.
			//조건을 만족하면 반환하면서 다시 lock을 건다.
			//할 일을 모아놓은 큐가 비어있지 않으면 대기를 빠져나와야 한다.
			cv.wait(lock, [this]() {return !this->jobs.empty() || stopAll; });
			if (stopAll && jobs.empty()) return;

			//함수포인터라 역시 우측값으로 바꾸는게 큰 의미는 없어 보인다.
			std::function<void()> job = std::move(jobs.front());
			jobs.pop();
			lock.unlock();
			//큐에서 얻은 함수포인터 실행
			job();
		}
	};
	void EnqueueJob(std::function<void()> job)
	{
		if (stopAll) return;
		//스코프를 벗어나면 락이 해제되면서 예외발생시에도 안전하게 락을 풀어주는 기능을 한다.
		//할 일을 큐에 넣는 부분까지만 락을 걸기 위한 {}스코프. 의미가 있는 중괄호임.
		{
			std::lock_guard<std::mutex> lock(mt);
			//함수 포인터임으로 별다른 복사생성같은 게 일어나지는 않을테니 우측값으로 바꿔주는 move는 큰 의미는 없을 듯.
			jobs.push(std::move(job));
		}
		//wait한 스레드 중 하나를 깨운다.
		cv.notify_one();
	};
public:
	size_t threadNum;
	std::condition_variable cv; //조건변수
	std::mutex mt; //뮤텍스
	std::vector<std::thread> workerThreads; //워커 스레드 보관 자료구조
	std::queue<std::function<void()>> jobs; //함수단위 할 일 들을 보관할 큐
	bool stopAll; //스레드 종료 플래그
};
*/