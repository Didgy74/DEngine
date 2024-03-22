#include <DEngine/Std/Allocator.hpp>

using namespace DEngine;
using namespace DEngine::Std;

void* Std::DefaultAllocator::Alloc(uSize size, uSize alignment) noexcept
{
	return {};
}

bool Std::DefaultAllocator::Realloc(void* ptr, uSize newSize) noexcept
{
	return {};
}

void Std::DefaultAllocator::Free(void* in) noexcept
{

}