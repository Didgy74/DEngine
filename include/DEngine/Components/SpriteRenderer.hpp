#pragma once

#include "DEngine/Components/Components.hpp"

#include "DEngine/AssetManager/AssetManager.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"

#include "DEngine/Enum.hpp"

namespace Engine
{
	namespace Components
	{
		class SpriteRenderer : public ComponentBase
		{
		public:
			using ParentType = ComponentBase;

			explicit SpriteRenderer(SceneObject& owningObject);
			~SpriteRenderer();

			void SetSprite(AssMan::Sprite newTexture);
			[[nodiscard]] AssMan::Sprite GetSprite() const;

			Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;
			Math::Matrix4x4 GetModel(Space space) const;

			Math::Vector2D positionOffset{};
			float rotation{};
			Math::Vector2D scale{ 1, 1 };

		private:
			AssMan::Sprite sprite = AssMan::Sprite::None;
		};
	}
	
}