#pragma once

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Containers/Span.hpp>
#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/Vector.hpp>

#include <unordered_map>

namespace DEngine::Gfx
{
	struct GuiVertex;
}

namespace DEngine::Gfx::Vk
{
	class DebugUtilsDispatch;
	class DeviceDispatch;
	class GlobUtils;
	
	class GuiResourceManager
	{
	public:
		static constexpr uSize minVertexCapacity = 4096;
		vk::Buffer vertexBuffer{};
		VmaAllocation vertexVmaAlloc{};
		uSize vertexCapacity{};
		void* vertexBufferData = nullptr;

		static constexpr uSize minIndexCapacity = 4096;
		vk::Buffer indexBuffer{};
		VmaAllocation indexVmaAlloc{};
		uSize indexCapacity{};
		void* indexBufferData = nullptr;

		struct FilledMeshPushConstant
		{
			Math::Mat2 orientation;
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
			Math::Vec4 color;
		};
		vk::Pipeline filledMeshPipeline{};
		vk::PipelineLayout filledMeshPipelineLayout{};


		struct GlyphData
		{
			vk::Image img{};
			vk::ImageView imgView{};
			vk::DescriptorSet descrSet{};
		};
		std::unordered_map<u32, GlyphData> glyphDatas;
		struct FontPushConstant
		{
			Math::Mat2 orientation;
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
			Math::Vec4 color;
		};
		vk::DescriptorPool font_descrPool{};
		vk::DescriptorSetLayout font_descrSetLayout{};
		vk::Sampler font_sampler{};
		vk::PipelineLayout font_pipelineLayout{};
		vk::Pipeline font_pipeline{};

		struct ViewportPushConstant
		{
			Math::Mat2 orientation;
			Math::Vec2 rectOffset;
			Math::Vec2 rectExtent;
		};
		vk::Pipeline viewportPipeline{};
		vk::PipelineLayout viewportPipelineLayout{};
		vk::DescriptorSetLayout viewportDescrSetLayout{};
		vk::DescriptorPool viewportDescrPool{};

		static void Init(
			GuiResourceManager& manager,
			DeviceDispatch const& device,
			VmaAllocator vma,
			u8 inFlightCount,
			vk::RenderPass guiRenderPass,
			DebugUtilsDispatch const* debugUtils);

		static void Update(
			GuiResourceManager& manager,
			GlobUtils const& globUtils,
			Std::Span<GuiVertex const> guiVertices,
			Std::Span<u32 const> guiIndices,
			u8 inFlightIndex);
		
		static void NewFontTexture(
			GuiResourceManager& manager,
			GlobUtils const& globUtils,
			u32 id,
			u32 width,
			u32 height,
			u32 pitch,
			Std::Span<std::byte const> data);

	};
}
