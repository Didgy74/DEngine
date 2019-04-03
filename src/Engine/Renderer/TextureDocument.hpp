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
			enum class Type;
			struct CreateInfo;

			TextureDocument() = delete;
			explicit TextureDocument(CreateInfo&&);
			explicit TextureDocument(TextureDocument&&) = default;
			TextureDocument(const TextureDocument&) = delete;
			~TextureDocument();

			const std::array<uint32_t, 3>& GetDimensions() const;

			const uint8_t* TextureDocument::GetData() const;
			Format TextureDocument::GetBaseInternalFormat() const;
			Type TextureDocument::GetType() const;

			static CreateInfo ToCreateInfo(TextureDocument&& texDoc);

		private:
			std::vector<uint8_t> byteArray;
			Format baseInternalFormat{};
			Type type{};
			uint8_t numDimensions = 0;
			std::array<uint32_t, 3> dimensions{};
			size_t numLayers = 0;
			size_t numLevels = 0;
		};

		std::optional<TextureDocument> LoadTextureDocument(std::string path);

		struct TextureDocument::CreateInfo
		{
			std::vector<uint8_t> byteArray;
			Format baseInternalFormat{};
			Type type{};
			uint8_t numDimensions = 0;
			std::array<uint32_t, 3> dimensions{};
			size_t numLayers = 0;
			size_t numLevels = 0;
		};

		enum class TextureDocument::Format
		{
			Invalid,
			RGBA,
			RGB,
			R8G8B8A8,

		};

		enum class TextureDocument::Type
		{
			Invalid,
			UnsignedByte,
		};
	}
}