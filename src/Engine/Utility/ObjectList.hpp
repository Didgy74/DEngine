#pragma once

#include <vector>

namespace Utility
{
	template<typename T>
	class ObjectList
	{
	public:
		ObjectList(std::vector<T*>&& input) noexcept :
			ref(input)
		{}

		std::vector<T*>& ref;
		
		T& operator[](size_t index) { return *ref[index]; }
		const T& operator[](size_t index) const { return *ref[index]; }
	};
}