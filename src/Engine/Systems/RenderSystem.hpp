#pragma once

#include "System.hpp"

#include "../Renderer/Typedefs.hpp"

#include "../Scene.hpp"

// Forward declarations
namespace Engine
{
	class Scene;

	namespace Components
	{
		class MeshRenderer;
		class SpriteRenderer;
		class PointLight;
	}
}

namespace Engine
{
	using RenderSystem = System
	<
		Components::MeshRenderer, 
		Components::SpriteRenderer,
		Components::PointLight
	>;

	template<>
	class System
	<
		Components::MeshRenderer, 
		Components::SpriteRenderer,
		Components::PointLight
	>
	{
	public:
		static void BuildRenderGraph(const Scene& scene, Renderer::RenderGraph& graph);
		static void BuildRenderGraphTransform(const Scene& scene, Renderer::RenderGraphTransform& transforms);
	};
}