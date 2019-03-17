#pragma once

#include "System.hpp"

#include "../Components/MeshRenderer.hpp"
#include "../Components/SpriteRenderer.hpp"
#include "../Renderer/Typedefs.hpp"

#include "../Scene.hpp"

namespace Engine
{
	using RenderSystem = System<Components::MeshRenderer, Components::SpriteRenderer>;

	template<>
	class System<Components::MeshRenderer, Components::SpriteRenderer>
	{
	public:
		static void BuildRenderGraph(const Scene& scene, Renderer::RenderGraph& graph, Renderer::RenderGraphTransform& transforms);
	};
}