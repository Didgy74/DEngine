#pragma once

#include <DEngine/Math/Matrix.hpp>

#include "DeletionQueue.hpp"
#include "ForwardDeclarations.hpp"
#include "GuiResourceManager.hpp"
#include "RaiiHandles.hpp"
#include "VMAIncluder.hpp"
#include "VulkanIncluder.hpp"

namespace DEngine::Gfx::Vk
{
	struct ObjectDataManager {
		// The type of uniforms that described by the descriptors for ObjectData uniforms
		static constexpr vk::DescriptorType objectDataUniformDescrType = vk::DescriptorType::eUniformBufferDynamic;

		// Contains minimum capacity of amount of elements.
		// Not measured in bytes.
		static constexpr uSize minObjectDataCapacity = 256;
		static constexpr uSize minGuiPlanesCapacity = 4;

		// Contains the capacity of amount of elements.
		// This is not amount in bytes.
		int inFlightCount = 0;
		// Should only be set once during init.
		uSize minUniformBufferOffsetAlignment = 0;
		// Owned by GuiResourceManager.
		vk::DescriptorSetLayout guiWindowDescrLayout;
		BoxVkDescriptorSetLayout objectDataDescrLayout = {};

		struct ObjectDataUniform {
			Math::Mat4 transform;
		};
		// Measured in bytes.
		static constexpr uSize minElementSize = 64;

		// In bytes, the stride between two items in our uniform buffer
		[[nodiscard]] uSize ObjectDataUniformElementAlignment() const {
			DENGINE_IMPL_GFX_ASSERT(minUniformBufferOffsetAlignment != 0);
			return Std::CeilToMultiple(
				(u64)sizeof(ObjectDataUniform),
				(u64)minUniformBufferOffsetAlignment);
		}

		struct ObjectDataResources {
			VmaAllocationInfo allocInfo = {};
			BoxVmaBuffer buffer = {};

			BoxVkDescriptorPool descrPool = {};
			vk::DescriptorSet descrSet = {};

			[[nodiscard]] Std::ByteSpan MappedMemory() const {
				return { (char*)allocInfo.pMappedData, allocInfo.size };
			}

			[[nodiscard]] uSize CapacityCount(ObjectDataManager const& mgr) const {
				// Note that size can be zero in the case where we have not yet allocated anything.
				return allocInfo.size / mgr.ObjectDataUniformElementAlignment() / mgr.inFlightCount;
			}

			void QueueDeletion(DeletionQueue& delQueue) {
				DENGINE_IMPL_GFX_ASSERT(!buffer.IsNull());
				DENGINE_IMPL_GFX_ASSERT(!descrPool.IsNull());
				delQueue.Destroy(Std::Move(buffer));
				delQueue.Destroy(Std::Move(descrPool));
				this->allocInfo = {};
				this->descrSet = vk::DescriptorSet{};
			}
		};

		ObjectDataResources objectDataResources;
		[[nodiscard]] uSize ObjectDataCapacity() const { return objectDataResources.CapacityCount(*this); }
		[[nodiscard]] vk::DescriptorSetLayout GetObjectDataUniformDescrLayout() const {
			DENGINE_IMPL_GFX_ASSERT(!objectDataDescrLayout.IsNull());
			return objectDataDescrLayout.Handle();
		}
		[[nodiscard]] vk::DescriptorSet GetObjectDataUniformDescrSet() const {
			DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(objectDataResources.descrSet));
			return objectDataResources.descrSet;
		}
		[[nodiscard]] uSize ObjectDataCapacityCount() const { return objectDataResources.CapacityCount(*this); }
		// This gets you the offset from the start of the buffer, into the specific item you want.
		[[nodiscard]] u32 GetObjectDataUniformBufferOffset(uSize inFlightIndex, uSize index) const {
			DENGINE_IMPL_GFX_ASSERT(inFlightIndex < inFlightCount);
			DENGINE_IMPL_GFX_ASSERT(index < ObjectDataCapacityCount());
			u32 offset = 0;
			// Offset into correct into correct in-flight set.
			offset += ObjectDataCapacityCount() * ObjectDataUniformElementAlignment() * inFlightIndex;
			// Offset into specific item.
			offset += ObjectDataUniformElementAlignment() * index;
			return offset;
		}

		using GuiPlaneUniformStruct = GuiResourceManager::WindowShaderUniforms::PerWindowUniform;

		struct GuiPlaneResources {
			// The element-capacity (per in-flight set) that all the resources below were
			// allocated for. This is the single source of truth for the GuiPlane layout:
			// the window buffer, the object-data buffer and the descriptor-set array are
			// all sized from this exact value. Do NOT derive capacity from a VmaAllocationInfo
			// size, as VMA may round the backing allocation up above what we requested, which
			// would desync the descriptor-set indexing and the offset math.
			int capacity = 0;

			// Holds ObjectDataUniforms (one per plane, per in-flight frame)
			VmaAllocationInfo objectDataAllocInfo = {};
			BoxVmaBuffer objectDataBuffer = {};
			BoxVkDescriptorPool objectDataDescrPool = {};
			vk::DescriptorSet objectDataDescrSet = {};

			// Holds GuiPlaneUniformStruct (one per plane, per in-flight frame)
			VmaAllocationInfo windowAllocInfo = {};
			BoxVmaBuffer windowBuffer = {};

			BoxVkDescriptorPool windowDataDescrPool = {};
			// Vector of descriptor sets - one per plane instance per in-flight frame
			std::vector<vk::DescriptorSet> windowDataDescrSets;

			[[nodiscard]] Std::ByteSpan ObjectDataMappedMemory() const {
				DENGINE_IMPL_GFX_ASSERT(objectDataAllocInfo.pMappedData != nullptr);
				return { (char*)objectDataAllocInfo.pMappedData, objectDataAllocInfo.size };
			}

			[[nodiscard]] static uSize WindowDataUniformAlignment(uSize minUniformBufferOffsetAlignment) {
				DENGINE_IMPL_GFX_ASSERT(minUniformBufferOffsetAlignment != 0);
				return GuiResourceManager::WindowShaderUniforms::UniformElementAlignment(
					minUniformBufferOffsetAlignment);
			}
			[[nodiscard]] Std::ByteSpan WindowDataMappedMemory() const {
				DENGINE_IMPL_GFX_ASSERT(windowAllocInfo.pMappedData != nullptr);
				return { (char*)windowAllocInfo.pMappedData, windowAllocInfo.size };
			}
			[[nodiscard]] Std::ByteSpan WindowDataMappedMemory_InFlighSet(int inFlightCount, int inFlightIndex) const {
				DENGINE_IMPL_GFX_ASSERT(inFlightCount > 0);
				DENGINE_IMPL_GFX_ASSERT(inFlightIndex >= 0);
				DENGINE_IMPL_GFX_ASSERT(inFlightIndex < inFlightCount);
				DENGINE_IMPL_GFX_ASSERT(windowAllocInfo.pMappedData != nullptr);
				return {
					(char*)windowAllocInfo.pMappedData + inFlightIndex * windowAllocInfo.size / inFlightCount,
					windowAllocInfo.size / inFlightCount };
			}

			void QueueDeletion(DeletionQueue& delQueue) {
				DENGINE_IMPL_GFX_ASSERT(!objectDataBuffer.IsNull());
				DENGINE_IMPL_GFX_ASSERT(!objectDataDescrPool.IsNull());
				DENGINE_IMPL_GFX_ASSERT(!windowBuffer.IsNull());
				delQueue.Destroy(Std::Move(objectDataBuffer));
				delQueue.Destroy(Std::Move(objectDataDescrPool));
				delQueue.Destroy(Std::Move(windowBuffer));
				delQueue.Destroy(Std::Move(windowDataDescrPool));

				this->capacity = 0;
				this->objectDataAllocInfo = {};
				this->objectDataDescrSet = vk::DescriptorSet{};
				this->windowDataDescrSets.clear();
				this->windowAllocInfo = {};
			}
		};

		GuiPlaneResources guiPlaneResources;
		[[nodiscard]] uSize GuiPlaneWindowDataUniformAlignment() const {
			return GuiPlaneResources::WindowDataUniformAlignment(minUniformBufferOffsetAlignment);
		}
		[[nodiscard]] uSize GetGuiPlaneCapacity() const {
			// This is the capacity all GuiPlane resources were allocated for, including the
			// window descriptor-set array and the window buffer. It is deliberately NOT
			// derived from a VMA allocation size; see GuiPlaneResources::capacity.
			return guiPlaneResources.capacity;
		}
		[[nodiscard]] Std::ByteSpan GetGuiPlaneWindowDataUniformMappedMemory() const {
			return guiPlaneResources.WindowDataMappedMemory();
		}
		// Returns the offset, in bytes, from the start of the buffer, to the given in-flight resource-set
		// For the WindowData uniforms
		[[nodiscard]] uSize GetGuiPlaneWindowDataUniformInFlightOffset(int inFlightIndex) const {
			DENGINE_IMPL_GFX_ASSERT(inFlightIndex < inFlightCount);
			auto elementAlignment = GuiPlaneWindowDataUniformAlignment();
			auto capacity = GetGuiPlaneCapacity();
			return capacity * elementAlignment * inFlightIndex;
		}
		// Returns the size of one in
		[[nodiscard]] vk::DescriptorSet GetGuiPlaneObjectDataUniformDescrSet() const {
			DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(guiPlaneResources.objectDataDescrSet));
			return guiPlaneResources.objectDataDescrSet;
		}

		[[nodiscard]] u32 GetGuiPlaneObjectDataUniformOffset(int inFlightIndex, int index) const {
			DENGINE_IMPL_GFX_ASSERT(inFlightIndex < inFlightCount);
			u32 offset = 0;
			auto capacity = GetGuiPlaneCapacity();
			DENGINE_IMPL_GFX_ASSERT(index < capacity);
			auto elementAlignment = ObjectDataUniformElementAlignment();
			// Offset into correct into correct in-flight set.
			offset += capacity * elementAlignment * inFlightIndex;
			// Offset into specific item.
			offset += elementAlignment * index;
			return offset;
		}

		static void Update(
			ObjectDataManager& manager,
			DeviceDispatch const& device,
			VmaAllocator vma,
			Std::Span<Math::Mat4 const> transforms,
			Std::Span<SceneGuiPlane const> guiPlanes,
			vk::CommandBuffer cmdBuffer,
			DeletionQueue& delQueue,
			u8 inFlightIndex,
			TransientAllocRef transientAlloc,
			DebugUtilsDispatch const* debugUtils);

		[[nodiscard]] static bool Init(
			ObjectDataManager& manager,
			DeviceDispatch const& device,
			uSize inFlightCount,
			vk::DescriptorSetLayout guiWindowDescrLayout,
			DebugUtilsDispatch const* debugUtils);

		[[nodiscard]] static vk::DescriptorSetLayoutBinding DescrLayoutBinding();

		[[nodiscard]] vk::DescriptorSet GetGuiPlaneDescrSet(u8 inFlightIndex, uSize planeIndex) const {
			DENGINE_IMPL_GFX_ASSERT(inFlightIndex < inFlightCount);
			DENGINE_IMPL_GFX_ASSERT(planeIndex < GetGuiPlaneCapacity());
			auto descrSetIndex = inFlightIndex * GetGuiPlaneCapacity() + planeIndex;
			return guiPlaneResources.windowDataDescrSets[descrSetIndex];
		}
	};
}