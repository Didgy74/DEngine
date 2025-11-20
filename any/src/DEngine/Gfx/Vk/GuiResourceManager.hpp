#pragma once

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"
#include "TransientAllocRef.hpp"
//#include "NativeWindowManager.hpp"
#include "StagingBufferAlloc.hpp"

#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Math/Matrix.hpp>

#include <DEngine/Gfx/Gfx.hpp>

#include <unordered_map>
#include <mutex>

import DEngine.Std.Array;
import DEngine.Std.Common;
import DEngine.Std.RangeFnRef;

namespace DEngine::Gfx {
	struct GuiVertex;
}

namespace DEngine::Gfx::Vk
{
	class GuiResourceManager {
	public:
		// Rationale for using a simple uniform buffer type is that we expect to have relatively few
		// windows overall. Like less than 20.
		static constexpr vk::DescriptorType windowDataUniformDescrType = vk::DescriptorType::eUniformBuffer;

		static constexpr uSize minVtxCapacity = 2048;
		vk::Buffer vtxBuffer = {};
		VmaAllocation vtxVmaAlloc = {};
		Std::Span<u8> vtxMappedMem = {};
		uSize vtxInFlightCapacity = 0;

		static constexpr uSize minIndexCapacity = 2048;
		vk::Buffer indexBuffer = {};
		VmaAllocation indexVmaAlloc = {};
		Std::Span<u8> indexMappedMem = {};
		uSize indexInFlightCapacity = 0;

		int m_inFlightCount = 0;
		uSize m_minUniformBufferAlignment = 0;

		// Our shaders demand a camera and object matrix transform, to
		// allow being placed in 3D scenes. These aren't used during
		// regular GUI rendering, but we need some default values
		// to feed into the draw-call. The both need to be dynamic uniform buffers to match
		// the 3D scene pipeline layouts. But we will always pass in 0 for the offset,
		// so we just make both descriptors point into a singular buffer.
		struct DummyCameraObjectUniforms {
			BoxVmaBuffer buffer = {};
			BoxVkDescriptorPool descrPool = {};
			vk::DescriptorSet cameraDescrSet = {};
			vk::DescriptorSet objectDescrSet = {};
		};
		DummyCameraObjectUniforms m_dummyCameraObjectUniforms = {};

		// Contains the per-window specific uniforms.
		// We store a linear buffer that contains N amount of per-window structs in memory, where N is the amount
		// of windows in existence.
		// We need to duplicate this buffer so that we have `inFlighCount` amount of buffers.
		// We update all current in-flight buffer every frame.
		struct WindowShaderUniforms {
			struct PerWindowUniform {
				int orientation = 0;
				alignas(8) Math::Vec2Int resolution = {};
			};

			BoxVmaBuffer buffer = {};
			VmaAllocationInfo vmaAllocResultInfo = {};

			BoxVkDescriptorSetLayout m_descrLayout = {};
			BoxVkDescriptorPool m_descrPool = {};
			// This needs to have length equal to total length of our buffer.
			// length = currentCapacity * inFlightCount
			std::vector<vk::DescriptorSet> windowUniformDescrSets;

			[[nodiscard]] Std::ByteSpan MappedMemory() const {
				return {
					(char*)vmaAllocResultInfo.pMappedData,
					vmaAllocResultInfo.size };
			}

			[[nodiscard]] static uSize UniformElementAlignment(uSize minUniformBufferAlignment) {
				DENGINE_IMPL_GFX_ASSERT(minUniformBufferAlignment > 0);
				return Std::CeilToMultiple((u64)sizeof(PerWindowUniform), static_cast<u64>(minUniformBufferAlignment));
			}

			[[nodiscard]] uSize Capacity(uSize minUniformBufferAlignment, int inFlightCount) const {
				DENGINE_IMPL_GFX_ASSERT(inFlightCount > 0);
				if (!Util::CheckNotNull(buffer.Handle())) {
					return 0;
				}
				DENGINE_IMPL_GFX_ASSERT(minUniformBufferAlignment > 0);
				DENGINE_IMPL_GFX_ASSERT(vmaAllocResultInfo.size > 0);
				auto elementAlignment = UniformElementAlignment(minUniformBufferAlignment);
				return vmaAllocResultInfo.size / elementAlignment / inFlightCount;
			}

			[[nodiscard]] uSize Buffer_InFlightSize(int inFlightCount) const {
				return vmaAllocResultInfo.size / inFlightCount;
			}

			// The WindowIDs are ordered the same way they are ordered in memory, for
			// the current in-flight-index.
			// This vector may be shorter than windowUniformDescrSets.
			std::vector<NativeWindowID> windowIds;

			static constexpr int minimumCapacity = 10;
		};

		WindowShaderUniforms windowUniforms = {};

		struct FilledMeshPushConstant {
			Math::Mat2 orientation;
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
			Math::Vec4 color;
		};
		BoxVkPipeline m_filledMeshPipeline = {};
		BoxVkPipelineLayout m_filledMeshPipelineLayout = {};

		struct RectanglePushConstant {
			Math::Vec2Int rectOffsetPx;
			Math::Vec2Int rectExtentPx;
			Math::Vec4 color;
			Math::Vec4Int radiusPx;
			static constexpr int sizeInBytes = 48;
		};
		BoxVkPipeline m_rectanglePipeline = {};
		BoxVkPipelineLayout m_rectanglePipelineLayout = {};

		struct StencilRectanglePushConstant {
			Math::Vec2Int rectOffsetPx;
			Math::Vec2Int rectExtentPx;
			Math::Vec4Int radiusPx;
			static constexpr int sizeInBytes = 32;
		};
		BoxVkPipelineLayout m_stencilRectanglePipelineLayout = {};
		// We need two pipelines that are nearly identical.
		// One will increment the stencil mask, the other will decrement
		BoxVkPipeline m_stencilRectanglePipelineIncrement = {};
		BoxVkPipeline m_stencilRectanglePipelineDecrement = {};

		struct RectangleShadowPushConstant {
			Math::Vec2Int rectOffsetPx;
			Math::Vec2Int rectExtentPx;
			Math::Vec4Int radiusPx;
			f32 falloffPx;
			f32 alpha;
		};
		BoxVkPipelineLayout m_rectangleShadowPipelineLayout = {};
		BoxVkPipeline m_rectangleShadowPipeline = {};

		struct GradientPushConstant {
			Math::Vec2Int rectOffsetPx;
			Math::Vec2Int rectExtentPx;
			Math::Vec4 colorA;
			Math::Vec4 colorB;
			f32 dirAngle;
		};
		BoxVkPipelineLayout m_gradientPipelineLayout = {};
		BoxVkPipeline m_gradientPipeline = {};

		std::mutex jobQueueLock;
		struct NewFontFaceJob {
			FontFaceId id;
		};
		std::vector<NewFontFaceJob> newFontFaceJobs;
		struct NewGlyphJob {
			FontFaceId fontFaceId;
			int dataOffset;
			int dataLength;
			int imgWidth;
			int imgHeight;
			u32 utfValue;
		};
		std::vector<NewGlyphJob> newGlyphJobs;
		std::vector<char> queuedGlyphBitmapData;

		struct GlyphData {
			vk::Image img = {};
			VmaAllocation imgAlloc = {};
			vk::ImageView imgView = {};
			vk::DescriptorSet descrSet = {};
		};

		struct FontFace {
			std::unordered_map<u32, GlyphData> glyphDatas;
			static constexpr uSize lowUtfGlyphDatasSize = 256;
			Std::Array<GlyphData, lowUtfGlyphDatasSize> lowUtfGlyphDatas;

			FontFace() = default;
			FontFace(FontFace const &) = delete;
			FontFace &operator=(FontFace const &) = delete;
			FontFace(FontFace &&) = default;
			FontFace &operator=(FontFace &&) = default;
		};

		struct FontFaceNode {
			FontFaceId id = FontFaceId::Invalid;
			FontFace face;
		};
		std::vector<FontFaceNode> fontFaceNodes;

		struct FontPushConstant {
			Math::Vec2Int rectOffsetPx;
			Math::Vec2Int rectExtentPx;
			Math::Vec4 color;
		};
		BoxVkDescriptorPool m_font_descrPool = {};
		BoxVkDescriptorSetLayout m_font_descrSetLayout = {};
		BoxVkSampler m_font_sampler = {};
		BoxVkPipelineLayout m_font_pipelineLayout = {};
		BoxVkPipeline m_font_pipeline = {};

		struct ViewportPushConstant {
			Math::Vec2Int rectOffsetPx;
			Math::Vec2Int rectExtentPx;
		};
		BoxVkPipeline m_viewportPipeline = {};
		BoxVkPipelineLayout m_viewportPipelineLayout = {};
		BoxVkDescriptorSetLayout m_viewportDescrSetLayout = {};
		BoxVkDescriptorPool m_viewportDescrPool = {};

		[[nodiscard]] static vk::DescriptorSetLayout BuildGuiWindowDescrLayout(
			GuiResourceManager &manager,
			DeviceDispatch const& device,
			DebugUtilsDispatch const* debugUtils);
		[[nodiscard]] vk::DescriptorSetLayout GetGuiWindowUniformDescrLayout() const;

		[[nodiscard]] uSize WindowUniformElementAlignment() const {
			return WindowShaderUniforms::UniformElementAlignment(m_minUniformBufferAlignment);
		}
		[[nodiscard]] uSize WindowUniformCapacity() const {
			DENGINE_IMPL_GFX_ASSERT(m_minUniformBufferAlignment > 0);
			DENGINE_IMPL_GFX_ASSERT(m_inFlightCount > 0);
			return windowUniforms.Capacity(this->m_minUniformBufferAlignment, m_inFlightCount);
		}
		[[nodiscard]] Std::ByteSpan WindowUniformsInFlightBufferSpan(u32 inFlightIndex) const {
			DENGINE_IMPL_GFX_ASSERT(inFlightIndex <= m_inFlightCount);
			auto inFlightSize = windowUniforms.vmaAllocResultInfo.size / m_inFlightCount;
			DENGINE_IMPL_GFX_ASSERT(inFlightSize * inFlightIndex < windowUniforms.vmaAllocResultInfo.size);
			return {
				(char*)windowUniforms.vmaAllocResultInfo.pMappedData + inFlightSize * inFlightIndex,
				inFlightSize };
		}

		struct Init_Params {
			DeviceDispatch const& device;
			VmaAllocator vma;
			vk::RenderPass guiRenderPass;
			vk::DescriptorSetLayout cameraDataUniformDescrLayout;
			vk::DescriptorSetLayout objectDataUniformDescrLayout;
			vk::DescriptorSetLayout viewportImgDescrSetLayout;
			Std::Span<vk::DescriptorSetLayout const> descrSetLayouts;
			u8 inFlightCount;
			Std::AllocRef transientAlloc;
			DebugUtilsDispatch const* debugUtils;
		};
		static void Init(
			GuiResourceManager &manager,
			Init_Params const& params);

		static void PreDraw(
            GuiResourceManager &manager,
            GlobUtils const& globUtils,
            Std::Span<GuiVertex const> guiVertices,
            Std::Span<u32 const> guiIndices,
            vk::CommandBuffer cmdBuffer,
            DeletionQueue& delQueue,
            TransientAllocRef transientAlloc,
            u8 inFlightIndex);


		struct UpdateWindowUniforms_Params {
			struct WindowInfo {
				NativeWindowID windowId;
				Gfx::WindowRotation orientation;
				int resolutionWidth;
				int resolutionHeight;
			};
			DeviceDispatch const& device;
			DeletionQueue& delQueue;
			VmaAllocator& vma;
			StagingBufferAlloc& stagingBufferAlloc;
			TransientAllocRef const& transientAlloc;
			DebugUtilsDispatch const* debugUtils;
			vk::CommandBuffer stagingCmdBuffer;
			Std::RangeFnRef<WindowInfo> windowRangeRef;
			int inFlightIndex;
		};
		static void UpdateWindowUniforms(
			GuiResourceManager& manager,
			UpdateWindowUniforms_Params const& params);

		static void NewFontFace(
			GuiResourceManager &manager,
			FontFaceId id);

		static void NewFontTextures(
			GuiResourceManager &manager,
			Std::Span<FontBitmapUploadJob const> const& jobs);

		static GlyphData GetGlyphData(
			GuiResourceManager const& manager,
			FontFaceId fontFaceId,
			u32 utfValue);

		static void PerformGuiDrawCmd_Text(
			GuiResourceManager const &manager,
			DeviceDispatch const& device,
			vk::CommandBuffer cmdBuffer,
			Std::Span<vk::DescriptorSet const> descrSets,
			Std::Span<u32 const> descrDynamicOffsets,
			GuiDrawCmd::Text const& drawCmd,
			Std::Span<u32 const> utfValuesAll,
			Std::Span<GlyphRect const> glyphRectsAll,
			Math::Vec2Int posPx);

		static void RenderRectangle(
			GuiResourceManager const& manager,
			DeviceDispatch const& device,
			Std::Span<vk::DescriptorSet const> descrSet,
			Std::Span<u32 const> descrDynamicOffsets,
			vk::CommandBuffer cmdBuffer,
			GuiDrawCmd::Rectangle const& drawCmd,
			Math::Vec2Int posPx,
			Math::Vec2Int extentPx);

		static void RenderRectangleShadow(
			GuiResourceManager const& manager,
			DeviceDispatch const& device,
			Std::Span<vk::DescriptorSet const> descrSets,
			Std::Span<u32 const> descrDynamicOffsets,
			vk::CommandBuffer cmdBuffer,
			GuiDrawCmd::RectangleShadow const& drawCmd,
			Math::Vec2Int posPx,
			Math::Vec2Int extentPx);

		static void RenderGradient(
			GuiResourceManager const& manager,
			DeviceDispatch const& device,
			Std::Span<vk::DescriptorSet const> descrSets,
			Std::Span<u32 const> descrDynamicOffsets,
			vk::CommandBuffer cmdBuffer,
			GuiDrawCmd::Gradient const& drawCmd,
			Math::Vec2Int posPx,
			Math::Vec2Int extentPx);

		static void RenderRectangleStencil(
			GuiResourceManager const& manager,
			DeviceDispatch const& device,
			Std::Span<vk::DescriptorSet const> descrSets,
			Std::Span<u32 const> descrDynamicOffsets,
			vk::CommandBuffer cmdBuffer,
			GuiDrawCmd::RectangleStencil const& drawCmd,
			u8& stencilRef,
			Math::Vec2Int posPx,
			Math::Vec2Int extentPx);

		struct PerformGuiDrawCmd_Scissor_Params {
			Math::Vec2Int rectExtentPx;
			Math::Vec2Int rectPosPx;
			Gfx::WindowRotation rotation;
			int resolutionX;
			int resolutionY;
		};
		static void PerformGuiDrawCmd_Scissor(
			GuiResourceManager const &manager,
			DeviceDispatch const& device,
			vk::CommandBuffer cmdBuffer,
			PerformGuiDrawCmd_Scissor_Params const& params);

		static void PerformGuiDrawCmd_Viewport(
			GuiResourceManager const& guiResMgr,
			DeviceDispatch const& device,
			vk::CommandBuffer cmdBuffer,
			Std::Span<vk::DescriptorSet const> descrSets,
			Std::Span<u32 const> descrDynamicOffsets,
			vk::DescriptorSet viewportDescr,
			Gfx::WindowRotation rotation,
			Math::Vec2Int rectOffsetPx,
			Math::Vec2Int rectExtentPx);

		[[nodiscard]] static vk::DescriptorSet GetPerWindowDescrSet(
			GuiResourceManager const& manager,
			NativeWindowID windowIdIn,
			int inFlightIndex);
	};
}
