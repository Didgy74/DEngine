#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/Containers/Str.hpp>

#include <string>
#include <functional>
#include <iterator>

namespace DEngine::Gui
{
	class Text : public Widget
	{
	public:
		using ParentType = Widget;

		struct TextPusher;

		std::string text;
		Math::Vec4 color = Math::Vec4::One();
		f32 relativeScale = 1.f;
		bool expandX = true;
		std::function<void(Context const&, Std::ConstAnyRef const&, TextPusher&)> getText;

		
		virtual ~Text() override {}

		virtual SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void BuildChildRects(
			BuildChildRects_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual void Render2(
			Render_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect) const override;

		struct TextPusher {
			using PushFnT = void(*)(void*, Std::Span<char const>);
			TextPusher(void* data, PushFnT pushFn) : data{ data }, pushFn{ pushFn } {}
			void* data = nullptr;
			PushFnT pushFn = nullptr;

			void Push(Std::Span<char const> const& in) { this->pushFn(this->data, in); }
			void Push(char a) { Push(Std::Span{ a }); }

			[[nodiscard]] auto BackInserter() {
				return BackInsertIt(*this);
			}

		struct BackInsertIt {
		public:
			using iterator_category = std::output_iterator_tag;
			using value_type = char;
			using difference_type = int;
			using pointer = char*;
			using reference = char&;
			protected:
				TextPusher* textPusher = nullptr;
			public:
				BackInsertIt(TextPusher& in) : textPusher{ &in } {}

				BackInsertIt& operator=(BackInsertIt const&) = default;
				BackInsertIt& operator=(char const& in) { textPusher->Push(in); return *this; }
				BackInsertIt& operator=(char&& in) { textPusher->Push(in); return *this; }
				BackInsertIt& operator*() noexcept { return *this; }
				BackInsertIt& operator++() noexcept { return *this; }
				BackInsertIt operator++(int) noexcept { return *this; }
			};
		};

	private:
		struct Impl;
		friend Impl;
	};
}