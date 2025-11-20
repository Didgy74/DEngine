export module DEngine.Gui.DrawEngine;

import DEngine.FixedWidthTypes;
import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.Utility;
import DEngine.Math.Vector;
import DEngine.Std.Span;

export namespace DEngine::Gui {
	/**
		This class is meant to be a short-lived interface for an actual draw engine.

		The Widget classes rely on this interface to draw their contents. The DrawEngine should
		only be alive for the duration of a single render event.
	*/
	class DrawEngine {
	public:
		class ScopedScissor {
		public:
			// Only activates if the target rect cannot fit within the visible rect.
			ScopedScissor(
				DrawEngine& drawInfoIn,
				Rect const& targetRect,
				Rect const& visibleRect)
			{
				// This seems error prone.
				if (!targetRect.FitsInside(visibleRect)) {
					drawInfo = &drawInfoIn;
					drawInfo->PushScissor(Intersection(targetRect, visibleRect));
				}
			}
			ScopedScissor(DrawEngine& drawInfo, Rect const& scissor) : drawInfo{ &drawInfo } {
				drawInfo.PushScissor(scissor);
			}
			ScopedScissor(ScopedScissor const&) = delete;
			ScopedScissor(ScopedScissor&& other) noexcept : drawInfo{ other.drawInfo } {
				other.drawInfo = nullptr;
			}

			ScopedScissor& operator=(ScopedScissor const&) = delete;
			ScopedScissor& operator=(ScopedScissor&& other) noexcept {
				if (this == &other) {
					return *this;
				}
				drawInfo = other.drawInfo;
				other.drawInfo = nullptr;
				return *this;
			}

			~ScopedScissor() {
				if (drawInfo) {
					drawInfo->PopScissor();
				}
			}

		protected:
			DrawEngine* drawInfo = nullptr;
		};

		class ScopedMask {
		private:
			DrawEngine* m_drawInfo = nullptr;
			// For now this is just an index into the stack of StencilMasks
			u64 m_id = 0;
			// Clears members without running release stuff.
			void Reset() {
				m_drawInfo = nullptr;
				m_id = 0;
			}
		public:
			explicit ScopedMask(DrawEngine& drawInfo, u64 id) noexcept {
				m_drawInfo = &drawInfo;
				m_id = id;
			}
			explicit ScopedMask(ScopedMask&& other) noexcept {
				m_drawInfo = other.m_drawInfo;
				m_id = other.m_id;
				other.Reset();
			}
			ScopedMask& operator=(ScopedMask&& other) noexcept {
				if (this == &other)
					return *this;
				Release();
				m_drawInfo = other.m_drawInfo;
				m_id = other.m_id;
				return *this;
			}

			ScopedMask(ScopedMask const&) = delete;
			ScopedMask& operator=(ScopedMask const&) = delete;

			void Release() {
				if (m_drawInfo != nullptr) {
					m_drawInfo->PopMask(m_id);
					Reset();
				}
			}
			~ScopedMask() { Release(); }
		};

		virtual ~DrawEngine() = default;

		[[nodiscard]] virtual Extent GetFramebufferExtent() const = 0;

		// I'm not quite happy with this API because it requires copies.
		//
		// The goal of any newer PushText API is to let us push Rects without doing any temporary copies.
		// Ideally we should be able to push all our Rects and have the backend automatically convert when
		// necessary. There's some complications because to get the Gui::Rects, we need to have TextEngine
		// fill out the data.
		//
		// The intention is to have Rect-building and the TextEngine calls inside happening inside the callback.
		virtual void PushText(
			FontFaceSizeId fontFaceId,
			Std::Span<char const> text,
			Rect const* rects,
			Math::Vec2Int posOffset,
			Math::Vec4 color) = 0;

		virtual void PushScissor(Rect rect) = 0;
		virtual void PopScissor() = 0;

		// Angle dir is in range [0, 1] and rotates the gradient direction counter-clockwise
		// 0 representing downwards and 1 represents a full rotation.
		virtual void Gradient(Rect rect, Math::Vec4 colorA, Math::Vec4 colorB, f32 angleDir) = 0;

		enum class MaskOp {
			// The area outside the shape will be hidden in subsequent draws.
			Outside,
			// The area inside the shape will be hidden in subsequent draws.
			Inside
		};
		[[nodiscard]] virtual u64 PushRectMask(Rect rect, Math::Vec4Int radius, MaskOp op) = 0;
		[[nodiscard]] u64 PushRectMask(Rect rect, u32 radius, MaskOp op) {
			return PushRectMask(
				rect,
				Math::Vec4Int::SingleValue((i32)radius),
				op);
		}
		[[nodiscard]] ScopedMask PushRectMaskScoped(Rect rect, Math::Vec4Int radius , MaskOp op) {
			return ScopedMask{
				*this,
				PushRectMask(rect, radius, op) };
		}
		[[nodiscard]] ScopedMask PushRectMaskScoped(Rect rect, u32 radius, MaskOp op) {
			return PushRectMaskScoped(
				rect,
				Math::Vec4Int::SingleValue((i32)radius),
				op);
		}
		virtual void PopMask(u64) = 0;

		virtual void PushFilledQuad(Rect rect, Math::Vec4 color, Math::Vec4Int radius) = 0;
		void PushFilledQuad(Rect rect, Math::Vec4 color, u32 radius) { PushFilledQuad(rect, color, Math::Vec4Int::SingleValue((i32)radius)); }
		void PushFilledQuad(Rect rect, Math::Vec4 color) { PushFilledQuad(rect, color, {}); }

		virtual void PushRectShadow(Rect rect, Math::Vec4Int radius, f32 falloff, f32 alpha) = 0;
		void PushRectShadow(Rect rect, u32 radius, f32 falloff, f32 alpha) { return PushRectShadow(rect, Math::Vec4Int::SingleValue((i32)radius), falloff, alpha); }
	};
}
