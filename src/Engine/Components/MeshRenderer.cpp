#include "MeshRenderer.hpp"

#include <vector>
#include <string>
#include <cassert>

#include "../Renderer/Renderer.hpp"

#include "Math/LinearTransform3D.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

Engine::MeshRenderer::MeshRenderer(SceneObject& owningSceneObject) :
	ParentType(owningSceneObject),
	mesh(Asset::Mesh::None),
	position{ 0, 0, 0},
	scale{1, 1, 1},
	rotation()
{
}

Engine::MeshRenderer::~MeshRenderer()
{
}

Asset::Mesh Engine::MeshRenderer::GetMesh() const { return mesh; }

std::underlying_type_t<Asset::Mesh> Engine::MeshRenderer::GetMeshID() const { return static_cast<std::underlying_type_t<Asset::Mesh>>(GetMesh()); }

void Engine::MeshRenderer::SetMesh(Asset::Mesh newMesh)
{
	if (GetMesh() == newMesh)
		return;

	assert(Asset::CheckValid(newMesh));

	mesh = newMesh;
}

Math::Matrix4x4 Engine::MeshRenderer::GetModel(Space space) const
{
	using namespace Math::LinTran3D;
	auto localModel = Multiply(Scale_Reduced(scale), Rotate_Reduced(rotation));
	AddTranslation(localModel, position);

	if (space == Space::Local)
		return AsMat4(localModel);
	else
		return AsMat4(Multiply(GetSceneObject().transform.GetModel_Reduced(Space::World), localModel));
}


