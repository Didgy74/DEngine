#pragma once

#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Gui/Widget.hpp>

import DEngine.FixedWidthTypes;
import DEngine.Math.Common;

namespace DEngine::impl {
	class GuiAxisInputTracker {
	public:
		f32 deadzone = 0.05f;
		f32 triggerThreshold = 0.5f;

		// Resolves the primary continuous-nav control for this tick, or nullOpt when it's idle
		// (inside the deadzone with no fresh step). Retains the previous stick reading needed to
		// derive discrete steps across ticks.
		[[nodiscard]] Std::Opt<Gui::Widget::AxisInputSource> Update(Platform::InputSnapshot const& snapshot) {
			// Primary control follows the gamepad with device id 0.
			auto gamepadCount = snapshot.GetGamepadCount();
			Std::Opt<i32> primaryIndex;
			for (i32 i = 0; i < gamepadCount; i++) {
				if (snapshot.GetGamepadDeviceId(i) == Platform::GamepadDeviceId{ 0 }) {
					primaryIndex = i;
					break;
				}
			}
			if (!primaryIndex.Has()) {
				m_prevGamepad = Std::nullOpt;
				return Std::nullOpt;
			}

			auto newState = snapshot.GetGamepadState(primaryIndex.Get());
			f32 newX = newState.GetAxisValue(Platform::GamepadAxis::RightX);
			f32 newY = newState.GetAxisValue(Platform::GamepadAxis::RightY);

			// Platform axis has +Y up; the Gui module wants +Y down. Flip Y into Gui convention.
			Math::Vec2 analogGui = { newX, -newY };

			// Steps are threshold crossings, so they need last tick's reading.
			Math::Vec2Int steps = {};
			if (m_prevGamepad.Has()) {
				f32 oldX = m_prevGamepad.Get().GetAxisValue(Platform::GamepadAxis::RightX);
				f32 oldY = m_prevGamepad.Get().GetAxisValue(Platform::GamepadAxis::RightY);

				if (oldX > -triggerThreshold && newX < -triggerThreshold) {
					steps.x = -1;
				} else if (oldX < triggerThreshold && newX > triggerThreshold) {
					steps.x = 1;
				}

				// Y flipped to Gui convention (+down).
				if (oldY > -triggerThreshold && newY < -triggerThreshold) {
					steps.y = 1;
				} else if (oldY < triggerThreshold && newY > triggerThreshold) {
					steps.y = -1;
				}
			}
			m_prevGamepad = newState;

			bool outsideDeadzone = analogGui.MagnitudeSqrd() >= Math::Sqrd(deadzone);
			bool hasSteps = steps.x != 0 || steps.y != 0;
			if (!outsideDeadzone && !hasSteps) {
				return Std::nullOpt;
			}

			Gui::Widget::AxisInputSource source = {};
			source.analog = outsideDeadzone ? analogGui : Math::Vec2{};
			source.steps = steps;
			return source;
		}

	private:
		Std::Opt<Platform::GamepadState> m_prevGamepad;
	};
}
