#pragma once

namespace DEngine::Application
{
	enum class OS
	{
		Windows,
		Linux,
		Android
	};

	enum class Platform
	{
		Desktop,
		Mobile
	};

#if defined(_WIN32) || defined(_WIN64)
	constexpr OS targetOS = OS::Windows;
	constexpr Platform targetOSType = Platform::Desktop;
#elif defined(__ANDROID__)
	constexpr OS targetOS = OS::Android;
	constexpr Platform targetOSType = Platform::Mobile;
#elif defined(__GNUC__)
	constexpr OS targetOS = OS::Linux;
	constexpr Platform targetOSType = Platform::Desktop;
#else
#error Error. DEngine::Application does not support this platform/compiler
#endif

	void Log(char const* msg);
}

namespace DEngine
{
	namespace App = Application;
}