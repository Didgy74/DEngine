#pragma once

#include "Components.hpp"

#include "../Asset.hpp"

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

			[[nodiscard]] Asset::Mesh GetMesh() const;
			std::underlying_type_t<Asset::Mesh> GetMeshID() const;
			void SetMesh(Asset::Mesh newMesh);

			[[nodiscard]] Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
			Math::Matrix4x4 GetModel(Space space) const;

			Math::Vector3D positionOffset;
			Math::UnitQuaternion<> rotation;
			Math::Vector3D scale;

		private:
			Asset::Mesh mesh;
		};
	}
}
