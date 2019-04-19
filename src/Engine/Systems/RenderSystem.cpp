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
		// Copies all the MeshID and TextureID data (no positional data) from the components into the RenderGraph
		auto meshComponentsPtr = scene.GetAllComponents<Components::MeshRenderer>();
		if (meshComponentsPtr == nullptr)
			graph.meshes.clear();
		else
		{
			const auto& meshComponents = *meshComponentsPtr;
			graph.meshes.resize(meshComponents.size());
			for (size_t i = 0; i < meshComponents.size(); i++)
			{
				const auto& meshComponent = meshComponents[i];

				Renderer::MeshID meshID;
				meshID.meshID = meshComponent.mesh;
				meshID.diffuseID = meshComponent.texture;

				graph.meshes[i] = meshID;
			}
		}

		// Loads all PointLight data (no positional data) onto the RenderGraph struct.
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
				graph.pointLightIntensities[i] = { color.x, color.y, color.z };
			}
		}
	}

	void RenderSystem::BuildRenderGraphTransform(const Scene& scene, Renderer::RenderGraphTransform& transforms)
	{
		// Copies all the Mesh-components' positional data onto the RenderGraphTransform struct.
		// This copies a full 4x4 matrix, it's faster because everything has
		// to be converted to that format to move up the parent SceneObject chain anyways
		auto meshComponentsPtr = scene.GetAllComponents<Components::MeshRenderer>();
		if (meshComponentsPtr == nullptr)
			transforms.meshes.clear();
		else
		{
			const auto& meshComponents = *meshComponentsPtr;
			transforms.meshes.resize(meshComponents.size());
			for (size_t i = 0; i < meshComponents.size(); i++)
				transforms.meshes[i] = meshComponents[i].GetModel(Space::World).data;
		}

		// Loads all the PointLight components' positional data over to the RenderGraphTransform
		auto pointLightComponentsPtr = scene.GetAllComponents<Components::PointLight>();
		if (pointLightComponentsPtr == nullptr)
			transforms.pointLights.clear();
		else
		{
			const auto& pointLightComponents = *pointLightComponentsPtr;
			transforms.pointLights.resize(pointLightComponents.size());
			for (size_t i = 0; i < pointLightComponents.size(); i++)
			{
				const auto& position = pointLightComponents[i].GetPosition(Space::World);
				transforms.pointLights[i] = { position.x, position.y, position.z };
			}
		}
	}
}

