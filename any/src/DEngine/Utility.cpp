#include <DEngine/Std/Utility.hpp>

#include <random>

#include <cstring>

// For Std::NameThisThread
#include <DEngine/Application.hpp>
#if defined(DENGINE_OS_WINDOWS)
#	include <windows.h>
#	include <processthreadsapi.h>
#elif defined(DENGINE_OS_LINUX) || defined(DENGINE_OS_ANDROID)
#	include <pthread.h>
#endif


using namespace DEngine;
using namespace DEngine::Std;

template<>
f32 Std::Rand<f32>()
{
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<f32> dis(0.f, 1.f);

	return dis(gen);
}

template<>
u64 Std::RandRange(u64 a, u64 b)
{
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<u64> dis(a, b);

	return dis(gen);
}

template<>
f32 Std::RandRange(f32 a, f32 b)
{
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<f32> dis(a, b);

	return dis(gen);
}

void Std::NameThisThread(Str name)
{
	// Maximum size set by POSIX.
	// This includes the null-terminator.
	[[maybe_unused]] constexpr u32 maxThreadNameLength = 16;
	
	DENGINE_IMPL_ASSERT(name.Size() < maxThreadNameLength);

#if defined(DENGINE_OS_WINDOWS)
	wchar_t tempString[maxThreadNameLength] = {};
	for (u8 i = 0; i < (u8)name.Size(); i += 1)
		tempString[i] = name[i];

	[[maybe_unused]] HRESULT r = SetThreadDescription(
		GetCurrentThread(),
		tempString);
#elif defined(DENGINE_OS_LINUX) || defined(DENGINE_OS_ANDROID)
	char tempString[maxThreadNameLength] = {};
	for (u8 i = 0; i < (u8)name.Size(); i += 1)
		tempString[i] = name[i];

	pthread_setname_np(pthread_self(), tempString);
#endif
}
