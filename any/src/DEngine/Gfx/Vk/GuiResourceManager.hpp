#pragma once

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"
#include "ForwardDeclarations.hpp"
#include "TransientAllocRef.hpp"
#include "NativeWindowManager.hpp"
#include "StagingBufferAlloc.hpp"

#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/AllocRef.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/RangeFnRef.hpp>
#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/Vector.hpp>

#include <DEngine/Gfx/Gfx.hpp>

#include <unordered_map>
#include <mutex>

namespace DEngine::Gfx {
	struct GuiVertex;
}

namespace DEngine::Gfx::Vk
{
	class GuiResourceManager {
	public:
		static constexpr uSize minVtxCapacity = 2048;
		vk::Buffer vtxBuffer{};
		VmaAllocation vtxVmaAlloc{};
		Std::Span<u8> vtxMappedMem;
		uSize vtxInFlightCapacity = 0;

		static constexpr uSize minIndexCapacity = 2048;
		vk::Buffer indexBuffer{};
		VmaAllocation indexVmaAlloc{};
		Std::Span<u8> indexMappedMem;
		uSize indexInFlightCapacity = 0;

		// Contains the per-window specific uniforms.
		// We store a linear buffer that contains N amount of per-window structs in memory, where N is the amount
		// of windows in existence.
		// We need to duplicate this buffer so that we have `inFlighCount` amount of buffers.
		// We update all current in-flight buffer every frame.
		struct WindowShaderUniforms {
			struct PerWindowUniform {
				int orientation = 0;
				char padding0[12] = {};
				Math::Vec2Int resolution = {};
				static constexpr int sizeInBytes = 24;
			};

			vk::Buffer buffer = {};
			VmaAllocation vmaAlloc = {};
			VmaAllocationInfo vmaAllocResultInfo = {};
			Std::ByteSpan mappedMem = {};
			int currentCapacity = 0;
			// Each element has to be aligned by minUniformBufferAlignment,
			// so this tracks how big each uniform element actually is.
			int bufferElementSize = 0;
			vk::DescriptorSetLayout setLayout = {};
			vk::DescriptorPool descrPool = {};
			// This needs to have length equal to total length of our buffer.
			// length = currentCapacity * inFlightCount
			std::vector<vk::DescriptorSet> windowUniformDescrSets;

			// Gets the offset into a specific in-flight set of the total buffer.
			[[nodiscard]] int Buffer_InFlightOffset(int inFlightIndex) const {
				return Buffer_InFlightSize() * inFlightIndex;
			}
			[[nodiscard]] int Buffer_InFlightSize() const {
				return currentCapacity * bufferElementSize;
			}

			[[nodiscard]] Std::ByteSpan InFlightBufferSpan(int inFlightIndex) const {
				return {
					mappedMem.Data() + Buffer_InFlightOffset(inFlightIndex),
					(uSize)Buffer_InFlightSize() };
			}


			// The WindowIDs are ordered the same way they are ordered in memory.
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
		vk::Pipeline filledMeshPipeline{};
		vk::PipelineLayout filledMeshPipelineLayout{};

		struct RectanglePushConstant {
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
			Math::Vec4 color;
			Math::Vec4 radius;
			static constexpr int sizeInBytes = 48;
		};
		vk::Pipeline rectanglePipeline{};
		vk::PipelineLayout rectanglePipelineLayout{};
		
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
			vk::Image img{};
			VmaAllocation imgAlloc{};
			vk::ImageView imgView{};
			vk::DescriptorSet descrSet{};
		};

		struct FontFace {
			std::unordered_map<u32, GlyphData> glyphDatas;
			static constexpr uSize lowUtfGlyphDatasSize = 128;
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
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
			Math::Vec4 color;
		};
		vk::DescriptorPool font_descrPool{};
		vk::DescriptorSetLayout font_descrSetLayout{};
		vk::Sampler font_sampler{};
		vk::PipelineLayout font_pipelineLayout{};
		vk::Pipeline font_pipeline{};

		struct ViewportPushConstant {
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
		};
		vk::Pipeline viewportPipeline{};
		vk::PipelineLayout viewportPipelineLayout{};
		vk::DescriptorSetLayout viewportDescrSetLayout{};
		vk::DescriptorPool viewportDescrPool{};

		struct Init_Params {
			DeviceDispatch const& device;
			VmaAllocator vma;
			vk::RenderPass guiRenderPass;
			vk::DescriptorSetLayout viewportImgDescrLayout;
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
			Std::RangeFnRef<WindowInfo> windowRange;
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
			vk::DescriptorSet perWindowDescrSet,
			GuiDrawCmd::Text const& drawCmd,
			Std::Span<u32 const> utfValuesAll,
			Std::Span<GlyphRect const> glyphRectsAll);

		static void RenderRectangle(
			GuiResourceManager const& manager,
			DeviceDispatch const& device,
			vk::DescriptorSet descrSet,
			vk::CommandBuffer cmdBuffer,
			GuiDrawCmd::Rectangle const& drawCmd);

		struct PerformGuiDrawCmd_Scissor_Params {
			Math::Vec2 rectExtent;
			Math::Vec2 rectPos;
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
			vk::DescriptorSet perWindowDescr,
			vk::DescriptorSet viewportDescr,
			Gfx::WindowRotation rotation,
			Math::Vec2 rectOffset,
			Math::Vec2 rectExtent);

		[[nodiscard]] static vk::DescriptorSet GetPerWindowDescrSet(
			GuiResourceManager const& manager,
			NativeWindowID windowIdIn,
			int inFlightIndex);
	};
}
