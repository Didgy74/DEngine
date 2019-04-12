#include "RenderSystem.hpp"

#include "DRenderer/Renderer.hpp"

#include "DEngine/Components/SpriteRenderer.hpp"
#include "DEngine/Components/MeshRenderer.hpp"
#include "DEngine/Components/PointLight.hpp"

#include "DMath/LinearTransform3D.hpp"

namespace Engine
{
	void RenderSystem::BuildRenderGraph(const Scene& scene, Renderer::RenderGraph& graph)
	{
		auto spriteComponentsPtr = scene.GetAllComponents<Components::SpriteRenderer>();
		if (spriteComponentsPtr == nullptr)
			graph.sprites.clear();
		else
		{
			const auto& spriteComponents = *spriteComponentsPtr;
			graph.sprites.resize(spriteComponents.size());
			for (size_t i = 0; i < spriteComponents.size(); i++)
				graph.sprites[i] = Renderer::SpriteID(spriteComponents[i].GetSprite());
		}

		auto meshComponentsPtr = scene.GetAllComponents<Components::MeshRenderer>();
		if (meshComponentsPtr == nullptr)
			graph.meshes.clear();
		else
		{
			const auto& meshComponents = *meshComponentsPtr;
			graph.meshes.resize(meshComponents.size());
			for (size_t i = 0; i < meshComponents.size(); i++)
				graph.meshes[i] = Renderer::MeshID(meshComponents[i].GetMesh());
		}

		auto pointLightComponentsPtr = scene.GetAllComponents<Components::PointLight>();
		if (pointLightComponentsPtr == nullptr)
			graph.pointLightIntensities.clear();
		else
		{
			const auto& pointLightComponents = *pointLightComponentsPtr;
			graph.pointLightIntensities.resize(pointLightComponents.size());
			for (size_t i = 0; i < pointLightComponents.size(); i++)
			{
				Math::Vector3D color = pointLightComponents[i].color;
				for (auto& element : color)
					element = Math::Clamp(element, 0.f, 1.f) * pointLightComponents[i].intensity;
				graph.pointLightIntensities[i] = color;
			}
		}
	}

	void RenderSystem::BuildRenderGraphTransform(const Scene& scene, Renderer::RenderGraphTransform& transforms)
	{
		auto spriteComponentsPtr = scene.GetAllComponents<Components::SpriteRenderer>();
		if (spriteComponentsPtr == nullptr)
			transforms.sprites.clear();
		else
		{
			const auto& spriteComponents = *spriteComponentsPtr;
			transforms.sprites.resize(spriteComponents.size());
			for (size_t i = 0; i < spriteComponents.size(); i++)
				transforms.sprites[i] = spriteComponents[i].GetModel(Space::World);
		}

		auto meshComponentsPtr = scene.GetAllComponents<Components::MeshRenderer>();
		if (meshComponentsPtr == nullptr)
			transforms.meshes.clear();
		else
		{
			const auto& meshComponents = *meshComponentsPtr;
			transforms.meshes.resize(meshComponents.size());
			for (size_t i = 0; i < meshComponents.size(); i++)
				transforms.meshes[i] = meshComponents[i].GetModel(Space::World);
		}

		auto pointLightComponentsPtr = scene.GetAllComponents<Components::PointLight>();
		if (pointLightComponentsPtr == nullptr)
			transforms.pointLights.clear();
		else
		{
			const auto& pointLightComponents = *pointLightComponentsPtr;
			transforms.pointLights.resize(pointLightComponents.size());
			for (size_t i = 0; i < pointLightComponents.size(); i++)
			{
				const auto& model = pointLightComponents[i].GetModel_Reduced(Space::World);
				const Math::Vector3D& position = Math::LinTran3D::GetTranslation(model);
				transforms.pointLights[i] = position;
			}
		}
	}
}

