#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/FnRef.hpp>

#include <string>

import DEngine.Gui.FontFaceSizeId;
import DEngine.Math.Vector;

namespace DEngine::Gui::LineEditUtils {
    struct SelectionRange {
        u64 start;
        u64 count;
    };

    struct GetSizeHintParams {
        TextEngine& textEngine;
        f32 const& windowContentScale;
        f32 const& fontScale;
        f32 const& windowDpi;
        Std::Span<char const> const& text;
        Std::Span<Rect> const& outGlyphRects;
        f32 const& minimumSizeCm;
    	u32 const& textMarginPx;
    };
    struct GetSizeHint_Return {
        SizeHint sizeHint = {};
        FontFaceSizeId fontId = {};
        Extent textOuterExtent = {};
    };
    [[nodiscard]] GetSizeHint_Return GetSizeHint(
        GetSizeHintParams const &);

    struct Render_Params {
        Widget::Render_Params const& renderParams;
    	Widget const& widget;
    	RectCollection const& rectColl;
        Std::Opt<RectCollection::Iter> const& rectCollIter;
        Math::Vec4 const& backgroundColor;
		Math::Vec4 const& textColor;
        Extent const& textOuterExtent;
        Std::Span<char const> const& text;
        Std::Span<Rect const> const& glyphRects;
        FontFaceSizeId const& fontId;
        Std::Opt<SelectionRange> const& selectionRangeOpt;
    	u32 const& cornerRadiusPx;
    };

    void Render(Render_Params const &);

    enum class PointerType : u8 { Primary, Secondary };
    [[nodiscard]] inline PointerType ToPointerType(CursorButton in) noexcept
    {
        switch (in) {
            case CursorButton::Primary: return PointerType::Primary;
            case CursorButton::Secondary: return PointerType::Secondary;
            default:
                DENGINE_IMPL_UNREACHABLE();
                return {};
        }
    }

    static constexpr u8 cursorPointerId = ~static_cast<u8>(0);

    struct PointerPress_Pointer {
        u8 id;
        Math::Vec2 pos;
        PointerType type;
        bool pressed;
        bool consumed;
    };

    struct PointerPress_Params {
    	RectCollection const& rectColl;
    	Widget const& widget;
    	Std::Opt<RectCollection::Iter> const& rectCollIter;
        PointerPress_Pointer const& pointer;
        Std::Opt<u8> currentlyHeldPointerIdOpt;
        bool const& hasTextEditingSession;
        Std::FnRef<void(Std::Opt<u8>)> const& setHeldPointerIdFn;
        Std::FnRef<void()> const& startInputConnectionFn;
        Std::FnRef<void()> const& endInputConnectionFn;
    };

    [[nodiscard]] TouchEventConsumption PointerPress(PointerPress_Params const &);

    struct PointerMove_Pointer {
        u8 id;
        Math::Vec2 pos;
        bool consumed;
    };
    struct PointerMove_Params {
    	RectCollection const& rectColl;
    	Widget const& widget;
    	Std::Opt<RectCollection::Iter> const& rectCollIter;
        PointerMove_Pointer const& pointer;
        Std::Opt<u8> currentlyHeldPointerIdOpt;
        Std::FnRef<void(Std::Opt<u8>)> const& setHeldPointerIdFn;
    };
    [[nodiscard]] TouchEventConsumption PointerMove(PointerMove_Params const&);

    struct TextInput_Params {
        std::string& sourceText;
        Widget::WidgetEvent_TextInputParams const& event;
        Std::FnRef<void(SelectionRange)> const& setSelectionRangeFn;
    };
    void TextInput(TextInput_Params const&);

    struct TextSelection_Params {
        std::string const& sourceText;
        Widget::WidgetEvent_TextSelectionParams const& event;
        Std::FnRef<void(SelectionRange)> const& setSelectionRangeFn;
    };
    void TextSelection(TextSelection_Params const&);

    struct AccessibilityStuff_Params {
    	Widget::AccessibilityTest_Params const& eventParams;
        Widget const& widget;
    	Std::Opt<RectCollection::Iter> const& rectCollIter;
        Std::Span<char const> const& text;
        Widget::AccessibilityInfoPusher& pusher;
    };
    void AccessibilityStuff(AccessibilityStuff_Params const&);
}
