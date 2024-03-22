#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Common.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Utility.hpp>

#include <vector>
#include <stdlib.h>

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
		struct FnItem {
			virtual void Invoke(ArgsT...) const = 0;
			virtual ~FnItem() = default;
		};

		FnScratchList() = default;
		FnScratchList(FnScratchList const&) = delete;
		FnScratchList(FnScratchList&& other) noexcept {
			this->prevMemBlocks = Std::Move(other.prevMemBlocks);
			this->currentMemBlock = Std::Move(other.currentMemBlock);
			other.currentMemBlock = {};
			this->functions = Std::Move(other.functions);
		}
		FnScratchList& operator=(FnScratchList const&) = delete;
		FnScratchList& operator=(FnScratchList&& other) noexcept {
			if (this == &other)
				return *this;

			this->Clear();
			this->prevMemBlocks = Std::Move(other.prevMemBlocks);
			this->currentMemBlock = Std::Move(other.currentMemBlock);
			other.currentMemBlock = {};
			this->functions = Std::Move(other.functions);
			return *this;
		}

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
			Clear();
		}

		[[nodiscard]] auto const& At(int i) const { return *functions[i]; }
		[[nodiscard]] auto Size() const noexcept { return (int)functions.size(); }
		[[nodiscard]] auto IsEmpty() const noexcept { return functions.empty(); }

		void Clear() {
			ResetFunctions();
			currentMemBlock.offset = 0;
			for (auto& block : prevMemBlocks) {
				DENGINE_IMPL_ASSERT(block.data != nullptr);
				free(block.data);
			}
			prevMemBlocks.clear();
		}

		~FnScratchList() {
			Clear();
			// This could be a nullptr if the container has been moved from,
			// or never used.
			if (currentMemBlock.data != nullptr)
				free(currentMemBlock.data);
		}

	private:
		static constexpr uSize minimumMemBlockSize = 256;

		void ResetFunctions() {
			for (auto const& fn : functions)
				fn->~FnItem();
			functions.clear();
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

			auto alignedOffset = GetAlignedOffset(
				currentMemBlock.data,
				currentMemBlock.offset,
				align);

			if (alignedOffset + size >= currentMemBlock.capacity) {
				prevMemBlocks.push_back(currentMemBlock);
				// Alloc more space
				auto sizeToMalloc = Math::Max(size, currentMemBlock.capacity) * 2;
				currentMemBlock = {};
				currentMemBlock.data = malloc(sizeToMalloc);
				currentMemBlock.capacity = sizeToMalloc;

				alignedOffset = 0;
			}

			auto returnValue = (void*)((char*)currentMemBlock.data + alignedOffset);
			currentMemBlock.offset = alignedOffset + size;
			return returnValue;
		}

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