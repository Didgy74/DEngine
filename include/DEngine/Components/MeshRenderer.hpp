#pragma once

#include "DEngine/Components/Components.hpp"

#include "DEngine/AssetManager/AssetManager.hpp"

#include "DEngine/Enum.hpp"

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

			[[nodiscard]] size_t GetMesh() const;
			void SetMesh(size_t newMesh);

			[[nodiscard]] Math::Matrix<4, 3> GetModel_Reduced(Space space) const;
			Math::Matrix4x4 GetModel(Space space) const;

			Math::Vector<3, float> positionOffset{};
			Math::UnitQuaternion<float> rotation{};
			Math::Vector<3, float> scale{};

			size_t mesh{};
			size_t texture{};
		};
	}
}
