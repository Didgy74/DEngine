#pragma once

namespace Math
{
	namespace detail
	{
		template<typename T>
		constexpr T e = T(2.7182818284590452353602874713527);

		template<typename T>
		constexpr T goldenRatio = T(1.6180339887498948482);

		template<typename T>
		constexpr T pi = T(3.14159265358979323846264338327950288419716939937510582097494459230781);

		template<typename T>
		constexpr T degToRad = T(pi<long double> / 180);
		template<typename T>
		constexpr T radToDeg = T(180 / pi<long double>);
	}

	constexpr auto e = detail::e<float>;

	constexpr auto goldenRatio = detail::goldenRatio<float>;

	constexpr auto pi = detail::pi<float>;
	constexpr auto degToRad = detail::degToRad<float>;
	constexpr auto radToDeg = detail::radToDeg<float>;
}