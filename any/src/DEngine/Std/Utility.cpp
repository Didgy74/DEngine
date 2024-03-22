#include <DEngine/Std/Utility.hpp>

#include <random>

#include <cstring>

// For Std::NameThisThread
#include <DEngine/impl/Assert.hpp>

#include <DEngine/Std/Defines.hpp>

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

#if DENGINE_STD_COMPILER == DENGINE_STD_COMPILER_GCC_VALUE
#include <pthread.h>
#elif DENGINE_STD_COMPILER == DENGINE_STD_COMPILER_MSVC_VALUE
#include <Windows.h>
#include <processthreadsapi.h>
#endif

void Std::NameThisThread(Span<char const> name)
{
	// Maximum size set by POSIX.
	// This includes the null-terminator.
	[[maybe_unused]] constexpr u32 maxThreadNameLength = 16;

	// This accounts for null-terminator
	DENGINE_IMPL_ASSERT(name.Size() < maxThreadNameLength - 1);

#if DENGINE_STD_COMPILER == DENGINE_STD_COMPILER_GCC_VALUE
	char tempString[maxThreadNameLength] = {};
	for (u8 i = 0; i < (u8)name.Size(); i += 1)
		tempString[i] = name[i];
	tempString[maxThreadNameLength - 1] = 0;

	pthread_setname_np(pthread_self(), tempString);

#elif DENGINE_STD_COMPILER == DENGINE_STD_COMPILER_MSVC_VALUE
	wchar_t tempString[maxThreadNameLength] = {};
	for (u8 i = 0; i < (u8)name.Size(); i += 1)
		tempString[i] = name[i];
	tempString[maxThreadNameLength - 1] = 0;

	[[maybe_unused]] HRESULT r = SetThreadDescription(
		GetCurrentThread(),
		tempString);
#else
#error "Unsupported"
#endif
}
