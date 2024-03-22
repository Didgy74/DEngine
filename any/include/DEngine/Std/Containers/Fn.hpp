#pragma once

namespace DEngine::Std::impl { struct FnPlacementNewTag {}; }
template<class T>
constexpr void* operator new(decltype(sizeof(int)) size, T* data, DEngine::Std::impl::FnPlacementNewTag) noexcept { return data; }
// Having this delete operator silences a compiler warning.
[[maybe_unused]] constexpr void operator delete(void* data, DEngine::Std::impl::FnPlacementNewTag) noexcept {}

namespace DEngine::Std
{
	namespace impl
	{
		template<class Ret, class... Us>
		class Fn_IsFunctionType { static constexpr bool value = false; };

		template<class Ret, class... Us>
		class Fn_IsFunctionType<Ret(Us...)> { static constexpr bool value = true; };
	}

	template<class Alloc, class T, class... Us>
	class Fn;

	template<class Alloc, class Ret, class... Us>
	class Fn<Ret(Us...), Alloc>
	{
	public:
		explicit Fn(Alloc& alloc) :
			alloc{ &alloc } { }


		Fn(Fn&& other) :
			alloc{ other.alloc }
		{
			// If we are using external memory we don't have to do a move operation
			// on the internal storage.
			if (other.usingExternalMemory) {
				this->internal_ptr = other.internal_ptr;
				other.internal_ptr = nullptr;
			} else {
				other.moveConstrWrapperFn(this->internal_buffer, other.internal_buffer);
			}

			this->usingExternalMemory = other.usingExternalMemory;
			this->wrapperFn = other.wrapperFn;
			this->moveConstrWrapperFn = other.moveConstrWrapperFn;
			this->destructorWrapperFn = other.destructorWrapperFn;

			other.wrapperFn = nullptr;
			other.moveConstrWrapperFn = nullptr;
			other.destructorWrapperFn = nullptr;
		}

		~Fn() noexcept {
			Clear();
		}

		template<class Callable>
		void Set(Callable&& in)
		{
			if constexpr (bufferSize <= sizeof(Callable)) {
				usingExternalMemory = false;
			} else {
				internal_ptr = alloc->Alloc(sizeof(Callable), alignof(Callable));
				usingExternalMemory = true;
			}

			auto ptr = GetDataPtr();
			new(ptr, impl::FnPlacementNewTag{}) Callable(static_cast<Callable&&>(in));

			wrapperFn = [](void const* ptrIn, Us... args) -> Ret {
				return (*reinterpret_cast<Callable const*>(ptrIn))(args...);
			};
			destructorWrapperFn = [](void* ptrIn) {
				auto& callable = *reinterpret_cast<Callable*>(ptrIn);
				callable.~Callable();
			};
			moveConstrWrapperFn = [](void* aIn, void* bIn) {
				auto* a = reinterpret_cast<Callable*>(aIn);
				auto* b = reinterpret_cast<Callable*>(bIn);
				new(a, impl::FnPlacementNewTag{}) Callable(Std::Move(*b));
			};
		}

		Ret Invoke(Us... args) const {
			auto dataPtr = GetDataPtr();
			DENGINE_IMPL_CONTAINERS_ASSERT(dataPtr);
			DENGINE_IMPL_CONTAINERS_ASSERT(this->wrapperFn);
			return this->wrapperFn(dataPtr, args...);
		}

	private:
		void Clear() {
			auto ptr = this->GetDataPtr();
			DENGINE_IMPL_ASSERT(this->destructorWrapperFn);
			this->destructorWrapperFn(ptr);

			if (usingExternalMemory && ptr) {
				alloc->Free(ptr);
			}

			if (this->usingExternalMemory) {
				this->internal_ptr = nullptr;
			} else {
				this->internal_buffer = {};
			}
		}

		Alloc* alloc = nullptr;
		bool usingExternalMemory = false;
		static constexpr int bufferSize = 16;
		[[nodiscard]] auto const* GetDataPtr() const {
			if (usingExternalMemory)
				return this->internal_ptr;
			else
				return this->internal_buffer;
		}
		union {
			void* internal_ptr;
			char internal_buffer[bufferSize];
		};

		using FnWrapperT = Ret(*)(void const*, Us...);
		FnWrapperT wrapperFn = nullptr;
		using DestructorWrapperT = void(*)(void*);
		DestructorWrapperT destructorWrapperFn = nullptr;

		using MoveConstructWrapperFnT = void(*)(void const* a, void const* b);
		MoveConstructWrapperFnT moveConstrWrapperFn = nullptr;
	};

	template<class T, class Alloc>
	Fn<T, Alloc> MakeFn(Alloc& alloc)
	{
		return Fn<T, Alloc>(alloc);
	}
}