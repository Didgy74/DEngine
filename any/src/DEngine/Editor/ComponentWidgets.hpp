#pragma once

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/CollapsingHeader.hpp>
#include <DEngine/Gui/Dropdown.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/LineFloatEdit.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>

#include <DEngine/Scene.hpp>

#include <DEngine/Std/Utility.hpp>

namespace DEngine::Editor
{
	class EditorImpl;

	class MoveWidget : public Gui::CollapsingHeader
	{
	public:
		MoveWidget(EditorImpl const& editorImpl);
	};
	
	class TransformWidget : public Gui::CollapsingHeader
	{
	public:
		Gui::LineEdit* positionInputFields[3] = {};
		Gui::LineEdit* rotationInput = nullptr;
		Gui::LineEdit* scaleInputFields[2] = {};

		TransformWidget(EditorImpl const& editorImpl);
		void Update(Transform const& component);
	};

	class SpriteRenderer2DWidget : public Gui::CollapsingHeader
	{
	public:
		Gui::LineEdit* textureIdInput = nullptr;
		
		SpriteRenderer2DWidget(EditorImpl const& editorImpl);
		void Update(Gfx::TextureID const& component);
	};

	class RigidbodyWidget : public Gui::CollapsingHeader
	{
		Gui::Dropdown* bodyTypeDropdown = nullptr;

		Gui::Text* debug_VelocityLabel = nullptr;

	public:
		RigidbodyWidget(EditorImpl const& editorImpl);
		void Update(Physics::Rigidbody2D const& component);
	};
}