#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Pair.hpp"
#include "DEngine/Containers/StaticVector.hpp"

// Temp
#include "DEngine/Math/Vector.hpp"
#include "DEngine/Math/UnitQuaternion.hpp"
#include "DEngine/Gfx/Gfx.hpp"
#include "DEngine/Physics.hpp"

namespace DEngine
{
	enum class Entity : u64;

	class Scene;

	struct Move
	{
		void Update(Entity entity, Scene& scene, f32 deltaTime) const;
	};

	class Transform
	{
	public:
		Math::Vector<3, f32> position{};
		Math::UnitQuaternion<f32> rotation{};
		Math::Vector<3, f32> scale{};
	};

	class Scene
	{
	public:
		Scene() = default;

		Std::StaticVector<Entity, 10> entities;
		Std::StaticVector<Std::Pair<Entity, Transform>, 10> transforms;
		Std::StaticVector<Std::Pair<Entity, Gfx::TextureID>, 10> textureIDs;
		Std::StaticVector<Std::Pair<Entity, Physics::Rigidbody2D>, 10> rigidbodies;
		Std::StaticVector<Std::Pair<Entity, Physics::CircleCollider2D>, 10> circleColliders;
		Std::StaticVector<Std::Pair<Entity, Physics::BoxCollider2D>, 10> boxColliders;
		Std::StaticVector<Std::Pair<Entity, Move>, 10> moves;

		inline Entity NewEntity()
		{
			entities.PushBack((Entity)entityIdIncrementor);
			entityIdIncrementor += 1;

			return entities[entities.Size() - 1];
		}

		inline void DeleteEntity(Entity ent)
		{
			for (uSize i = 0; i < entities.Size(); i += 1)
			{
				if (entities[i] == ent)
				{
					entities.EraseUnsorted(i);
					break;
				}
			}

			for (uSize i = 0; i < transforms.Size(); i += 1)
			{
				if (transforms[i].a == ent)
				{
					transforms.EraseUnsorted(i);
					break;
				}
			}

			for (uSize i = 0; i < textureIDs.Size(); i += 1)
			{
				if (textureIDs[i].a == ent)
				{
					textureIDs.EraseUnsorted(i);
					break;
				}
			}

			for (uSize i = 0; i < rigidbodies.Size(); i += 1)
			{
				if (rigidbodies[i].a == ent)
				{
					rigidbodies.EraseUnsorted(i);
					break;
				}
			}

			for (uSize i = 0; i < circleColliders.Size(); i += 1)
			{
				if (circleColliders[i].a == ent)
				{
					circleColliders.EraseUnsorted(i);
					break;
				}
			}

			for (uSize i = 0; i < moves.Size(); i += 1)
			{
				if (moves[i].a == ent)
				{
					moves.EraseUnsorted(i);
					break;
				}
			}
		}

	private:
		u64 entityIdIncrementor = 0;
	};
}

