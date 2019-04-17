#pragma once

#include "Typedefs.hpp"

#include <string>
#include <string_view>
#include <optional>

#include "DRenderer/MeshDocument.hpp"

namespace Engine
{
	namespace AssetManager
	{
		struct MeshInfo
		{
			std::string path;
			size_t meshIndex;
			size_t primitiveIndex;

			bool operator==(const MeshInfo& right) const;
		};

		struct TextureInfo
		{
			std::string path;

			bool operator==(const TextureInfo& right) const;
		};

		constexpr std::string_view textureFolderPath = "Data/Textures/";
		constexpr std::string_view meshFolderPath = "Data/Meshes/";

		MeshInfo GetMeshInfo(size_t i);
		size_t AddMesh(MeshInfo&& meshInfo);
		TextureInfo GetTextureInfo(size_t i);
		size_t AddTexture(TextureInfo&& textureInfo);
		
		std::optional<Renderer::MeshDoc> GetRendererMeshDoc(size_t id);
		void Renderer_AssetLoadEndEvent();

		namespace Core
		{
			void Initialize();
			void Terminate();
		}
	}

	namespace AssMan = AssetManager;
}