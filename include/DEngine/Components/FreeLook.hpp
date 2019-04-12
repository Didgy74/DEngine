#pragma once

#include "DEngine/Components/ScriptBase.hpp"

namespace Engine
{
	namespace Components
	{
		class FreeLook : public ScriptBase
		{
		public:
			using ParentType = ScriptBase;

			explicit FreeLook(SceneObject& owningObject);

		protected:
			void SceneStart() override;

			void Tick() override;
		};
	}
}