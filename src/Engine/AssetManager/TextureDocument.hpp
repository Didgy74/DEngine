#pragma once

#include <array>
#include <vector>
#include <optional>

namespace Engine
{
	namespace AssetManager
	{
		class TextureDocument
		{
		public:
			enum class Format;

		private:
			Format format;
			std::array<uint32_t, 2> dimensions;
			std::vector<uint8_t> byteArray;
		};

		enum class TextureDocument::Format
		{
			RGBA
		};

		std::optional<TextureDocument> LoadTextureDocument(std::string path);
	}

	namespace AssMan = AssetManager;
}