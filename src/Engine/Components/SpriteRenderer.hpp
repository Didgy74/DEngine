#pragma once

#include "Components.hpp"

#include "../Asset.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"

#include "../Enum.hpp"

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

			void SetSprite(Asset::Sprite newTexture);
			[[nodiscard]] Asset::Sprite GetSprite() const;

			Math::Matrix<3, 2> GetModel2D_Reduced(Space space) const;
			Math::Matrix4x4 GetModel(Space space) const;

			Math::Vector2D position;
			float rotation;
			Math::Vector2D scale;

		private:
			Asset::Sprite sprite;
		};
	}
	
}