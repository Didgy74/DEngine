#include <DEngine/Std/Containers/Str.hpp>

#include <cstring>

using namespace DEngine;
using namespace DEngine::Std;

Str::Str(char const* in) noexcept
{
	ptr = in;
	length = strlen(in);
}