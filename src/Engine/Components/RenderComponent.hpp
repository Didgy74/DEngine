#pragma once

#include "ComponentBase.hpp"

namespace Engine
{
	class Scene;

	class RenderComponent : public ComponentBase
	{
	public:
		using ParentType = ComponentBase;

		RenderComponent(SceneObject& owningObject, size_t indexInObject, size_t indexInScene) noexcept;

		~RenderComponent();

	protected:
		void InvalidateRenderScene();
	};
}