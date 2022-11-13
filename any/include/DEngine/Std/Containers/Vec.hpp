#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Allocator.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Span.hpp>

namespace DEngine::Std::impl { struct VecPlacementNewTag {}; }
constexpr void* operator new(
	decltype(sizeof(int)) size,
	void* data,
	[[maybe_unused]] DEngine::Std::impl::VecPlacementNewTag) noexcept
{
	return data;
}

namespace DEngine::Std
{
	// Determines whether a type can be used with Std::Vec
	template<class T>
	concept CanBeUsedForVec =
		Trait::isMoveConstructible<T> &&
	    Trait::isMoveAssignable<T> &&
		!Trait::isConst<T>;

	template<CanBeUsedForVec T, class Alloc>
	class Vec
	{
	public:
		Vec() noexcept requires (Alloc::stateless) = default;
		Vec(Vec const&) = delete;
		explicit Vec(Alloc const& alloc) noexcept : alloc{ alloc } {}

		Vec(Vec&& other) noexcept :
			data{ other.data },
			count{ other.count },
			capacity{ other.capacity },
			alloc{ other.alloc }
		{
			other.Nullify();
		}

		~Vec() noexcept {
			Clear();
		}

		Vec& operator=(Vec const&) = delete;
		Vec& operator=(Vec&& other) noexcept
		{
			Clear();
			data = other.data;
			count = other.count;
			capacity = other.capacity;
			alloc = other.alloc;
			other.Nullify();
		}

		[[nodiscard]] Alloc Allocator() const noexcept { return alloc; }

		[[nodiscard]] bool Empty() const noexcept { return count == 0; }
		void Clear() noexcept
		{
			if (data) {
				if constexpr (!Trait::isTriviallyDestructible<T>) {
					for (auto i = 0; i < count; i += 1)
						data[i].~T();
				}
				alloc.Free(data);
				Nullify();
			}
		}

		void PushBack(T const& in) noexcept requires (Trait::isCopyConstructible<T>)
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(count <= capacity);
			// First check if we have any room left
			if (count == capacity)
			{
				auto const newCapacity = count + 1;
				Grow(newCapacity);
			}
			new(data + count, impl::VecPlacementNewTag{}) T(static_cast<T const&>(in));
			count += 1;
		}

		void PushBack(T&& in) noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(count <= capacity);
			// First check if we have any room left
			if (count == capacity)
			{
				auto const newCapacity = count + 1;
				Grow(newCapacity);
			}
			new(data + count, impl::VecPlacementNewTag{}) T(static_cast<T&&>(in));
			count += 1;
		}

		bool Reserve(uSize size) noexcept {
			if (capacity < size)
				Grow(size);
			return true;
		}

		void Resize(uSize newSize) noexcept requires (Trait::isDefaultConstructible<T>)
		{
			if (newSize > capacity)
			{
				auto newCapacity = newSize;
				Grow(newCapacity);
			}

			if (newSize > count)
			{
				if constexpr (!Trait::isTriviallyDefaultConstructible<T>)
				{
					for (uSize i = count; i < newSize; i += 1)
						new(data + i, impl::VecPlacementNewTag{}) T;
				}
			}
			else if (newSize < count)
			{
				if constexpr (!Trait::isTriviallyDestructible<T>)
				{
					for (uSize i = newSize; i < count; i += 1)
						data[i].~T();
				}
			}

			count = newSize;
		}

		void EraseBack() noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(count > 0);
			if constexpr (!Trait::isTriviallyDestructible<T>)
				data[count - 1].~T();
			count -= 1;
		}

		void Erase(uSize index) noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
				index < count,
				"Attempted to .Erase() a Vec with an out-of-bounds index.");
			for (uSize i = index; i < count - 1; i += 1)
				data[i] = static_cast<T&&>(data[i + 1]);
			if constexpr (!Trait::isTriviallyDestructible<T>)
				data[count - 1].~T();
			count -= 1;
		}

		void EraseUnsorted(uSize i) noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(i < count);
			data[i] = static_cast<T&&>(data[count - 1]);
			if constexpr (!Trait::isTriviallyDestructible<T>)
				data[count - 1].~T();
			count -= 1;
		}

		[[nodiscard]] T& At(uSize i) noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(i < count);
			return data[i];
		}
		[[nodiscard]] T const& At(uSize i) const noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(i < count);
			return data[i];
		}
		[[nodiscard]] T& operator[](uSize i) noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(i < count);
			return data[i];
		}
		[[nodiscard]] T const& operator[](uSize i) const noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(i < count);
			return data[i];
		}

		[[nodiscard]] Std::Span<T> ToSpan() noexcept { return { data, count }; }
		[[nodiscard]] Std::Span<T const> ToSpan() const noexcept { return { data, count }; }

		[[nodiscard]] T* Data() noexcept { return data; }
		[[nodiscard]] T const* Data() const noexcept { return data; }
		[[nodiscard]] uSize Size() const noexcept { return count; }

		[[nodiscard]] T* begin() noexcept { return data; }
		[[nodiscard]] T const* begin() const noexcept { return data; }
		[[nodiscard]] T* end() noexcept { return data + count; }
		[[nodiscard]] T const* end() const noexcept { return data + count; }

	protected:
		T* data = nullptr;
		uSize count = 0;
		uSize capacity = 0;
		Alloc alloc;

		// Sets all memmbers to zero.
		void Nullify() {
			data = nullptr;
			count = 0;
			capacity = 0;
		}

		bool Grow(uSize newCapacity) noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(newCapacity > capacity);

			bool needNewAlloc;
			if (data)
				needNewAlloc = !alloc.Resize(data, newCapacity * sizeof(T));
			else
				needNewAlloc = true;

			if (needNewAlloc) {
				auto oldData = data;
				data = static_cast<T*>(alloc.Alloc(newCapacity * sizeof(T), alignof(T)));

				if (oldData) {
					for (uSize i = 0; i < count; i++) {
						new(data + i, impl::VecPlacementNewTag{}) T(static_cast<T&&>(oldData[i]));
						if constexpr (!Trait::isTriviallyDestructible<T>)
							oldData[i].~T();
					}
					alloc.Free(oldData);
				}
			}

			capacity = newCapacity;

			return true;
		}
	};

	template<class T, class AllocRefT>
	inline auto NewVec(AllocRefT const& alloc) { return Std::Vec<T, AllocRefT>{ alloc }; }

	template<class T, class AllocRefT>
	inline auto NewVec_WithCapacity(AllocRefT const& alloc, uSize size) {
		auto temp = Std::Vec<T, AllocRefT>{ alloc };
		temp.Reserve(size);
		return temp;
	}
}