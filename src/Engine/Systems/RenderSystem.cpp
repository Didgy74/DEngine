#include "RenderSystem.hpp"

#include "../Renderer/Renderer.hpp"

namespace Engine
{
	void RenderSystem::BuildRenderGraph(const Scene& scene, Renderer::RenderGraph& graph, Renderer::RenderGraphTransform& transforms)
	{
		auto spriteComponentsPtr = scene.GetAllComponents<Components::SpriteRenderer>();
		if (spriteComponentsPtr == nullptr)
		{
			graph.sprites.clear();
			transforms.sprites.clear();
		}
		else
		{
			const auto& spriteComponents = *spriteComponentsPtr;
			graph.sprites.resize(spriteComponents.size());
			transforms.sprites.resize(spriteComponents.size());
			for (size_t i = 0; i < spriteComponents.size(); i++)
			{
				graph.sprites[i] = static_cast<Renderer::SpriteID>(spriteComponents[i].GetSprite());
				transforms.sprites[i] = spriteComponents[i].GetModel(Space::World);
			}
		}

		auto meshComponentsPtr = scene.GetAllComponents<Components::MeshRenderer>();
		if (meshComponentsPtr == nullptr)
		{
			graph.meshes.clear();
			transforms.meshes.clear();
		}
		else
		{
			const auto& meshComponents = *meshComponentsPtr;
			graph.meshes.resize(meshComponents.size());
			transforms.meshes.resize(meshComponents.size());
			for (size_t i = 0; i < meshComponents.size(); i++)
			{
				graph.meshes[i] = static_cast<Renderer::MeshID>(meshComponents[i].GetMesh());
				transforms.meshes[i] = meshComponents[i].GetModel(Space::World);
			}
		}
	}
}

