#pragma once

namespace Utility
{
	struct Color
	{
		static constexpr unsigned char length = 4;

		float r;
		float g;
		float y;
		float x;

		static constexpr Color Black();
		static constexpr Color White();
		static constexpr Color Grey();
		static constexpr Color Red();
		static constexpr Color Green();
		static constexpr Color Blue();
	};

	constexpr Color Color::Black() { return { 0, 0, 0, 1 }; }

	constexpr Color Color::White() { return { 1, 1, 1, 1 }; }

	constexpr Color Color::Grey() { return { 0.5f, 0.5f, 0.5f, 1 }; }

	constexpr Color Color::Red() { return { 1, 0, 0, 1 }; }

	constexpr Color Color::Green() { return { 0, 1, 0, 1 }; }

	constexpr Color Color::Blue() { return { 0, 0, 1, 1 }; }
}