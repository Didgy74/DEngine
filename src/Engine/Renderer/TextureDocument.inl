#pragma once

#include "TextureDocument.hpp"

namespace Engine
{
	namespace Renderer
	{
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
	}
}