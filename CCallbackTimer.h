#pragma once
#include <iostream>
#include <thread>
#include <functional>
#include <chrono>

class CCallbackTimer
{
public:
	CCallbackTimer():bIsExcute(false)
	{};
	~CCallbackTimer() 
	{
		if (bIsExcute)	Stop();
	};
public:
	//T&& 는 우측값 레퍼런스만 받는게 아님.
	//forward reference  라고 하여 우측값이 오면 우측값 전달 좌측값이 오면 좌측값 전달된다.
	//T&& callback 은 람다로 넘어올 경우 우측값
	//l/r value인지 확실히 전달하기 위한 perfect forward를 위한 전달 참조와 std::forward.
	//forward reference를 통해 함수의 인자로 전달 받은 레퍼런스는 이름이 있고 주소가 있음으로
	//lvalue reference든 rvalue reference는 그 자체는 lvalue이다. 
	//이 lvalue를 lvalue lvalue reference인지 rvalue reference인지 리턴해주는 것이 std::forward.
	template<typename T, typename... Types>
	void Start(unsigned int period, T&& callback, Types&&... args)
	{
		//auto task = [callback, args...]() {callback(args...); };
		//args...캡쳐를 &args로 레퍼런스로 해야 std::forward에서 레퍼런스를 추론할 수 있는 것 같다.
		auto task = [callback, &args...](){callback(std::forward<Types>(args)...); };
		bIsExcute = true;
		workThread = std::thread(
			[this, period, task]()
			{
				while (true) 
				{
					//그냥 true일 때로 했을 경우 this 객체가 사라져 쓰레기 값이 들어와도
					//0이 아니라 true로 인식해버린다. 1이 아닐 때 루프를 탈출해서 스레드가 끝나도록 하자.
					if (this->bIsExcute != 1) break;
					std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point nextTime = startTime + std::chrono::milliseconds(period);
					task();
					std::this_thread::sleep_until(nextTime);
					//std::cout << this << std::endl;
				}
			}
		);
		workThread.detach();
	}
	void Stop() 
	{
		bIsExcute = false;
	}
public:
	bool bIsExcute;
	std::thread workThread;
};

//example
//version 1
//callable을 std::function으로 명시하고 밖에서 호출할 때
//std::bind로 인자와 콜백함수를 뭉쳐서 인자 없이 넘겨받아서 이용
/*
void Start(unsigned int period, std::function<void()> func)
{
	bIsExcute = true;
	workThread = std::thread(
		[this, period, func]()
		{
			while (this->bIsExcute)
			{
				std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
				std::chrono::system_clock::time_point nextTime = startTime + std::chrono::milliseconds(period);
				func();
				std::this_thread::sleep_until(nextTime);
			}
		}
	);
	workThread.detach();
}
*/
/*
void add(int x, int y)
{
	std::cout << x << " + " << y << " = " << x + y << std::endl;
}
*/
/*
int main() 
{
	std::function<void(int, int)> pFADD1 = std::bind(add, 2, 3);
	pFADD1(3, 5);
	//bind된 인자에 따라서 3+5가 아니고 2+3 나옴.
	//이 경우 인자를 맞춰서 pFADD(int,int)로 호출해줘야 하는데
	//callbacktimer start(int, std::function<void()>)에서 호출하기 위해서
	//auto로 선언해서 bind하고 pFADD2() 처럼 인자를 없애서 호출해서 쓰는게 좋겠다.
	auto pFADD2 = std::bind(add, 2, 3);
	pFADD2();
	
	CCallbackTimer calltimer;
	calltimer.Start(1000, pFADD2);
}
*/
//version 2
//가변인자 템플릿으로 콜백과 인자를 받아서 함수 안에서 바인드
/*
template<typename T, typename... Types>
void Start(unsigned int period, T callback, Types... args)
{
	auto task = std::bind(callback, args...);
	bIsExcute = true;
	workThread = std::thread(
		[this, period, task]()
		{
			while (this->bIsExcute)
			{
				std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
				std::chrono::system_clock::time_point nextTime = startTime + std::chrono::milliseconds(period);
				//func();
				task();
				std::this_thread::sleep_until(nextTime);
			}
		}
	);
	workThread.detach();
}
*/
/*
void add(int x, int y)
{
	std::cout << x << " + " << y << " = " << x + y << std::endl;
}
*/
/*
int main()
{
	std::function<void(int, int)> pFADD3 = add;
    CCallbackTimer calltimer;
	//가변인자로 넘겨서 Start함수 안에서 pFADD3 callable에 인자 바인드.
    calltimer.Start(1000, pFADD3, 1,2);
}
*/
//version3 (now)
//std::bind는 가독성 및 오버로딩(인자가 다른 동명의 함수)호출 시의 선택 문제(함수 포인터로 오버로딩된 함수중 무엇인가 넘겨줘야 한다고 함)등으로
//람다를 이용할 것을 권장한다고 하여 람다로 가변인자를 처리할 수 있도록.
//다른 가변인자를 받는 예제에서 보면 T&&를 우측값 레퍼런스 식으로 받던데 이건 의미를 잘 모르겠다.(조사할 것!)
//T&&는 우측값 레퍼런스가 아닌 전달 레퍼런스Forwarding Reference 또는 Universal Reference
//lvalue는 lvalue, rvalue는 rvalue, const lvalue는 const lvalue, const rvalue는 const rvalue로 추론해준다.
//전달되는 값이 lvalue reference인지 rvalue reference인지 const l/r value reference인지
//타입을 제대로 전달하기 위한 것이 perfect forwarding이라고 하고 그를 위해 전달 레퍼런스와 std::forward를 이용
/*
template<typename T, typename... Types>
	void Start(unsigned int period, T&& callback, Types&&... args)
	{
		//auto task = [callback, args...]() {callback(args...); };
		//args...캡쳐를 &args로 해야 std::forward에서 레퍼런스를 지우고 lvalue로 만들 수 있는 것 같다.
		auto task = [callback, &args...](){callback(std::forward<Types>(args)...); };
		bIsExcute = true;
		workThread = std::thread(
			[this, period, task]()
			{
				while (true)
				{
					//그냥 true일 때로 했을 경우 this 객체가 사라져 쓰레기 값이 들어와도
					//0이 아니라 true로 인식해버린다. 1이 아닐 때 루프를 탈출해서 스레드가 끝나도록 하자.
					if (this->bIsExcute != 1) break;
					std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point nextTime = startTime + std::chrono::milliseconds(period);
					task();
					std::this_thread::sleep_until(nextTime);
					//std::cout << this << std::endl;
				}
			}
		);
		workThread.detach();
	}
*/
/*
CCallbackTimer calltimer;
calltimer.Start(1000, [](int x, int y) {std::cout << x << " + " << y << " = " << x + y << std::endl; }, 4,5);
*/