#pragma once

#include "Components.hpp"

#include "../AssetManager/AssetManager.hpp"

#include "../Enum.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/UnitQuaternion.hpp"
#include "DMath/Matrix/Matrix.hpp"

namespace Engine
{
	namespace Components
	{
		class MeshRenderer : public ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			explicit MeshRenderer(SceneObject& owningObject);
			~MeshRenderer();

			[[nodiscard]] AssMan::Mesh GetMesh() const;
			std::underlying_type_t<AssMan::Mesh> GetMeshID() const;
			void SetMesh(AssMan::Mesh newMesh);

			[[nodiscard]] Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
			Math::Matrix4x4 GetModel(Space space) const;

			Math::Vector3D positionOffset;
			Math::UnitQuaternion<> rotation;
			Math::Vector3D scale;

		private:
			AssMan::Mesh mesh;
		};
	}
}
