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

			const std::vector<uint8_t>& GetByteArray() const;
			const uint8_t* GetData() const;
			size_t GetByteLength() const;

			const std::array<uint32_t, 3>& GetDimensions() const;


			Format GetBaseInternalFormat() const;
			Format GetInternalFormat() const;
			Type GetType() const;

			bool IsCompressed() const;

			static CreateInfo ToCreateInfo(TextureDocument&& texDoc);

		private:
			std::vector<uint8_t> byteArray;
			Format internalFormat{};
			Format baseInternalFormat{};
			Type type{};
			uint8_t numDimensions = 0;
			std::array<uint32_t, 3> dimensions{};
			size_t numLayers = 0;
			size_t numLevels = 0;
			bool isCompressed = false;
		};

		struct TextureDocument::CreateInfo
		{
			std::vector<uint8_t> byteArray;
			Format internalFormat{};
			Format baseInternalFormat{};
			Type type{};
			uint8_t numDimensions = 0;
			std::array<uint32_t, 3> dimensions{};
			size_t numLayers = 0;
			size_t numLevels = 0;
			bool isCompressed = false;
		};

		enum class TextureDocument::Format
		{
			Invalid,
			RGBA,
			RGB,
			R8G8B8A8,
			Compressed_RGB_S3TC_DXT1_ANGLE,
			Compressed_RGBA_S3TC_DXT1_ANGLE,
			Compressed_RGBA_S3TC_DXT3_ANGLE,
			Compressed_RGBA_S3TC_DXT5_ANGLE,
			Compressed_RGBA_BPTC_UNORM,
			Compressed_SRGB_ALPHA_BPTC_UNORM,
			Compressed_RGB_BPTC_SIGNED_FLOAT,
			Compressed_RGB_BPTC_UNSIGNED_FLOAT,
		};

		enum class TextureDocument::Type
		{
			Invalid,
			UnsignedByte,
		};
	}
}