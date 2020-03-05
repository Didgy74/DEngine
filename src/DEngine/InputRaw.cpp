#define DENGINE_INPUT_EVENTTYPE_COUNT
#define DENGINE_INPUT_RAW_BUTTON_COUNT
#include "DEngine/InputRaw.hpp"

namespace DEngine::Input::Raw
{
	static Cont::Array<bool, uSize(Button::COUNT)> buttonValues{};
	static Cont::Array<EventType, uSize(Button::COUNT)> buttonEvents{};

	static Cont::Array<u16, 2> mousePosition{};
	static Cont::Array<i16, 2> mouseDelta{};
}

bool DEngine::Input::Raw::GetValue(Button input)
{
	return buttonValues[uSize(input)];
}

DEngine::Input::EventType DEngine::Input::Raw::GetEventType(Button input)
{
	return buttonEvents[uSize(input)];
}

DEngine::Cont::Array<DEngine::i16, 2> DEngine::Input::Raw::GetMouseDelta()
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

void DEngine::Input::Core::UpdateMouseInfo(u16 posX, u16 posY)
{
	Raw::mouseDelta[0] = i16(posX) - i16(Raw::mousePosition[0]);
	Raw::mouseDelta[1] = i16(posY) - i16(Raw::mousePosition[1]);
	Raw::mousePosition = { posX, posY };
}

void DEngine::Input::Core::TickStart()
{
	Raw::buttonEvents.Fill(EventType::Unchanged);
	Raw::mouseDelta = { 0, 0 };
}
