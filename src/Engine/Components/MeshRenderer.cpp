#include "MeshRenderer.hpp"

#include <vector>
#include <string>
#include <cassert>

#include "../Renderer/Renderer.hpp"

#include "Math/LinearTransform3D.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::MeshRenderer::MeshRenderer(SceneObject& owningSceneObject, size_t indexInSceneObject, size_t indexInScene) : 
	ParentType(owningSceneObject, indexInSceneObject, indexInScene), 
	mesh(Asset::Mesh::None)
{
}

Engine::MeshRenderer::~MeshRenderer()
{
	if (GetMesh() != Asset::Mesh::None)
	{
		InvalidateRenderScene();
		Renderer::RemoveMeshReference(static_cast<size_t>(GetMesh()));
	}
}

Asset::Mesh Engine::MeshRenderer::GetMesh() const { return mesh; }

std::underlying_type_t<Asset::Mesh> Engine::MeshRenderer::GetMeshID() const { return static_cast<std::underlying_type_t<Asset::Mesh>>(GetMesh()); }

void Engine::MeshRenderer::SetMesh(Asset::Mesh newMesh)
{
	if (GetMesh() == newMesh)
		return;

	assert(Asset::CheckValid(newMesh));

	if (GetMesh() == Asset::Mesh::None && newMesh != Asset::Mesh::None)
		Renderer::AddMeshReference(static_cast<Renderer::AssetID>(newMesh));
	else if (GetMesh() != Asset::Mesh::None && newMesh != Asset::Mesh::None)
	{
		Renderer::RemoveMeshReference(static_cast<Renderer::AssetID>(GetMesh()));
		Renderer::AddMeshReference(static_cast<Renderer::AssetID>(newMesh));
	}
	else if (GetMesh() != Asset::Mesh::None && newMesh == Asset::Mesh::None)
		Renderer::RemoveMeshReference(static_cast<Renderer::AssetID>(GetMesh()));

	mesh = newMesh;

	InvalidateRenderScene();
}

Math::Matrix<4, 3> Engine::MeshRenderer::GetModel_Reduced(Space space) const
{
	const auto& scaleModel = Math::LinTran3D::Scale_Reduced(scale);
	const auto& rotateModel = Math::LinTran3D::Rotate_Reduced(rotation);
	auto localModel = Math::LinTran3D::Multiply(scaleModel, rotateModel);
	Math::LinTran3D::AddTranslation(localModel, position);

	if (space == Space::Local)
		return localModel;
	else
	{
		auto parentModel = GetSceneObject().transform.GetModel_Reduced(space);
		return Math::LinTran3D::Multiply(parentModel, localModel);
	}
}
