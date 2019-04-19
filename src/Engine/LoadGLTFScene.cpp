#include "LoadGLTFScene.hpp"

#include "DEngine/Scene.hpp"
#include "DEngine/SceneObject.hpp"
#include "DEngine/Components/MeshRenderer.hpp"

#include "fx/gltf.h"

#include "DEngine/AssetManager/AssetManager.hpp"

namespace Engine
{
	bool LoadGLTFScene(SceneObject& obj, std::filesystem::path path)
	{
		auto gltfDoc = fx::gltf::LoadFromText(path.string());

		Scene& scene = obj.GetScene();

		// Generate one sceneObject for all our primitives to be attached to.
		auto& sceneObj = scene.NewSceneObject();
		sceneObj.SetParent(&obj);

		// Software currently only supports one node with several primitives attached.
		// Found no sample files to test with multiple nodes.
		// Multiple nodes would mean multiple SceneObjects.

		sceneObj.localScale = { gltfDoc.nodes[0].scale[0], gltfDoc.nodes[0].scale[1], gltfDoc.nodes[0].scale[2], };

		// Grabs the GLTF mesh node 0 uses.
		const auto& mesh = gltfDoc.meshes[gltfDoc.nodes[0].mesh];
			
		// Iterates through the primitives in this mesh and adds it to the AssetManager, then adds it as a component on the SceneObject
		for (size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); primitiveIndex++)
		{
			const auto& primitive = mesh.primitives[primitiveIndex];

			auto& compRef = sceneObj.AddComponent<Components::MeshRenderer>().second.get();

			AssetManager::MeshInfo meshInfo{};
			meshInfo.path = path.string();
			meshInfo.meshIndex = gltfDoc.nodes[0].mesh;
			meshInfo.primitiveIndex = primitiveIndex;

			size_t meshID = AssetManager::AddMesh(std::move(meshInfo));
			compRef.SetMesh(meshID);

			// Adds the BaseColor (Diffuse) texture associated with this primitive, to the AssetManager
			// and then references it in the MeshRenderer component.
			size_t baseColorTexIndex = gltfDoc.materials[primitive.material].pbrMetallicRoughness.baseColorTexture.index;
			AssetManager::TextureInfo textureInfo{};
			textureInfo.path = path.parent_path().string() + '/' + gltfDoc.images[baseColorTexIndex].uri;

			size_t textureID = AssetManager::AddTexture(std::move(textureInfo));
			compRef.texture = textureID;
		}

		return true;
	}
}