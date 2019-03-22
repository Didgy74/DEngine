#include "MeshRenderer.hpp"

#include <vector>
#include <string>
#include <cassert>

#include "../Renderer/Renderer.hpp"

#include "DMath/LinearTransform3D.hpp"

#include "../Scene.hpp"
#include "../SceneObject.hpp"

namespace Engine
{
	namespace Components
	{
		MeshRenderer::MeshRenderer(SceneObject& owningSceneObject) :
			ParentType(owningSceneObject),
			mesh(Asset::Mesh::None),
			positionOffset{ 0, 0, 0 },
			scale{ 1, 1, 1 },
			rotation()
		{
		}

		MeshRenderer::~MeshRenderer()
		{
		}

		Asset::Mesh MeshRenderer::GetMesh() const { return mesh; }

		std::underlying_type_t<Asset::Mesh> MeshRenderer::GetMeshID() const { return static_cast<std::underlying_type_t<Asset::Mesh>>(GetMesh()); }

		void MeshRenderer::SetMesh(Asset::Mesh newMesh)
		{
			if (GetMesh() == newMesh)
				return;

			assert(Asset::CheckValid(newMesh));

			mesh = newMesh;
		}

		Math::Matrix<4, 3> MeshRenderer::GetModel_Reduced(Space space) const
		{
			using namespace Math::LinTran3D;
			Math::Matrix<4, 3> localModel = Multiply(Scale_Reduced(scale), Rotate_Reduced(rotation));
			AddTranslation(localModel, positionOffset);

			if (space == Space::Local)
				return localModel;
			else
				return Multiply(GetSceneObject().transform.GetModel_Reduced(Space::World), localModel);
		}

		Math::Matrix4x4 MeshRenderer::GetModel(Space space) const
		{
			const auto& model = GetModel_Reduced(space);
			return Math::LinTran3D::AsMat4(model);
		}
	}
}




