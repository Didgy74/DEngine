#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include <vector>

import DEngine.Gui.DrawEngine;
import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.Utility;

namespace DEngine {
    class GfxGuiDrawEngineImpl : public Gui::DrawEngine {
    public:
    	std::vector<Gfx::GuiVertex>* vertices;
    	std::vector<u32>* indices;
    	std::vector<Gfx::GuiDrawCmd>* drawCmds;
    	std::vector<u32>* m_utfValues;
    	std::vector<Gfx::GlyphRect>* m_textGlyphRects;

    protected:
    	Math::Vec2Int m_posOffset = {};
    	Gui::Extent m_framebufferExtent = {};
    	//std::vector<Gui::Rect> scissors;

    	// TODO: Rewrite with more advanced API so we can push arbitrary shapes
    	// For now we only support rounded rects as masks.
    	struct MaskItem {
    		Gui::Rect rect;
    		Math::Vec4Int radius;
    		DrawEngine::MaskOp op;
    	};
    	std::vector<MaskItem> m_maskStack;
    	std::vector<u64> m_scissors;
    public:
    	[[nodiscard]] Gui::Extent GetFramebufferExtent() const override { return m_framebufferExtent; }

    	// It's annoying to write all these parameters everywhere,
    	// so I made a struct.
    	struct Params {
    		std::vector<Gfx::GuiDrawCmd>& drawCmds;
    		std::vector<u32>& indices;
    		std::vector<Gfx::GlyphRect>& textGlyphRects;
    		std::vector<u32>& utfValues;
    		std::vector<Gfx::GuiVertex>& vertices;
    	};
    	explicit GfxGuiDrawEngineImpl(
			Params const& params,
			Gui::Extent framebufferExtent,
			Math::Vec2Int posOffset)
			: GfxGuiDrawEngineImpl {
				params.vertices,
				params.indices,
				params.drawCmds,
				params.utfValues,
				params.textGlyphRects,
				framebufferExtent,
				posOffset }
    	{}

    	explicit GfxGuiDrawEngineImpl(
			std::vector<Gfx::GuiVertex>& verticesIn,
			std::vector<u32>& indicesIn,
			std::vector<Gfx::GuiDrawCmd>& drawCmdsIn,
			std::vector<u32>& utfValuesIn,
			std::vector<Gfx::GlyphRect>& textGlyphRectsIn,
			Gui::Extent framebufferExtent,
			Math::Vec2Int posOffset) :
		vertices{ &verticesIn },
        indices{ &indicesIn },
        drawCmds{ &drawCmdsIn },
        m_utfValues{ &utfValuesIn },
        m_textGlyphRects{ &textGlyphRectsIn },
    	m_posOffset{ posOffset },
    	m_framebufferExtent{ framebufferExtent }
    	{
            BuildQuad();
        }
    	GfxGuiDrawEngineImpl(GfxGuiDrawEngineImpl const&) = delete;

        // Not part of the interface facing Gui::Widget
        u32 quadVertexOffset = 0;
        u32 quadIndexOffset = 0;

        // Not part of the interface facing Gui::Widget
        void BuildQuad() {
            quadVertexOffset = (u32)vertices->size();
            quadIndexOffset = (u32)indices->size();

            // Insert vertices
            vertices->reserve(vertices->size() + 4);
            Gfx::GuiVertex newVertex = {};
            // Top left
            newVertex = {{0.f, 0.f}, {0.f, 0.f}};

            vertices->push_back(newVertex);
            // Top right
            newVertex = {{1.f, 0.f}, {1.f, 0.f}};
            vertices->push_back(newVertex);
            // Bottom left
            newVertex = {{0.f, 1.f}, {0.f, 1.f}};
            vertices->push_back(newVertex);
            // Bottom right
            newVertex = {{1.f, 1.f}, {1.f, 1.f}};
            vertices->push_back(newVertex);

            // Insert indices
            indices->reserve(indices->size() + 6);
            indices->push_back(0);
            indices->push_back(2);
            indices->push_back(1);
            indices->push_back(1);
            indices->push_back(2);
            indices->push_back(3);
        }

        [[nodiscard]] Gfx::GuiDrawCmd::MeshSpan GetQuadMesh() const {
            Gfx::GuiDrawCmd::MeshSpan newMeshSpan = {};
            newMeshSpan.vertexOffset = quadVertexOffset;
            newMeshSpan.indexOffset = quadIndexOffset;
            newMeshSpan.indexCount = 6;
            return newMeshSpan;
        }

        void PushText(
            Gui::FontFaceSizeId fontFaceId,
            Std::Span<char const> text,
            Gui::Rect const *rects,
            Math::Vec2Int posOffset,
            Math::Vec4 color) override
        {
            DENGINE_IMPL_ASSERT((i64)fontFaceId != 0);
            DENGINE_IMPL_ASSERT((i64)fontFaceId != -1);
            DENGINE_IMPL_ASSERT(rects != nullptr);

            int const length = (int) text.Size();

            auto& utfValues = *this->m_utfValues;
            int utfStartIndex = (int)utfValues.size();
            utfValues.resize(utfStartIndex + length);
            for (int i = 0; i < length; i++)
                utfValues[i + utfStartIndex] = (u32) (unsigned char) text[i];

            auto& textGlyphRects = *this->m_textGlyphRects;
            textGlyphRects.resize(utfStartIndex + length);
            for (int i = 0; i < length; i++) {
                auto const& srcRect = rects[i];
                Gfx::GlyphRect outRect = {};
                outRect.posPx = srcRect.position;
                outRect.extentPx = { (i32)srcRect.extent.width, (i32)srcRect.extent.height };
                textGlyphRects[i + utfStartIndex] = outRect;
            }

            Gfx::GuiDrawCmd cmd = {};
            cmd.type = Gfx::GuiDrawCmd::Type::Text;
            cmd.text = {};
            cmd.text.startIndex = utfStartIndex;
            cmd.text.count = length;
            cmd.text.fontFaceId = (Gfx::FontFaceId) fontFaceId;
            cmd.rectPosPx.x = posOffset.x + m_posOffset.x;
            cmd.rectPosPx.y = posOffset.y + m_posOffset.y;
            cmd.text.color = color;
            drawCmds->push_back(cmd);
        }

    	/*
        void PushScissor(Gui::Rect rect) override {
            DENGINE_IMPL_GUI_ASSERT(!this->m_framebufferExtent.IsNothing());
        	DENGINE_IMPL_GUI_ASSERT(!rect.IsNothing());
        	const auto framebufferExtent = this->m_framebufferExtent;

            Gui::Rect rectToUse = {};
            if (!scissors.empty())
                rectToUse = Gui::Rect::Intersection(rect, scissors.back());
            else
                rectToUse = Gui::Rect::Intersection(rect, Gui::Rect{{}, framebufferExtent });
            scissors.push_back(rectToUse);

            Gfx::GuiDrawCmd cmd = {};
            cmd.type = Gfx::GuiDrawCmd::Type::Scissor;
            cmd.rectPosPx = rectToUse.position;
            cmd.rectExtentPx = { (i32)rectToUse.extent.width, (i32)rectToUse.extent.height };

        	DENGINE_IMPL_GUI_ASSERT(cmd.rectExtentPx.x > 0 && cmd.rectExtentPx.y > 0);
        	DENGINE_IMPL_GUI_ASSERT(cmd.rectPosPx.x + cmd.rectExtentPx.x <= framebufferExtent.width);
        	DENGINE_IMPL_GUI_ASSERT(cmd.rectPosPx.y  + cmd.rectExtentPx.y <= framebufferExtent.height);
        	PushBackCmd(cmd);
        }

        void PopScissor() override {
            DENGINE_IMPL_GUI_ASSERT(!scissors.empty());
            scissors.pop_back();
            Gfx::GuiDrawCmd cmd = {};
            cmd.type = Gfx::GuiDrawCmd::Type::Scissor;
            if (!scissors.empty()) {
                Gui::Rect rect = scissors.back();
                cmd.rectPosPx.x = rect.position.x;
                cmd.rectPosPx.y = rect.position.y;
                cmd.rectExtentPx.x = (i32)rect.extent.width;
                cmd.rectExtentPx.y = (i32)rect.extent.height;
            } else {
                cmd.rectPosPx = {};
                cmd.rectExtentPx = { (i32)m_framebufferExtent.width, (i32)m_framebufferExtent.height };
            }
        	PushBackCmd(cmd);
        }
        */

    	virtual void PushScissor(Gui::Rect rect) override {
	        m_scissors.push_back(DrawEngine::PushRectMask(rect, 0, Gui::DrawEngine::MaskOp::Outside));
        }

    	virtual void PopScissor() override {
        	DENGINE_IMPL_ASSERT(!m_scissors.empty());
        	u64 id = m_scissors.back();
        	m_scissors.pop_back();
        	PopMask(id);
        }

    	virtual u64 PushRectMask(Gui::Rect rect, Math::Vec4Int radiusPx, Gui::DrawEngine::MaskOp op) override {
        	DENGINE_IMPL_ASSERT(!this->m_framebufferExtent.IsNothing());
        	Gfx::GuiDrawCmd cmd = {};
        	cmd.type = Gfx::GuiDrawCmd::Type::RectangleStencil;
        	cmd.rectPosPx = rect.position + this->m_posOffset;
        	cmd.rectExtentPx = { (i32)rect.extent.width, (i32)rect.extent.height };
        	auto& data = cmd.rectangleStencil;
        	data = {};
        	data.radiusPx = radiusPx;
        	data.increment = true;
        	data.op = ToGfxMaskOp(op);

			u64 returnValue = this->m_maskStack.size();
        	this->m_maskStack.push_back(MaskItem { .rect = rect, .radius = radiusPx, .op = op });
        	PushBackCmd(cmd);
        	return returnValue;
        }

    	virtual void PopMask(u64 id) override {
        	DENGINE_IMPL_ASSERT(m_maskStack.size() > 0);
        	DENGINE_IMPL_ASSERT(id == m_maskStack.size() - 1);
        	auto maskItem = m_maskStack.back();
        	m_maskStack.pop_back();
        	auto rect = maskItem.rect;
        	auto radius = maskItem.radius;

        	DENGINE_IMPL_ASSERT(!this->m_framebufferExtent.IsNothing());
        	Gfx::GuiDrawCmd cmd = {};
        	cmd.type = Gfx::GuiDrawCmd::Type::RectangleStencil;
        	cmd.rectPosPx = rect.position;
        	cmd.rectExtentPx = { (i32)rect.extent.width, (i32)rect.extent.height };
        	auto& data = cmd.rectangleStencil;
        	data = {};
        	data.radiusPx = radius;
        	data.increment = false;
        	data.op = ToGfxMaskOp(maskItem.op);

        	PushBackCmd(cmd);
        }

    	virtual void PushRectShadow(Gui::Rect rect, Math::Vec4Int radiusPx, f32 falloffPx, f32 alpha) override {
        	DENGINE_IMPL_ASSERT(alpha >= 0.0f && alpha <= 1.0f);
        	Gfx::GuiDrawCmd cmd = {};
        	cmd.rectPosPx = rect.position;
        	cmd.rectExtentPx = { (i32)rect.extent.width, (i32)rect.extent.height };
        	cmd.type = Gfx::GuiDrawCmd::Type::RectangleShadow;
        	auto& data = cmd.rectangleShadow;
        	data = Gfx::GuiDrawCmd::RectangleShadow{};
        	data.falloffPx = falloffPx;
        	data.radiusPx = radiusPx;
        	data.alpha = alpha;
        	PushBackCmd(cmd);
        }

        virtual void PushFilledQuad(Gui::Rect rect, Math::Vec4 color, Math::Vec4Int radiusPx) override {
        	DENGINE_IMPL_ASSERT(!rect.IsNothing());
            Gfx::GuiDrawCmd cmd = {};
        	cmd.rectPosPx = rect.position + this->m_posOffset;
        	cmd.rectExtentPx = { (i32)rect.extent.width, (i32)rect.extent.height };
        	cmd.type = Gfx::GuiDrawCmd::Type::Rectangle;
        	cmd.rectangle = Gfx::GuiDrawCmd::Rectangle{};
        	cmd.rectangle.color = color;
        	cmd.rectangle.radiusPx = radiusPx;
        	PushBackCmd(cmd);
        }

    	void PushViewport(Gui::Rect rect, Gfx::ViewportID viewportId) {
        	Gfx::GuiDrawCmd cmd = {};
        	cmd.rectPosPx = rect.position;
        	cmd.rectExtentPx = { (i32)rect.extent.width, (i32)rect.extent.height };
        	cmd.type = Gfx::GuiDrawCmd::Type::Viewport;
        	cmd.viewport.id = viewportId;
        	PushBackCmd(cmd);
        }

    	virtual void Gradient(Gui::Rect rect, Math::Vec4 colorA, Math::Vec4 colorB, f32 angleDir) override {
        	Gfx::GuiDrawCmd cmd = {};
        	cmd.rectPosPx = rect.position;
        	cmd.rectExtentPx = { (i32)rect.extent.width, (i32)rect.extent.height };
        	cmd.type = Gfx::GuiDrawCmd::Type::Gradient;
        	auto& data = cmd.gradient;
        	data = Gfx::GuiDrawCmd::Gradient{};
        	data.colorA = colorA;
        	data.colorB = colorB;
        	data.dirAngle = angleDir;
        	PushBackCmd(cmd);
        }

    protected:
    	void PushBackCmd(Gfx::GuiDrawCmd const& cmd) {
    		//DENGINE_IMPL_GUI_ASSERT(cmd.rectPosPx.x <= (i32)m_framebufferExtent.width);
    		//DENGINE_IMPL_GUI_ASSERT(cmd.rectPosPx.y <= (i32)m_framebufferExtent.height);
    		drawCmds->push_back(cmd);
    	}

    	[[nodiscard]] static Gfx::GuiDrawCmd::MaskOp ToGfxMaskOp(Gui::DrawEngine::MaskOp in)
    	{
    		switch (in) {
    			case Gui::DrawEngine::MaskOp::Outside: return Gfx::GuiDrawCmd::MaskOp::Outside;
    			case Gui::DrawEngine::MaskOp::Inside: return Gfx::GuiDrawCmd::MaskOp::Inside;
    			default:
    				DENGINE_IMPL_UNREACHABLE();
    				return {};
    		}
    	}
    };
}
