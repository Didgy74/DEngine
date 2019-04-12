#pragma once

#include <cstdint>
#include <array>

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

			std::array<int16_t, 2> GetMouseDelta();
		}

		namespace Core
		{
			void UpdateKey(bool buttonValue, Input::Raw::Button button);
			void UpdateMouseInfo(uint16_t posX, uint16_t posY);
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
	LeftCtrl,
	LeftMouse,
	RightMouse,
#ifdef INPUT_BUTTON_COUNT
	COUNT
#endif
};