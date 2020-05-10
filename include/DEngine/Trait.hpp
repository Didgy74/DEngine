#pragma once

namespace DEngine::Std::detail
{
	template<bool expr, typename T, typename U>
	struct Conditional;
	template<typename T, typename U>
	struct Conditional<true, T, U> { using Type = T; };
	template<typename T, typename U>
	struct Conditional<false, T, U> { using Type = U; };
}

namespace DEngine::Std
{
	template<bool expr, typename T, typename U>
	using Conditional = typename detail::Conditional<expr, T, U>::Type;
}