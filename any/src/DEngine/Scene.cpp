#include <DEngine/Scene.hpp>

#include <DEngine/GuiPlaneComponentSystem.hpp>

#include <cmath>

using namespace DEngine;

void Scene::Copy(Scene& output) const
{
	DENGINE_IMPL_ASSERT(output.GetEntities().Empty());

	output.entities = entities;
	output.entityIdIncrementor = entityIdIncrementor;
	output.moves = moves;
	output.rigidBodies = rigidBodies;
	output.textureIDs = textureIDs;
	output.transforms = transforms;

	DENGINE_IMPL_ASSERT(m_createGuiPlaneWidgetFn);
	output.m_createGuiPlaneWidgetFn = m_createGuiPlaneWidgetFn;

	// Build GUI planes
	for (auto const& [entity, plane] : GetAllComponents<Component::GuiPlane>()) {
		auto widget = m_createGuiPlaneWidgetFn(plane.constructorId);
		output.AddComponent(entity, Component::GuiPlane{
			.constructorId = plane.constructorId,
			.widget = m_createGuiPlaneWidgetFn(plane.constructorId)
		});
	}

	// Don't need to copy physics world, it's not initialized anyways.
}

Entity Scene::NewEntity() noexcept
{
	auto returnVal = (Entity)entityIdIncrementor;
	entityIdIncrementor += 1;
	entities.push_back(returnVal);
	return returnVal;
}

void Scene::DeleteEntity(Entity ent) noexcept
{
	DENGINE_IMPL_ASSERT(ValidateEntity(ent));
	DeleteComponent_CanFail<Transform>(ent);
	DeleteComponent_CanFail<Gfx::TextureID>(ent);
	DeleteComponent_CanFail<Move>(ent);

	for (uSize i = 0; i < entities.size(); i += 1)
	{
		if (ent == entities[i])
		{
			entities.erase(entities.begin() + i);
			break;
		}
	}
}

// Confirm that this entity exists.
bool Scene::ValidateEntity(Entity entity) const noexcept
{
	for (auto const& item : entities) {
		if (item == entity)
			return true;
	}
	return false;
}

void Scene::Begin()
{
	DENGINE_IMPL_ASSERT(!physicsWorld);

	auto physWorld = new b2World({ 0.f, -10.f });
	physicsWorld = Std::Box{ physWorld };

	for (auto& [entity, rb] : GetAllComponents<Physics::Rigidbody2D>()) {
		auto *transformPtr = GetComponent<Transform>(entity);
		if (transformPtr == nullptr) {
			continue;
		}
		auto const& transform = *transformPtr;

		b2BodyDef bodyDef = {};
		bodyDef.angle = Math::degToRad * transform.rotation.ToEulerAngles().z;
		bodyDef.awake = true;
		bodyDef.enabled = true;
		bodyDef.gravityScale = 1.f;
		bodyDef.position = { transform.position.x, transform.position.y };
		switch (rb.type) {
		case Physics::Rigidbody2D::Type::Dynamic:
			bodyDef.type = b2BodyType::b2_dynamicBody;
			break;
		case Physics::Rigidbody2D::Type::Static:
			bodyDef.type = b2BodyType::b2_staticBody;
			break;
		default:
			DENGINE_IMPL_UNREACHABLE();
			break;
		}
		b2Body* newBody = physWorld->CreateBody(&bodyDef);

		rb.b2BodyPtr = newBody;

		b2FixtureDef fixtureDef{};
		fixtureDef.density = 1.f;
		fixtureDef.friction = 1.f;
		fixtureDef.restitution = 0.f;

		b2PolygonShape shape = {};
		shape.SetAsBox(transform.scale.x / 2.f, transform.scale.y / 2.f);
		fixtureDef.shape = &shape;

		newBody->CreateFixture(&fixtureDef);
	}
}

// NOTE: Very WIP stuff here
void PlatformToSceneEventForwarder::ButtonEvent(
	Platform::Context& platformCtx,
	Platform::WindowID windowId,
	Platform::Button button,
	bool state)
{
	auto& scene = GetScene();
	auto& textEngine = GetTextEngine();

	if (!state) {
		return;
	}

	Std::Array<Platform::Button, 4> directionalButtons = {
		Platform::Button::Left,
		Platform::Button::Right,
		Platform::Button::Up,
		Platform::Button::Down,
	};

	// If we didn't get a directional button, just return early
	if (directionalButtons.ToSpan().Contains(button) == true) {
		f32 directionalAngle = 0;
		if (button == Platform::Button::Up) {
			directionalAngle = 0;
		}
		if (button == Platform::Button::Left) {
			directionalAngle = 0.25f;
		}
		if (button == Platform::Button::Down) {
			directionalAngle = 0.5f;
		}
		if (button == Platform::Button::Right) {
			directionalAngle = 0.75f;
		}

		GuiPlaneComponentSystem::DispatchNavigationEvent(
			scene,
			Std::AnyRef{ scene },
			textEngine,
			platformCtx,
			directionalAngle);
	} else if (button == Platform::Button::Escape) {
		// TODO: Are we supposed to do anything with this?
		GuiPlaneComponentSystem::DispatchBackEvent(
			scene,
			Std::AnyRef{ scene },
			textEngine,
			platformCtx);
	} else if (button == Platform::Button::Space && state) {
		GuiPlaneComponentSystem::DispatchActivateEvent(
			scene,
			Std::AnyRef{ scene },
			textEngine,
			platformCtx);
	} else if (button == Platform::Button::Tab && state) {

	}
}

void PlatformToSceneEventForwarder::TextInputEvent(
	Platform::Context& platformCtx,
	Platform::WindowID windowId,
	u64 start,
	u64 count,
	Std::Span<u32 const> newText)
{
	Gui::Widget::WidgetEvent_TextInputParams event = {};
	event.start = start;
	event.count = count;
	event.newText = newText;
	GuiPlaneComponentSystem::DispatchTextInputEvent(GetScene(), event);
}

void PlatformToSceneEventForwarder::TextSelectionEvent(
	Platform::Context& platformCtx,
	Platform::WindowID windowId,
	u64 start,
	u64 count)
{
	Gui::Widget::WidgetEvent_TextSelectionParams event = {};
	event.start = start;
	event.count = count;
	GuiPlaneComponentSystem::DispatchTextSelectionEvent(GetScene(), event);
}

void PlatformToSceneEventForwarder::EndTextInputSessionEvent(
	Platform::Context& platformCtx,
	Platform::WindowID windowId)
{
	Gui::Widget::WidgetEvent_EndTextInputSessionParams event = {};
	GuiPlaneComponentSystem::DispatchEndTextInputSessionEvent(GetScene(), event);
}

void PlatformToSceneEventForwarder::GamepadKeyEvent(
	Platform::Context& platformCtx,
	Platform::WindowID windowId,
	Platform::GamepadDeviceId deviceId,
	Platform::GamepadKey key,
	bool pressed)
{
	if (!pressed) {
		return;
	}

	auto& scene = GetScene();
	auto& textEngine = GetTextEngine();

	if (key == Platform::GamepadKey::A) {
		GuiPlaneComponentSystem::DispatchActivateEvent(
			scene,
			Std::AnyRef{ scene },
			textEngine,
			platformCtx);
	} else if (key == Platform::GamepadKey::B) {
		GuiPlaneComponentSystem::DispatchBackEvent(
			scene,
			Std::AnyRef{ scene },
			textEngine,
			platformCtx);
	}
}

void PlatformToSceneEventForwarder::GamepadAxisEvent(
	Platform::Context& platformCtx,
	Platform::WindowID windowId,
	Platform::GamepadDeviceId deviceId,
	Platform::GamepadAxis axis,
	f32 value)
{
	auto& nav = GetScene().m_joystickNavState;

	if (axis == Platform::GamepadAxis::LeftX) {
		nav.leftX = value;
	} else if (axis == Platform::GamepadAxis::LeftY) {
		nav.leftY = value;
	} else {
		return;
	}

	constexpr f32 pressThresholdSq   = 0.5f * 0.5f;
	constexpr f32 releaseThresholdSq  = 0.25f * 0.25f;
	f32 const magnitudeSq = (nav.leftX * nav.leftX) + (nav.leftY * nav.leftY);

	if (nav.navActive) {
		if (magnitudeSq < releaseThresholdSq) {
			nav.navActive = false;
		}
		return;
	}

	if (magnitudeSq < pressThresholdSq) {
		return;
	}
	
	nav.navActive = true;

	// Convert to directional angle matching the existing convention:
	//   0 = up, 0.25 = left, 0.5 = down, 0.75 = right  (CCW from up)
	// LeftX positive = right, LeftY positive = up (XInput convention).
	constexpr f32 twoPi = 6.28318530717958647692f;
	f32 rawAngle = std::atan2(-nav.leftX, nav.leftY);
	f32 directionalAngle = rawAngle / twoPi;
	if (directionalAngle < 0.f) {
		directionalAngle += 1.f;
	}

	auto& scene = GetScene();
	auto& textEngine = GetTextEngine();
	GuiPlaneComponentSystem::DispatchNavigationEvent(
		scene,
		Std::AnyRef{ scene },
		textEngine,
		platformCtx,
		directionalAngle);
}