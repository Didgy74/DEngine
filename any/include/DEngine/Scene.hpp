#pragma once

#include <DEngine/impl/Assert.hpp>
#include <DEngine/Std/Containers/Pair.hpp>
#include <DEngine/Std/Containers/FnScratchList.hpp>

// Temp
#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Physics.hpp>
#include <box2d/box2d.h>

#include <functional>

namespace DEngine {
	enum class Entity : u64 { Invalid = u64(-1) };

	class Scene;

	namespace Component {
		struct GuiPlane {
			enum class ConstructorId : u64 {};
			// Should be serialized
			ConstructorId constructorId;

			Std::Box<Gui::Widget> widget;
			struct FocusWidget {
				Gui::Widget* widget = nullptr;
				Std::Opt<u64> secondaryIndex = Std::nullOpt;
			};
			Std::Opt<FocusWidget> focusWidget = Std::nullOpt;

			struct FrontmostLayer {
				Std::Opt<FocusWidget> focusWidget = Std::nullOpt;
				Math::Vec2Int positionOffset = {};
				Gui::Extent extent = {};
				Std::Box<Gui::Widget> widget;
			};
			Std::Opt<FrontmostLayer> frontmostLayer;

			struct HeldPointer {
				u8 id;
				Math::Vec2 position;
			};
			Std::StackVec<HeldPointer, 20> heldPointerIds;

			struct GuiTextInputSession : public Gui::Widget::Widget_TextInputSession {
				Platform::Context* m_platformCtx = nullptr;
				// When we are destroyed, we must make sure the scene knows there is no
				// current text input session.
				Scene* m_scene = nullptr;
				~GuiTextInputSession() override;
			};

			struct GuiWindowHandler : public Gui::Widget::WidgetEvent_WindowHandler {

				GuiPlane* m_guiPlane = nullptr;
				bool* m_removeCurrentLayer = nullptr;

				// This is necessary to forward into Widget_TextInputSession objects
				Scene* m_scene = nullptr;
				Platform::Context* m_platformCtx = nullptr;

				virtual bool SetFrontmostLayer(
					Std::Box<Gui::Widget>&&,
					Math::Vec2Int,
					Gui::Extent,
					Std::Opt<Gui::Widget::Widget_FocusWidgetResult> const&) override;
				virtual void QueueRemoveCurrentLayer() override;

				// Note: Important to remember that our GuiWindowHandler object is short-lived,
				// while this TextInputSession is long-lived!
				virtual Std::Box<Gui::Widget::Widget_TextInputSession> TakeTextInputConnection(
					Gui::Widget& widget,
					Gui::TextInputType textInputFilter,
					Std::Span<char const> currentText,
					u64 selStart,
					u64 selCount) override;
			};
		};
	}

	struct Move {
		void Update(
			Platform::Context& appCtx,
			Entity entity,
			Scene& scene,
			f32 deltaTime) const;
	};

	class Transform {
	public:
		Math::Vec3 position = {};
		Math::UnitQuat rotation;
		Math::Vec2 scale = { 1.f, 1.f };
	};

	class PlatformToSceneEventForwarder : public Platform::EventForwarder {
	public:
		explicit PlatformToSceneEventForwarder(Scene& scene, Gui::TextEngine& textEngine) {
			m_scene = &scene;
			m_textEngine = &textEngine;
		}

		[[nodiscard]] Scene& GetScene() const {
			DENGINE_IMPL_ASSERT(m_scene != nullptr);
			return *m_scene;
		}
		Scene* m_scene = nullptr;

		[[nodiscard]] Gui::TextEngine& GetTextEngine() const {
			DENGINE_IMPL_ASSERT(m_textEngine != nullptr);
			return *m_textEngine;
		}
		// TODO: I don't quite like this.
		Gui::TextEngine* m_textEngine = nullptr;

		void ButtonEvent(
			Platform::Context& appCtx,
			Platform::WindowID windowId,
			Platform::Button button,
			bool state) override;

		void TextInputEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count,
			Std::Span<u32 const> newText) override;

		void TextSelectionEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count) override;

		void EndTextInputSessionEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId) override;

		void GamepadAxisEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			Platform::GamepadDeviceId deviceId,
			Platform::GamepadAxis axis,
			f32 value) override;

		void GamepadKeyEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			Platform::GamepadDeviceId deviceId,
			Platform::GamepadKey key,
			bool pressed) override;
	};

	class Scene {
	public:
		Scene() = default;
		Scene(Scene&&) = default;
		Scene& operator=(Scene&&) = default;

		void Copy(Scene& output) const;

		using CreateGuiPlaneWidgetFnT = Std::Box<Gui::Widget>(Component::GuiPlane::ConstructorId);
		std::function<CreateGuiPlaneWidgetFnT> m_createGuiPlaneWidgetFn;

		struct GuiTheme {
			static constexpr auto defaultTextMargin = Gui::Distance::Mm(2.5, true);
			static constexpr auto defaultCornerRadius = Gui::Distance::Mm(10, true);
			static constexpr auto defaultSpacing = Gui::Distance::Mm(2.5, true);
			static constexpr auto defaultScrollbarPadding = Gui::Distance::Px(50, true);

			Gui::Distance textMargin = defaultTextMargin;
			Gui::Distance cornerRadius = defaultCornerRadius;
			Gui::Distance spacing = defaultSpacing;
			Gui::Distance scrollbarPadding = defaultScrollbarPadding;
		};
		GuiTheme m_guiTheme;
		[[nodiscard]] GuiTheme const& GetGuiTheme() const { return m_guiTheme; }

		// Persists across ticks so the nav-active guard survives forwarder reconstruction.
		// TODO: Should be moved into a system at some point
		struct JoystickNavState {
			f32 leftX = 0.f;
			f32 leftY = 0.f;
			bool navActive = false;
		};
		JoystickNavState m_joystickNavState;

		struct GuiPlaneState {
			// Might make sense to store which Widget and which GuiPlane
			// is holding this
			// text input session.
			Component::GuiPlane::GuiTextInputSession* activeTextInputSession = nullptr;
		};
		GuiPlaneState m_guiPlaneState;

		// TODO: Should be moved into a system at some point
		static constexpr u32 guiPlaneVerticalResolution = 1000;
		static constexpr f32 guiPlaneDpi = 72.f;
		static constexpr f32 guiPlaneContentScale = 1.f;
		static constexpr f32 guiPlaneTextScale = 5.f;

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
		void AddComponent(Entity entity, T&& component)
		{
			DENGINE_IMPL_ASSERT(ValidateEntity(entity));

			auto& componentVector = Impl_GetAllComponents<Std::Trait::RemoveCVRef<T>>();
			// Crash if we already got this component
			DENGINE_IMPL_ASSERT(GetComponent<Std::Trait::RemoveCVRef<T>>(entity) == nullptr);

			componentVector.push_back(Std::Pair{ entity, std::forward<T>(component) });
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
		std::vector<Std::Pair<Entity, Component::GuiPlane>> guiPlanes;
		std::vector<Entity> entities;
	};

	template<>
	inline Std::Span<Std::Pair<Entity, Component::GuiPlane>> Scene::GetAllComponents<Component::GuiPlane>() { return { guiPlanes.data(), guiPlanes.size() }; }
	template<>
	inline Std::Span<Std::Pair<Entity, Component::GuiPlane> const> Scene::GetAllComponents<Component::GuiPlane>() const { return { guiPlanes.data(), guiPlanes.size() }; }
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
	inline std::vector<Std::Pair<Entity, Component::GuiPlane>>& Scene::Impl_GetAllComponents<Component::GuiPlane>() { return guiPlanes; }
	template<>
	inline std::vector<Std::Pair<Entity, Component::GuiPlane>> const& Scene::Impl_GetAllComponents<Component::GuiPlane>() const { return guiPlanes; }
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

