#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <string>
#include <tchar.h>
#include <windows.h>

//char* to wchar*
inline void Char2Wchar(wchar_t* dst, size_t size, char* src, size_t count) 
{
	if(size > count)
		mbstowcs_s(NULL, dst, size, src, count);

	//다른 방법. mbstowcs, MultiByteToWideChar 등이 있음.
	/*int len = strlen(src);
	wchar_t* wsz = new wchar_t[len + 1];
	swprintf_s(wsz, len + 1, L"%hs", src);*/
}
inline wchar_t* Char2Wchar(char* src, size_t count)
{
	wchar_t* wsz = new wchar_t[count + 1];
	ZeroMemory(wsz, (count + 1) * sizeof(wchar_t));
	mbstowcs_s(NULL, wsz, count + 1, src, count);
	return wsz;
} 
//wchar_t* to char*
inline void Wchar2char(char* dst, size_t size, wchar_t* src, size_t count)
{
	if (size > count)
		wcstombs_s(NULL, dst, size, src, count);
}
inline char* Wchar2char(wchar_t* src, size_t count)
{
	char* sz = new char[count + 1];
	ZeroMemory(sz, (count + 1) * sizeof(char));
	wcstombs_s(NULL, sz, count + 1, src, count);
	return sz;
}

//string to wstring
inline void Str2Wstr(std::wstring& dst, std::string& src)
{
	dst.assign(src.begin(), src.end());
}

inline std::wstring Str2Wstr(std::string src)
{
	std::wstring dst;
	dst.assign(src.begin(), src.end());
	return dst;
}

//wstring to string
inline void Wstr2Str(std::string& dst, std::wstring& src)
{
	dst.assign(src.begin(), src.end());
}

inline std::string Wstr2Str(std::wstring src)
{
	std::string dst;
	dst.assign(src.begin(), src.end());
	return dst;
}

//string to char
inline void Str2Char(char* dst, std::string& src)
{
	//const 제거용 캐스팅
	memcpy(dst, (char*)src.c_str(), src.length());
}

inline char* Str2Char(std::string src)
{
	int len = src.length();
	char* dst = new char[len + 1];
	ZeroMemory(dst, len + 1);
	memcpy(dst, (char*)src.c_str(), len);
	return dst;
}

//wstring to wchar
inline void WStr2Wchar(wchar_t* dst, std::wstring& src)
{
	memcpy(dst, (wchar_t*)src.c_str(), src.length() * sizeof(wchar_t));
}

inline wchar_t* WStr2Wchar(std::wstring src)
{
	int len = src.length();
	wchar_t* dst = new wchar_t[len + 1];
	ZeroMemory(dst, (len + 1) * sizeof(wchar_t));
	memcpy(dst, (wchar_t*)src.c_str(), len * sizeof(wchar_t));
	return dst;
}

//char to string
inline void Char2Str(std::string& dst, char* src)
{
	dst = src;
}

inline std::string Char2Str(char* src)
{
	std::string dst = src;
	return dst;
}

//wchar to wstring
inline void Wchar2Wstr(std::wstring& dst, wchar_t* src)
{
	dst = src;
}

inline std::wstring Wchar2Wstr(wchar_t* src)
{
	std::wstring dst = src;
	return dst;
}

//string to wchar
inline void Str2Wchar(wchar_t* dst, std::string& src)
{
	std::wstring ws = Str2Wstr(src);
	WStr2Wchar(dst, ws);
}

inline wchar_t* Str2Wchar(std::string src)
{
	std::wstring ws = Str2Wstr(src);
	wchar_t* dst = WStr2Wchar(ws);
	return dst;
}

//wstring to char
inline void Wstr2Char(char* dst, std::wstring& src)
{
	std::string str = Wstr2Str(src);
	Str2Char(dst, str);
}

inline char* Wstr2Char(std::wstring src)
{
	std::string str = Wstr2Str(src);
	char* dst = Str2Char(str);
	return dst;
}
//char to wstring
inline void Char2Wstr(std::wstring& dst, char* src)
{
	int len = strlen(src);
	wchar_t* wsz = new wchar_t[len + 1];
	ZeroMemory(wsz, (len + 1) * sizeof(wchar_t));
	Char2Wchar(wsz, len + 1, src, len);
	Wchar2Wstr(dst, wsz);
	delete[] wsz;
}

inline std::wstring Char2Wstr(char* src)
{
	int len = strlen(src);
	wchar_t* wsz = new wchar_t[len + 1];
	Char2Wchar(wsz, len + 1, src, len);
	std::wstring dst = Wchar2Wstr(wsz);
	delete[] wsz;
	return dst;
}
//wchar to string
inline void Wchar2Str(std::string& dst, wchar_t* src)
{
	int len = wcslen(src);
	char* sz = new char[len + 1];
	Wchar2char(sz, len + 1, src, len);
	dst = Char2Str(sz);
	delete[] sz;
}

inline std::string Wchar2Str(wchar_t* src)
{
	int len = wcslen(src);
	char* sz = new char[len + 1];
	Wchar2char(sz, len + 1, src, len);
	std::string dst = Char2Str(sz);
	delete[] sz;
	return dst;
}

//cch : count of character 버퍼의 요소 개수. 
inline void Utf8toUnicode(wchar_t* dst, size_t cbDstSize, char* src, size_t cbSrcSize)
{
	//utf8 length
	int len = MultiByteToWideChar(CP_UTF8, 0, src, cbSrcSize, NULL, NULL);

	//복사하려는 버퍼의 사이즈가 작으면 MultiByteToWideChar에서 터지기 전에 리턴
	int cchWideChar = cbDstSize / sizeof(wchar_t);
	if (cchWideChar < len)
		return;
	
	MultiByteToWideChar(CP_UTF8, 0, src, cbSrcSize, dst, len);
}

inline void UnicodetoUtf8(char* dst, size_t cbDstSize, wchar_t* src, size_t cbSrcSize)
{
	int len = WideCharToMultiByte(CP_UTF8, 0, src, cbSrcSize, NULL, NULL, NULL, NULL);
	
	int cchChar = cbDstSize / sizeof(char);
	if (cchChar < len)
		return;

	WideCharToMultiByte(CP_UTF8, 0, src, cbSrcSize, dst, len, NULL, NULL);
}


//사이즈를 인자로 안받고 포인터에서부터 버퍼 사이즈를 얻어내려고 했는데 이상한 짓 하지 말자.
/*
template<typename T>
void Utf8toUnicoded(T& dst, char* src)
{
	return;
}
template<>
void Utf8toUnicoded(wchar_t*& dst, char* src) 
{
	int len = MultiByteToWideChar(CP_UTF8, 0, src, strlen(src), NULL, NULL);

	int bbb = sizeof(dst);
	auto pType = dst;
	int typesize = sizeof(std::remove_pointer<decltype(pType)>::type);
	int buffsize = 0;
	if (std::is_array<wchar_t*>::value)
	{
		//decltype(dst) : wchar_t[20]&
		//sizeof(decltype(dst)) : 20; 배열타입이라 타입의 사이즈가 제대로 안나온다.
		//typename std::remove_pointer<T>::type test;

		buffsize = sizeof(dst) / typesize;
	}
	else
		buffsize = _msize(dst) / typesize;

	MultiByteToWideChar(CP_UTF8, 0, src, strlen(src), dst, len);
}
//template<typename T, int N>
//void Utf8toUnicoded(T (&dst)[N],  char* src)
//{	
//	auto pType = dst;
//	int typesize = sizeof(std::remove_pointer<decltype(pType)>::type);
//	int buffsize = 0;
//	buffsize = sizeof(dst) / typesize;
//
//}
template<int N>
void Utf8toUnicoded(wchar_t(&dst)[N], char* src) 
{
	int b = 0;
}
*/