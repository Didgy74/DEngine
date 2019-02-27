#pragma once

#include "RenderComponent.hpp"

#include "../Asset.hpp"

#include "../Enum.hpp"

#include "Math/Vector/Vector.hpp"
#include "Math/UnitQuaternion.hpp"
#include "Math/Matrix/Matrix.hpp"

namespace Engine
{
	class MeshRenderer : public RenderComponent
	{
	public:
		using ParentType = RenderComponent;

		MeshRenderer(SceneObject& owningObject, size_t indexInSceneObject, size_t indexInScene);
		~MeshRenderer();

		[[nodiscard]] Asset::Mesh GetMesh() const;
		std::underlying_type_t<Asset::Mesh> GetMeshID() const;
		void SetMesh(Asset::Mesh newMesh);

		Math::Matrix<4, 3> GetModel_Reduced(Space space) const;

		Math::Vector3D position;
		Math::UnitQuaternion<> rotation;
		Math::Vector3D scale;


	private:
		Asset::Mesh mesh;
	};
}
