#include "RenderComponent.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::RenderComponent::RenderComponent(SceneObject& owningObject, size_t indexInObject, size_t indexInScene) noexcept :
	ParentType(owningObject, indexInObject, indexInScene)
{
}

Engine::RenderComponent::~RenderComponent()
{

}

void Engine::RenderComponent::InvalidateRenderScene()
{
	GetSceneObject().GetScene().InvalidateRenderScene();
}