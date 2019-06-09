#pragma once

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace DRenderer::Vulkan
{
	struct DeletionQueues
	{
		template<typename T>
		using QueueType = std::vector<T>;

		template<typename T>
		using QueueSetType = std::vector<QueueType<T>>;

		struct QueueTable
		{
			void(*objectDeletionPfn)(vk::Device, uint8_t, void*) = nullptr;
			void(*queueDestructorPfn)(void*&) = nullptr;
			void* queueSets = nullptr;
		};

		vk::Device device{};
		uint8_t queueDelayLength = 3;
		uint8_t currentFrame = 0;
		std::unordered_map<std::type_index, QueueTable> hashmap;

		inline void CleanupCurrentFrame()
		{
			for (auto elementIterator : hashmap)
			{
				QueueTable& element = elementIterator.second;
				element.objectDeletionPfn(device, currentFrame, element.queueSets);
			}

			currentFrame = (currentFrame + uint8_t(1)) % queueDelayLength;
		}

		template<typename T>
		inline void InsertObjectForDeletion(T object)
		{
			QueueSetType<T>* queueSetsRef = nullptr;

			auto iterator = hashmap.find(typeid(T));
			if (iterator == hashmap.end())
			{
				auto iterator = hashmap.insert({ typeid(T), {} });

				QueueTable& table = iterator.first->second;

				table.objectDeletionPfn = &ObjectDeleter<T>;

				queueSetsRef = new QueueSetType<T>();
				assert(queueDelayLength > 0);
				queueSetsRef->resize(queueDelayLength);
				for (QueueType<T>& queue : *queueSetsRef)
					queue.reserve(10);

				table.queueSets = queueSetsRef;
			}
			else
				queueSetsRef = reinterpret_cast<QueueSetType<T>*>(iterator->second.queueSets);

			assert(queueSetsRef != nullptr);

			QueueSetType<T>& queueSets = *queueSetsRef;

			size_t queueIndexToPushback = (currentFrame - 1 + queueDelayLength) % queueDelayLength;
			queueSets[queueIndexToPushback].push_back(object);
		}

	private:
		template<typename T>
		static inline void ObjectDeleter(vk::Device device, uint8_t queueSetIndex, void* queueSetsPtr)
		{
			QueueSetType<T>& queueSets = *reinterpret_cast<QueueSetType<T>*>(queueSetsPtr);

			QueueType<T>& queue = queueSets[queueSetIndex];

			for (T element : queue)
			{
				if constexpr (std::is_same_v<T, vk::DeviceMemory>)
					device.freeMemory(element);
				else
					device.destroy(element);
			}

			queue.clear();
		}

		template<typename T>
		static inline void QueueDeleter(void*& ptr)
		{
			delete static_cast<QueueSetType<T>*>(ptr);
			ptr = nullptr;
		}
	};
}