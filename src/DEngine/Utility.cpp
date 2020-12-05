#include <DEngine/Std/Utility.hpp>

#include <random>

#include <string>

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

void Std::NameThisThread(std::string const& name)
{
#if defined(DENGINE_OS_WINDOWS)
	std::basic_string<wchar_t> tempString;
	tempString.reserve(name.size());
	for (auto character : name)
		tempString.push_back(character);

	HRESULT r = SetThreadDescription(
		GetCurrentThread(),
		tempString.data());
#elif defined(DENGINE_OS_LINUX) || defined(DENGINE_OS_ANDROID)
	pthread_setname_np(pthread_self(), name.data());
#endif
}
