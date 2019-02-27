#pragma once

#include "Math/Vector/Vector.hpp"

#include <cstdint>

namespace Engine
{
	namespace Input
	{
		enum class EventType : uint8_t;

		namespace Raw
		{
			enum class Button : uint16_t;

			bool GetValue(Button input);

			EventType GetEventType(Button input);

			Math::Vector<2, int16_t> GetMouseDelta();
		}

		namespace Core
		{
			void UpdateSingle(bool buttonValue, Input::Raw::Button button);
			void UpdateMouseInfo(int16_t posX, int16_t posY, int16_t moveX, int16_t moveY);
			void Initialize();
			void Terminate();
			void ClearValues();
		}
	}
}

enum class Engine::Input::EventType : uint8_t
{
	Unpressed,
	Pressed,
	Unchanged,
#ifdef INPUT_EVENTTYPE_COUNT
	COUNT
#endif
};

enum class Engine::Input::Raw::Button : uint16_t
{
	Undefined,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	Up, Down, Left, Right,
	Space,
	Ctrl,
	LeftMouse,
	RightMouse,
#ifdef INPUT_BUTTON_COUNT
	COUNT
#endif
};