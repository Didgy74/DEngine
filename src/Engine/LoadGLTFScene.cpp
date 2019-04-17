#include "LoadGLTFScene.hpp"

#include "DEngine/Scene.hpp"
#include "DEngine/SceneObject.hpp"
#include "DEngine/Components/MeshRenderer.hpp"

#include "fx/gltf.h"

#include "DEngine/AssetManager/AssetManager.hpp"

namespace Engine
{
	bool LoadGLTFScene(Scene& scene, std::filesystem::path path)
	{
		auto gltfDoc = fx::gltf::LoadFromText(path.string());

		auto& sceneObj = scene.NewSceneObject();

		sceneObj.localScale = { 0.1f, 0.1f, 0.1f };

		for (size_t meshIndex = 0; meshIndex < gltfDoc.meshes.size(); meshIndex++)
		{
			const auto& mesh = gltfDoc.meshes[meshIndex];
			
			for (size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); primitiveIndex++)
			{
				const auto& primitive = mesh.primitives[primitiveIndex];

				auto& compRef = sceneObj.AddComponent<Components::MeshRenderer>().second.get();

				AssetManager::MeshInfo meshInfo{};
				meshInfo.path = path.string();
				meshInfo.meshIndex = meshIndex;
				meshInfo.primitiveIndex = primitiveIndex;

				size_t meshID = AssetManager::AddMesh(std::move(meshInfo));
				compRef.SetMesh(meshID);

				size_t baseColorTexIndex = gltfDoc.materials[primitive.material].pbrMetallicRoughness.baseColorTexture.index;
				AssetManager::TextureInfo textureInfo{};
				textureInfo.path = path.parent_path().string() + '/' + gltfDoc.images[baseColorTexIndex].uri;
				
				size_t textureID = AssetManager::AddTexture(std::move(textureInfo));
				compRef.texture = textureID;
			}
		}

		return true;
	}
}