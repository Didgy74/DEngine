#pragma once

#include "TextureDocument.hpp"

#include <string>
#include <optional>

#include "ktx.h"

namespace Engine
{
	namespace AssetManager
	{
		TextureDocument::Format ConvertGLFormat(uint32_t glFormat);
		TextureDocument::Type ConvertGLType(uint32_t glType);

		TextureDocument::TextureDocument(CreateInfo&& info) :
			byteArray(std::move(info.byteArray)),
			baseInternalFormat(info.baseInternalFormat),
			internalFormat(info.internalFormat),
			type(info.type),
			numDimensions(info.numDimensions),
			dimensions(info.dimensions),
			numLayers(info.numLayers),
			numLevels(info.numLevels),
			isCompressed(info.isCompressed)
		{

		}

		TextureDocument::~TextureDocument()
		{
			
		}

		const std::vector<uint8_t>& TextureDocument::GetByteArray() const
		{
			return byteArray;
		}

		const uint8_t* TextureDocument::GetData() const
		{
			return byteArray.data();
		}

		size_t TextureDocument::GetByteLength() const
		{
			return byteArray.size();
		}

		const std::array<uint32_t, 3>& TextureDocument::GetDimensions() const
		{
			return dimensions;
		}

		bool TextureDocument::IsCompressed() const
		{
			return isCompressed;
		}

		TextureDocument::Format TextureDocument::GetBaseInternalFormat() const
		{
			return baseInternalFormat;
		}

		TextureDocument::Format TextureDocument::GetInternalFormat() const
		{
			return internalFormat;
		}

		TextureDocument::Type TextureDocument::GetType() const
		{
			return type;
		}

		TextureDocument::CreateInfo TextureDocument::ToCreateInfo(TextureDocument&& texDoc)
		{
			CreateInfo info{};

			info.byteArray = std::move(texDoc.byteArray);
			info.isCompressed = texDoc.isCompressed;
			info.baseInternalFormat = texDoc.baseInternalFormat;
			info.internalFormat = texDoc.internalFormat;
			info.type = texDoc.type;
			info.numDimensions = texDoc.numDimensions;
			info.dimensions = texDoc.dimensions;
			info.numLayers = texDoc.numLayers;
			info.numLevels = texDoc.numLevels;

			return info;
		}

		std::optional<TextureDocument> LoadTextureDocument(std::string path)
		{
			if (path == "")
				return {};

			ktxTexture* texture = nullptr;
			KTX_error_code result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &texture);
			if (result != KTX_SUCCESS)
				return  {};

			TextureDocument::CreateInfo createInfo{};

			createInfo.byteArray.resize(texture->dataSize);

			result = ktxTexture_LoadImageData(texture, createInfo.byteArray.data(), createInfo.byteArray.size());

			createInfo.numDimensions = texture->numDimensions;
			createInfo.dimensions = { texture->baseWidth, texture->baseHeight, texture->baseDepth };

			createInfo.numLayers = texture->numLayers;
			createInfo.numLevels = texture->numLevels;

			createInfo.isCompressed = texture->isCompressed;
			createInfo.type = ConvertGLType(texture->glType);
			createInfo.baseInternalFormat = ConvertGLFormat(texture->glBaseInternalformat);
			createInfo.internalFormat = ConvertGLFormat(texture->glInternalformat);

			ktxTexture_Destroy(texture);

			return std::optional<TextureDocument>( std::move(TextureDocument(std::move(createInfo))) );
		}
	}
}

namespace Engine
{
	namespace AssetManager
	{
		TextureDocument::Type ConvertGLType(uint32_t glType)
		{
			using Type = TextureDocument::Type;

			constexpr auto GL_UNSIGNED_BYTE = 0x1401;

			switch (glType)
			{
			case GL_UNSIGNED_BYTE:
				return Type::UnsignedByte;
			default:
				return Type::Invalid;
			}
		}

		TextureDocument::Format ConvertGLFormat(uint32_t glFormat)
		{
			using Format = TextureDocument::Format;

			// Uncompressed
			constexpr auto GL_RGB = 0x1907;
			constexpr auto GL_RGBA = 0x1908;
			constexpr auto GL_RGBA8 = 0x8058;
			switch (glFormat)
			{
			case GL_RGB:
				return Format::RGB;
			case GL_RGBA:
				return Format::RGBA;
			case GL_RGBA8:
				return Format::R8G8B8A8;
			};

			// DXT
			constexpr auto GL_COMPRESSED_RGB_S3TC_DXT1_ANGLE = 0x83F0;
			constexpr auto GL_COMPRESSED_RGBA_S3TC_DXT1_ANGLE = 0x83F1;
			constexpr auto GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE = 0x83F2;
			constexpr auto GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE = 0x83F3;
			switch (glFormat)
			{
			case GL_COMPRESSED_RGB_S3TC_DXT1_ANGLE:
				return Format::Compressed_RGB_S3TC_DXT1_ANGLE;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_ANGLE:
				return Format::Compressed_RGBA_S3TC_DXT1_ANGLE;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
				return Format::Compressed_RGBA_S3TC_DXT3_ANGLE;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
				return Format::Compressed_RGBA_S3TC_DXT5_ANGLE;
			}

			// BPTC
			constexpr auto GL_COMPRESSED_RGBA_BPTC_UNORM = 0x8E8C;
			constexpr auto GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM = 0x8E8D;
			constexpr auto GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT = 0x8E8E;
			constexpr auto GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = 0x8E8F;
			switch (glFormat)
			{
			case GL_COMPRESSED_RGBA_BPTC_UNORM:
				return Format::Compressed_RGBA_BPTC_UNORM;
			case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
				return Format::Compressed_SRGB_ALPHA_BPTC_UNORM;
			case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
				return Format::Compressed_RGB_BPTC_SIGNED_FLOAT;
			case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
				return Format::Compressed_RGB_BPTC_UNSIGNED_FLOAT;
			}

			return Format::Invalid;
		}
	}
}