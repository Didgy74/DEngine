#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Common.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Utility.hpp>

#include <vector>

namespace DEngine::Std {
	namespace impl {
		struct FnScratchList_MemBlock {
			void* data = nullptr;
			uSize capacity = 0;
			uSize offset = 0;
		};
	}

	template<class... ArgsT>
	class FnScratchList {
	public:
		FnScratchList() = default;
		FnScratchList(FnScratchList const&) = delete;
		FnScratchList(FnScratchList&&) = delete;
		FnScratchList& operator=(FnScratchList const&) = delete;
		FnScratchList& operator=(FnScratchList&&) = delete;

		template<class Callable> requires (!Trait::isRef<Callable>)
		void Push(Callable&& in) noexcept {
			using Test = FnItemImpl<Callable>;
			auto ptr = Allocate(sizeof(Test), alignof(Test));
			auto test = new(ptr) Test(Std::Move(in));
			functions.push_back(test);
		}

		template<class Callable>
		void operator+=(Callable&& in) requires (!Trait::isRef<Callable>) {
			Push(Std::Move(in));
		}

		void Consume(ArgsT... args) {
			for (auto const& fn : functions)
				fn->Invoke(args...);
			ClearMinimal();
		}

		[[nodiscard]] bool IsEmpty() const noexcept { return functions.empty(); }

		~FnScratchList() {
			ClearMinimal();
		}

	private:
		static constexpr uSize minimumMemBlockSize = 256;

		void ClearFunctions() {
			for (auto const& fn : functions)
				fn->~FnItem();
			functions.clear();
		}

		void ClearMinimal() {
			ClearFunctions();
			currentMemBlock.offset = 0;
			for (auto& block : prevMemBlocks) {
				if (block.data != nullptr)
					free(block.data);
			}
			prevMemBlocks.clear();
		}

		static auto GetAlignedOffset(
			void const* ptr,
			uSize offset,
			uSize alignment) noexcept
		{
			u64 const asInt = (uintptr_t)ptr + offset;
			auto const alignedAsInt = Math::CeilToMultiple(asInt, (u64)alignment);
			DENGINE_IMPL_CONTAINERS_ASSERT((alignedAsInt % alignment) == 0);
			auto const alignedOffset = offset + alignedAsInt - asInt;
			return alignedOffset;
		}
		void* Allocate(uSize size, uSize align) noexcept {
			if (currentMemBlock.data == nullptr) {
				auto sizeToMalloc = Math::Max(size, minimumMemBlockSize);
				currentMemBlock.data = malloc(sizeToMalloc);
				currentMemBlock.capacity = sizeToMalloc;
			}

			auto const alignedOffset = GetAlignedOffset(
				currentMemBlock.data,
				currentMemBlock.offset,
				align);

			if (alignedOffset + size >= currentMemBlock.capacity) {
				prevMemBlocks.push_back(currentMemBlock);
				// Alloc more space
				auto sizeToMalloc = Math::Max(size, currentMemBlock.capacity * 2);
				currentMemBlock = {};
				currentMemBlock.data = malloc(sizeToMalloc);
				currentMemBlock.capacity = sizeToMalloc;
			}

			auto returnValue = (void*)((char*)currentMemBlock.data + alignedOffset);
			currentMemBlock.offset = alignedOffset + size;
			return returnValue;
		}

		struct FnItem {
			virtual void Invoke(ArgsT...) const = 0;
			virtual ~FnItem() {}
		};
		template<class Callable> requires (!Trait::isRef<Callable>)
		struct FnItemImpl : public FnItem {
			Callable item;
			explicit FnItemImpl(Callable&& in) : item{ Std::Move(in) } {}
			virtual void Invoke(ArgsT... args) const override {
				item(args...);
			}
			virtual ~FnItemImpl() = default;
		};
		std::vector<FnItem*> functions;

		using MemBlock = impl::FnScratchList_MemBlock;
		MemBlock currentMemBlock = {};
		std::vector<MemBlock> prevMemBlocks;
	};
}