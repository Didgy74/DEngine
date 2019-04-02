#pragma once

#include "TextureDocument.hpp"

#include <string>
#include <optional>

#include "ktx.h"

namespace Engine
{
	namespace AssetManager
	{
		std::optional<TextureDocument> LoadTextureDocument(std::string path)
		{
			if (path == "")
				return {};

			ktxTexture* texture;
			KTX_error_code result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);



			return {};
		}
	}
}