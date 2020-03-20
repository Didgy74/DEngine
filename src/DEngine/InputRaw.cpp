#define DENGINE_INPUT_EVENTTYPE_COUNT
#define DENGINE_INPUT_RAW_BUTTON_COUNT
#include "DEngine/InputRaw.hpp"

#include <iostream>

namespace DEngine::Input::Raw
{
	static Cont::Array<bool, uSize(Button::COUNT)> buttonValues{};
	static Cont::Array<EventType, uSize(Button::COUNT)> buttonEvents{};

	static Cont::Array<u32, 2> mousePosition{};
	static Cont::Array<i32, 2> mouseDelta{};
}

bool DEngine::Input::Raw::GetValue(Button input)
{
	return buttonValues[uSize(input)];
}

DEngine::Input::EventType DEngine::Input::Raw::GetEventType(Button input)
{
	return buttonEvents[uSize(input)];
}

DEngine::Cont::Array<DEngine::i32, 2> DEngine::Input::Raw::GetMouseDelta()
{
	return mouseDelta;
}

namespace DEngine::Input
{
	EventType ToEventType(bool input)
	{
		return input ? EventType::Pressed : EventType::Unpressed;
	}
}
void DEngine::Input::Core::UpdateKey(Raw::Button button, bool buttonValue)
{
	uSize index = uSize(button);
	Raw::buttonValues[index] = buttonValue;
	Raw::buttonEvents[index] = ToEventType(buttonValue);
}

void DEngine::Input::Core::UpdateMouseInfo(u32 posX, u32 posY, i32 deltaX, i32 deltaY)
{
	Raw::mouseDelta = { deltaX, deltaY };
	Raw::mousePosition = { posX, posY };
}

void DEngine::Input::Core::TickStart()
{
	Raw::buttonEvents.Fill(EventType::Unchanged);
	Raw::mouseDelta = { 0, 0 };
}
