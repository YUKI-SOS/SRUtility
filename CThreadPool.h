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


//https://docs.microsoft.com/en-us/windows/win32/sync/using-condition-variables msdn�� winapi���� ���Ǻ���condition variable
//std::condition_variableŬ������ wait�� notify_one ���� Ÿ�� �ö󰡸� �ᱹ winapi SleepConditionVariableCS() WakeConditionVariable()�� ���´�.

//https://modoocode.com/285 �ι�° �ڵ� �м�.

class CThreadPool 
{
public:
	CThreadPool(size_t cnt): threadNum(cnt), stopAll(false)
	{
		workerThreads.reserve(threadNum);
		for (size_t i = 0; i < threadNum; i++) 
		{
			//emplace_back �� ���ڸ� �Ѱܼ� ��ü�� �����ڷ� �޾� ���Ϳ� ���Ҹ� �߰��ϴ� �Լ�.
			//std::thread([this](){this->WorkerThread();}) ���·� ������ ���� �� �������.
			workerThreads.emplace_back([this]() {this->WorkerThread(); });
		}
	};
	~CThreadPool() 
	{
		stopAll = true;
		cv.notify_all(); //wait�ߴ� �����带 ���� �����.
		for (auto& elements : workerThreads)
			elements.join(); //��������� �� �������� ������ ���.
	};
public:
	
	//������-�Һ��� �������� �����ڴ� �� �ϵ��� ť�� �߰��ϰ� �Һ��ڴ� ť�� ���� ������ ���� �ִٰ� ����� ���� ó���Ѵ�.
	void WorkerThread() 
	{
		while (true)
		{
			//EnqueueJob���� std::lock_guard�ʹ� �޸� std::unique_lock�� ���� ������ �⺻������ �������� ��� ��
			//unlock�Ǵ� �� ������ �߰��� lock unlock�� �߰��� �� �� �־.
			//�׸��� ���⼭ ���� �Ŵ� ������ wait�Լ��� ���ڷ� ���� �ʿ�� �ϱ� ����.
			std::unique_lock<std::mutex> lock(mt);
			//wait�Լ��� ���ڷ� �ѱ� ���ǿ� �ش��� �� ���� �����带 ����, unlock���¿� ����.
			//������ �����ϸ� ��ȯ�ϸ鼭 �ٽ� lock�� �Ǵ�.
			//�� ���� ��Ƴ��� ť�� ������� ������ ��⸦ �������;� �Ѵ�.
			cv.wait(lock, [this]() {return !this->jobs.empty() || stopAll; });
			if (stopAll && jobs.empty()) return;

			//�Լ������Ͷ� ���� ���������� �ٲٴ°� ū �ǹ̴� ���� ���δ�.
			std::function<void()> job = std::move(jobs.front());
			jobs.pop();
			lock.unlock();
			//ť���� ���� �Լ������� ����
			job();
		}
	};

	template<typename T, typename... Args>
	std::future<typename std::result_of<T(Args...)>::type> EnqueueJob(T pFunc, Args... args) 
	{
		//if (stopAll) return;

		//�Ѿ���� �Լ��� int work(int, int) intŸ�� ���ڸ� �� �� �޾� int�� return���ִ� �Լ��� ��.
		//std::result_of<T(Args...)>::type�� ���Ϲ��� int��. retType�� int���� ��Ÿ����.
		using retType = typename std::result_of<T(Args...)>::type;
		
		//shared_ptr�� packaged_task�� ��� ����Ǵ��� Ȯ�ο�
		//std::packaged_task<int(int)> task(some_task);
		//std::shared_ptr<A> p1(new A());
		//std::shared_ptr<A> p1 = std::make_shared<A>();
		

		//std::packaged_task<retType()> job(std::bind(pFunc, args...));
		//std::future<retType> jobProcFuture = job.get_future();
		//���� ���� ����job�� ���������̱� ������ future�� ���� promise�� set_value�� ���ֱ� ����(workthread���� job�� �����ϱ� ����)��
		//�̹� EnqueueJob�� �������� future��ü�� �ı��Ǿ��� ���̴�.
		//�̸� �ذ��ϱ� ���� shared_ptr�� �̿��ؼ� packaged_task�� ���Ǵ� ���� ��ü�� �ı����� �ʵ��� �Ѵ�.
		//std::packaged_task<retType()>>�� retType�� int�� �߷еǾ����� ()���ڰ� ���� intŸ���� ���ھ��� �Լ��� �޴´�.
		//std::shared_ptr<std::packaged_task<retType()>> job = new std::packaged_task<retType()>(std::bind(pFunc, args...));
		//Ǯ��� �̷� ���� �� ������ ���̸�, std::bind(pFunc, args...)�� ���� int Ÿ���� �����ϴ� ���ڰ� ���� callable�� �ǰ� �̸�
		//std::packaged_task<retType()>>�� intŸ���� ���ڰ� ���� ȣ���ϴ� ���¿� ��������.
		auto job = std::make_shared<std::packaged_task<retType()>>(std::bind(pFunc, args...));
		//future�� promise ����
		std::future<retType> jobProcFuture = job->get_future();
		
		//�������� ����� ���� �����Ǹ鼭 ���ܹ߻��ÿ��� �����ϰ� ���� Ǯ���ִ� ����� �Ѵ�.
		//�� ���� ť�� �ִ� �κб����� ���� �ɱ� ���� {}������. �ǹ̰� �ִ� �߰�ȣ��.
		{
			std::lock_guard<std::mutex> lock(mt);
			//packaged_task�� �����ϴ� ����. (*job)�ؼ� ���޹��� pFunc�Լ������͸� ��Ÿ���� �׸� �����ϴ� callable�� ť�� �߰�.
			//ť�� std::function<void()> Ÿ���̾��� ������ ���ٷ� �μ� Ÿ���� ������ ��.
			jobs.push([job]() {(*job)();});
		}
		//wait�� ������ �� �ϳ��� �����.
		cv.notify_one();

		return jobProcFuture;
	};

public:
	size_t threadNum;
	std::condition_variable cv; //���Ǻ���
	std::mutex mt; //���ؽ�
	std::vector<std::thread> workerThreads; //��Ŀ ������ ���� �ڷᱸ��
	std::queue<std::function<void()>> jobs; //�Լ����� �� �� ���� ������ ť
	bool stopAll; //������ ���� �÷���
};

//version1
//https://modoocode.com/285 ù��° �ڵ� �м�.
//����Ÿ���� �ִ� �Լ��� ������ ��� ���ϰ��� ���� �� ���� ����.
//voidŸ�� �Լ��� ť�� ���� �� �־� �ٸ� Ÿ���� ���ٷ� ���μ� �����ؾ߸� �ϴ� ����.
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
			//emplace_back �� ���ڸ� �Ѱܼ� ��ü�� �����ڷ� �޾� ���Ϳ� ���Ҹ� �߰��ϴ� �Լ�.
			//std::thread([this](){this->WorkerThread();}) ���·� ������ ���� �� �������.
			workerThreads.emplace_back([this]() {this->WorkerThread(); });
		}
	};
	~CThreadPool()
	{
		stopAll = true;
		cv.notify_all(); //wait�ߴ� �����带 ���� �����.
		for (auto& elements : workerThreads)
			elements.join(); //��������� �� �������� ������ ���.
	};
public:

	//������-�Һ��� �������� �����ڴ� �� �ϵ��� ť�� �߰��ϰ� �Һ��ڴ� ť�� ���� ������ ���� �ִٰ� ����� ���� ó���Ѵ�.
	void WorkerThread()
	{
		while (true)
		{
			//EnqueueJob���� std::lock_guard�ʹ� �޸� std::unique_lock�� ���� ������ �⺻������ �������� ��� ��
			//unlock�Ǵ� �� ������ �߰��� lock unlock�� �߰��� �� �� �־.
			//�׸��� ���⼭ ���� �Ŵ� ������ wait�Լ��� ���ڷ� ���� �ʿ�� �ϱ� ����.
			std::unique_lock<std::mutex> lock(mt);
			//wait�Լ��� ���ڷ� �ѱ� ���ǿ� �ش��� �� ���� �����带 ����, unlock���¿� ����.
			//������ �����ϸ� ��ȯ�ϸ鼭 �ٽ� lock�� �Ǵ�.
			//�� ���� ��Ƴ��� ť�� ������� ������ ��⸦ �������;� �Ѵ�.
			cv.wait(lock, [this]() {return !this->jobs.empty() || stopAll; });
			if (stopAll && jobs.empty()) return;

			//�Լ������Ͷ� ���� ���������� �ٲٴ°� ū �ǹ̴� ���� ���δ�.
			std::function<void()> job = std::move(jobs.front());
			jobs.pop();
			lock.unlock();
			//ť���� ���� �Լ������� ����
			job();
		}
	};
	void EnqueueJob(std::function<void()> job)
	{
		if (stopAll) return;
		//�������� ����� ���� �����Ǹ鼭 ���ܹ߻��ÿ��� �����ϰ� ���� Ǯ���ִ� ����� �Ѵ�.
		//�� ���� ť�� �ִ� �κб����� ���� �ɱ� ���� {}������. �ǹ̰� �ִ� �߰�ȣ��.
		{
			std::lock_guard<std::mutex> lock(mt);
			//�Լ� ������������ ���ٸ� ����������� �� �Ͼ���� �����״� ���������� �ٲ��ִ� move�� ū �ǹ̴� ���� ��.
			jobs.push(std::move(job));
		}
		//wait�� ������ �� �ϳ��� �����.
		cv.notify_one();
	};
public:
	size_t threadNum;
	std::condition_variable cv; //���Ǻ���
	std::mutex mt; //���ؽ�
	std::vector<std::thread> workerThreads; //��Ŀ ������ ���� �ڷᱸ��
	std::queue<std::function<void()>> jobs; //�Լ����� �� �� ���� ������ ť
	bool stopAll; //������ ���� �÷���
};
*/