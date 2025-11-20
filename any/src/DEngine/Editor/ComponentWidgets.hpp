#pragma once

#include <DEngine/Gui/StdWidgets/CollapsingHeader.hpp>
#include <DEngine/Gui/StdWidgets/Dropdown.hpp>
#include <DEngine/Gui/StdWidgets/LineIntEdit.hpp>
#include <DEngine/Gui/StdWidgets/LineFloatEdit.hpp>
#include <DEngine/Gui/StdWidgets/Text.hpp>

#include <DEngine/Scene.hpp>

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

		Gui::LineFloatEdit* positionInputFields[3] = {};
		Gui::LineFloatEdit* rotationInput = nullptr;
		Gui::LineFloatEdit* scaleInputFields[2] = {};

		explicit TransformWidget(EditorImpl const& editorImpl);
		void Update(ComponentType const& component);
	};

	class SpriteRenderer2DWidget : public Gui::CollapsingHeader
	{
	public:
		using ComponentType = Gfx::TextureID;

		Gui::LineIntEdit* textureIdInput = nullptr;
		
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