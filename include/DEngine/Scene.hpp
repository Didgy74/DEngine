#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>

// Temp
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Physics.hpp>

namespace DEngine
{
	enum class Entity : u64;
	constexpr Entity invalidEntity = Entity(-1);

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

		std::vector<Entity> entities;
		std::vector<Std::Pair<Entity, Transform>> transforms;
		std::vector<Std::Pair<Entity, Gfx::TextureID>> textureIDs;
		std::vector<Std::Pair<Entity, Physics::Rigidbody2D>> rigidbodies;
		std::vector<Std::Pair<Entity, Physics::CircleCollider2D>> circleColliders;
		std::vector<Std::Pair<Entity, Physics::BoxCollider2D>> boxColliders;
		std::vector<Std::Pair<Entity, Move>> moves;

		inline Entity NewEntity()
		{
			entities.push_back((Entity)entityIdIncrementor);
			entityIdIncrementor += 1;

			return entities[entities.size() - 1];
		}

		inline void DeleteEntity(Entity ent)
		{
			for (uSize i = 0; i < entities.size(); i += 1)
			{
				if (entities[i] == ent)
				{
					entities.erase(entities.begin() + i);
					break;
				}
			}

			for (uSize i = 0; i < transforms.size(); i += 1)
			{
				if (transforms[i].a == ent)
				{
					transforms.erase(transforms.begin() + i);
					break;
				}
			}

			for (uSize i = 0; i < textureIDs.size(); i += 1)
			{
				if (textureIDs[i].a == ent)
				{
					textureIDs.erase(textureIDs.begin() + i);
					break;
				}
			}

			for (uSize i = 0; i < rigidbodies.size(); i += 1)
			{
				if (rigidbodies[i].a == ent)
				{
					rigidbodies.erase(rigidbodies.begin() + 1);
					break;
				}
			}

			for (uSize i = 0; i < circleColliders.size(); i += 1)
			{
				if (circleColliders[i].a == ent)
				{
					circleColliders.erase(circleColliders.begin() + i);
					break;
				}
			}

			for (uSize i = 0; i < moves.size(); i += 1)
			{
				if (moves[i].a == ent)
				{
					moves.erase(moves.begin() + i);
					break;
				}
			}
		}

	private:
		u64 entityIdIncrementor = 0;
	};
}

