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
		using ComponentType = Move;

		MoveWidget(EditorImpl const& editorImpl);
	};
	
	class TransformWidget : public Gui::CollapsingHeader
	{
	public:
		using ComponentType = Transform;

		Gui::LineEdit* positionInputFields[3] = {};
		Gui::LineEdit* rotationInput = nullptr;
		Gui::LineEdit* scaleInputFields[2] = {};

		explicit TransformWidget(EditorImpl const& editorImpl);
		void Update(ComponentType const& component);
	};

	class SpriteRenderer2DWidget : public Gui::CollapsingHeader
	{
	public:
		using ComponentType = Gfx::TextureID;

		Gui::LineEdit* textureIdInput = nullptr;
		
		explicit SpriteRenderer2DWidget(EditorImpl const& editorImpl);
		void Update(ComponentType const& component);
	};

	class RigidbodyWidget : public Gui::CollapsingHeader
	{
	public:
		using ComponentType = Physics::Rigidbody2D;

		Gui::Dropdown* bodyTypeDropdown = nullptr;

		Gui::Text* debug_VelocityLabel = nullptr;

		explicit RigidbodyWidget(EditorImpl const& editorImpl);
		void Update(ComponentType const& component);
	};
}