#define INPUT_BUTTON_COUNT
#include "InputRaw.hpp"
#include "Math/Vector/Vector.hpp"

#include "SDL2/SDL.h"

#include <array>
#include <memory>

namespace Engine
{
	namespace Input
	{
		namespace Core
		{
			struct Data
			{
				std::array<bool, static_cast<size_t>(Input::Raw::Button::COUNT)> buttonValues;
				std::array<Input::EventType, static_cast<size_t>(Input::Raw::Button::COUNT)> eventTypes;

				Math::Vector<2, int16_t> mousePosition;
				Math::Vector<2, int16_t> mouseDelta;
			};

			static std::unique_ptr<Data> data;

			EventType ToEventType(bool input);
		}
	}
}

Engine::Input::EventType Engine::Input::Core::ToEventType(bool input) { return input ? Engine::Input::EventType::Pressed : Engine::Input::EventType::Unpressed; }

bool Engine::Input::Raw::GetValue(Button input) { return Core::data->buttonValues[static_cast<size_t>(input)]; }

Engine::Input::EventType Engine::Input::Raw::GetEventType(Button input) { return Core::data->eventTypes[static_cast<size_t>(input)]; }

Math::Vector<2, int16_t> Engine::Input::Raw::GetMouseDelta() { return Core::data->mouseDelta; }

void Engine::Input::Core::UpdateSingle(bool buttonValue, Raw::Button button) 
{
	size_t index = static_cast<size_t>(button);
	data->buttonValues[index] = buttonValue;
	data->eventTypes[index] = ToEventType(buttonValue);
}

void Engine::Input::Core::UpdateMouseInfo(int16_t posX, int16_t posY, int16_t moveX, int16_t moveY)
{
	data->mousePosition = { posX, posY };
	data->mouseDelta = { moveX, moveY };
}

void Engine::Input::Core::Initialize()
{
	data = std::make_unique<Data>();

	data->buttonValues.fill(false);
	data->eventTypes.fill(EventType::Unchanged);
}

void Engine::Input::Core::Terminate()
{
	data = nullptr;
}

void Engine::Input::Core::ClearValues()
{
	data->eventTypes.fill(Input::EventType::Unchanged);
	data->mouseDelta = { 0, 0 };
}