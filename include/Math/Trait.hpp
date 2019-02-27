#pragma once

#include <limits>
#include <type_traits>
#include <cstdint>

namespace Math
{
	namespace detail
	{
		namespace Trait
		{
			template<size_t n, typename = void>
			struct SmallestUInt {};

			template<size_t n>
			struct SmallestUInt<n, std::enable_if_t<0 <= n && n <= std::numeric_limits<uint8_t>::max()>>
			{
				using Type = uint8_t;
			};

			template<size_t n>
			struct SmallestUInt<n, std::enable_if_t<std::numeric_limits<uint8_t>::max() < n && n <= std::numeric_limits<uint16_t>::max()>>
			{
				using Type = uint16_t;
			};

			template<size_t n>
			struct SmallestUInt<n, std::enable_if_t<std::numeric_limits<uint16_t>::max() < n && n <= std::numeric_limits<uint32_t>::max()>>
			{
				using Type = uint32_t;
			};

			template<size_t n>
			struct SmallestUInt<n, std::enable_if_t<std::numeric_limits<uint32_t>::max() < n && n <= std::numeric_limits<uint64_t>::max()>>
			{
				using Type = uint64_t;
			};
		}
	}

	namespace Trait
	{
		template<size_t n>
		using SmallestUInt = typename detail::Trait::SmallestUInt<n>::Type;
	}
}