#include <DEngine/impl/Assert.hpp>

#include <iostream>

#ifdef _MSC_VER
#   include <intrin.h>
#	define DENGINE_IMPL_BREAKPOINT() __debugbreak()
#endif

namespace DEngine::impl
{
	[[noreturn]] void Assert(
		char const* conditionString,
		const char* file,
		unsigned long long line, 
		const char* msg) noexcept
	{
		std::cout << std::endl;
		std::cout << "#################" << std::endl << std::endl;
		std::cout << "Assertion failure" << std::endl;
		std::cout << "#################" << std::endl << std::endl;
		if (conditionString)
			std::cout << "Expression: " << conditionString << std::endl;
		std::cout << "File: " << file << std::endl;
		std::cout << "Line: " << line << std::endl;
		if (msg != nullptr)
			std::cout << "Info: '" << msg << "'" << std::endl;
		std::cout << std::endl;

#ifdef DENGINE_IMPL_BREAKPOINT
		//DENGINE_IMPL_BREAKPOINT();
#endif

		std::abort();
	}
}