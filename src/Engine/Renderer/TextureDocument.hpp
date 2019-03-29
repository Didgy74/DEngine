#pragma once

#include <array>
#include <vector>
#include <cstddef>

namespace Engine
{
	namespace Renderer
	{
		class TextureDocument
		{
		public:
			enum class Format;

		private:
			Format format;
			std::array<uint32_t, 2> dimensions;
			std::array<uint8_t> byteArray;
		};

		enum class TextureDocument::Format
		{
			RGBA
		};
	}
}