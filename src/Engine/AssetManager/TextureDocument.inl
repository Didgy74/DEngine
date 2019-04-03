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
			type(info.type),
			numDimensions(info.numDimensions),
			dimensions(info.dimensions),
			numLayers(info.numLayers),
			numLevels(info.numLevels)
		{

		}

		TextureDocument::~TextureDocument()
		{
			
		}

		const std::array<uint32_t, 3>& TextureDocument::GetDimensions() const
		{
			return dimensions;
		}

		const uint8_t* TextureDocument::GetData() const
		{
			return byteArray.data();
		}

		TextureDocument::Format TextureDocument::GetBaseInternalFormat() const
		{
			return baseInternalFormat;
		}

		TextureDocument::Type TextureDocument::GetType() const
		{
			return type;
		}

		TextureDocument::CreateInfo TextureDocument::ToCreateInfo(TextureDocument&& texDoc)
		{
			CreateInfo info{};

			info.byteArray = std::move(texDoc.byteArray);
			info.baseInternalFormat = texDoc.baseInternalFormat;
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

			createInfo.type = ConvertGLType(texture->glType);
			createInfo.baseInternalFormat = ConvertGLFormat(texture->glBaseInternalformat);

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
			default:
				return Format::Invalid;
			}
		}
	}
}