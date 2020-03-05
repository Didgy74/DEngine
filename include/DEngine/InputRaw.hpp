#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Array.hpp"

namespace DEngine::Input
{
	enum class EventType : u8;

	namespace Raw
	{
		enum class Button : u16;

		bool GetValue(Button input);

		EventType GetEventType(Button input);

		Cont::Array<i16, 2> GetMouseDelta();
	}

	namespace Core
	{
		void UpdateKey(Raw::Button button, bool buttonValue);
		void UpdateMouseInfo(u16 posX, u16 posY);
		void TickStart();
	}
}

enum class DEngine::Input::EventType : DEngine::u8
{
	Unchanged,
	Unpressed,
	Pressed,
#ifdef DENGINE_INPUT_EVENTTYPE_COUNT
	COUNT
#endif
};

enum class DEngine::Input::Raw::Button : DEngine::u16
{
	Undefined,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	Up, Down, Left, Right,
	Space,
	LeftCtrl,
	LeftMouse,
	RightMouse,
#ifdef DENGINE_INPUT_RAW_BUTTON_COUNT
	COUNT
#endif
};