#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/impl/Assert.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/StackVec.hpp>
#include <DEngine/Std/Utility.hpp>

// Temp
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Physics.hpp>
#include <box2d/box2d.h>

namespace DEngine
{
	enum class Entity : u64 { Invalid = u64(-1) };

	class Scene;

	struct Move
	{
		void Update(Entity entity, Scene& scene, f32 deltaTime) const;
	};

	class Transform
	{
	public:
		Math::Vec3 position = {};
		f32 rotation = 0;
		Math::Vec2 scale = { 1.f, 1.f };
	};

	class Scene
	{
	public:
		Scene() = default;
		Scene(Scene&&) = default;
		Scene& operator=(Scene&&) = default;

		void Copy(Scene& output) const;

	private:

		std::vector<Entity>& Impl_GetEntities() { return entities; }

		template<typename T>
		std::vector<Std::Pair<Entity, T>>& Impl_GetAllComponents() = delete;
		template<typename T>
		std::vector<Std::Pair<Entity, T>> const& Impl_GetAllComponents() const = delete;

	public:
		// We need this to be a pointer to heap because the struct is so huge.
		// And lots of the objects in it require a pointer to it.
		Std::Box<b2World> physicsWorld;

		Std::Span<Entity const> GetEntities() const { return { entities.data(), entities.size() }; }

		template<typename T>
		Std::Span<Std::Pair<Entity, T>> GetAllComponents() = delete;
		template<typename T>
		Std::Span<Std::Pair<Entity, T> const> GetAllComponents() const = delete;

		void Begin();

		Entity NewEntity() noexcept;

		void DeleteEntity(Entity ent) noexcept;

		// Confirm that this entity exists.
		[[nodiscard]] bool ValidateEntity(Entity entity) const noexcept;

		template<typename T>
		void AddComponent(Entity entity, T const& component)
		{
			DENGINE_IMPL_ASSERT(ValidateEntity(entity));

			auto& componentVector = Impl_GetAllComponents<T>();
			// Crash if we already got this component
			DENGINE_IMPL_ASSERT(GetComponent<T>(entity) == nullptr);

			componentVector.push_back({ entity, component });
		}
		template<typename T>
		void DeleteComponent(Entity entity)
		{
			DENGINE_IMPL_ASSERT(ValidateEntity(entity));
			auto& componentVector = Impl_GetAllComponents<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			DENGINE_IMPL_ASSERT(componentIt != componentVector.end());
			componentVector.erase(componentIt);
		}
		template<typename T>
		void DeleteComponent_CanFail(Entity entity)
		{
			auto& componentVector = Impl_GetAllComponents<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			if (componentIt != componentVector.end())
				componentVector.erase(componentIt);
		}
		template<typename T>
		[[nodiscard]] T* GetComponent(Entity entity)
		{
			DENGINE_IMPL_ASSERT(ValidateEntity(entity));
			auto& componentVector = Impl_GetAllComponents<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			if (componentIt != componentVector.end())
				return &componentIt->b;
			else
				return nullptr;
		}
		template<typename T>
		[[nodiscard]] T const* GetComponent(Entity entity) const
		{
			DENGINE_IMPL_ASSERT(ValidateEntity(entity));
			auto& componentVector = Impl_GetAllComponents<T>();
			auto const componentIt = Std::FindIf(
				componentVector.begin(),
				componentVector.end(),
				[entity](auto const& val) -> bool { return entity == val.a; });
			if (componentIt != componentVector.end())
				return &componentIt->b;
			else
				return nullptr;
		}

	private:
		u64 entityIdIncrementor = 0;
		std::vector<Std::Pair<Entity, Transform>> transforms;
		std::vector<Std::Pair<Entity, Gfx::TextureID>> textureIDs;
		std::vector<Std::Pair<Entity, Move>> moves;
		std::vector<Std::Pair<Entity, Physics::Rigidbody2D>> rigidBodies;
		std::vector<Entity> entities;
	};

	template<>
	inline Std::Span<Std::Pair<Entity, Transform>> Scene::GetAllComponents<Transform>() { return { transforms.data(), transforms.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Transform> const> Scene::GetAllComponents<Transform>() const { return { transforms.data(), transforms.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Gfx::TextureID>> Scene::GetAllComponents<Gfx::TextureID>() { return { textureIDs.data(), textureIDs.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Gfx::TextureID> const> Scene::GetAllComponents<Gfx::TextureID>() const { return { textureIDs.data(), textureIDs.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Move>> Scene::GetAllComponents<Move>() { return { moves.data(), moves.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Move> const> Scene::GetAllComponents<Move>() const { return { moves.data(), moves.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Physics::Rigidbody2D>> Scene::GetAllComponents<Physics::Rigidbody2D>() { return { rigidBodies.data(), rigidBodies.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Physics::Rigidbody2D> const> Scene::GetAllComponents<Physics::Rigidbody2D>() const { return { rigidBodies.data(), rigidBodies.size() }; }

	template<>
	inline std::vector<Std::Pair<Entity, Transform>>& Scene::Impl_GetAllComponents<Transform>() { return transforms; }
	template<>
	inline std::vector<Std::Pair<Entity, Transform>> const& Scene::Impl_GetAllComponents<Transform>() const { return transforms; }
	template<>
	inline std::vector<Std::Pair<Entity, Gfx::TextureID>>& Scene::Impl_GetAllComponents<Gfx::TextureID>() { return textureIDs; }
	template<>
	inline std::vector<Std::Pair<Entity, Gfx::TextureID>> const& Scene::Impl_GetAllComponents<Gfx::TextureID>() const { return textureIDs; }
	template<>
	inline std::vector<Std::Pair<Entity, Move>>& Scene::Impl_GetAllComponents<Move>() { return moves; }
	template<>
	inline std::vector<Std::Pair<Entity, Move>> const& Scene::Impl_GetAllComponents<Move>() const { return moves; }
	template<>
	inline std::vector<Std::Pair<Entity, Physics::Rigidbody2D>>& Scene::Impl_GetAllComponents<Physics::Rigidbody2D>() { return rigidBodies; }
	template<>
	inline std::vector<Std::Pair<Entity, Physics::Rigidbody2D>> const& Scene::Impl_GetAllComponents<Physics::Rigidbody2D>() const { return rigidBodies; }
}

