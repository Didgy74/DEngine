#pragma once

#include "TextureDocument.hpp"

namespace Engine
{
	namespace Renderer
	{
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
	}
}