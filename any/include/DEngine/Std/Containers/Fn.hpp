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
	class Fn
	{ };

	template<class Alloc, class Ret, class... Us>
	class Fn<Ret(Us...), Alloc>
	{
	public:
		explicit Fn(Alloc& alloc) :
			alloc{ &alloc }
		{ }

		Fn(Fn&& other) :
			alloc{ other.alloc }
		{

		}

		~Fn() noexcept
		{
			if (usingExternalMemory && ptr)
			{
				alloc->Free(ptr);
			}
		}

		template<class Callable>
		void Set(Callable&& in)
		{
			void* tempPtr;
			if constexpr (bufferSize <= sizeof(Callable))
			{
				tempPtr = buffer;
				usingExternalMemory = false;
			}
			else
			{
				tempPtr = alloc->Alloc(sizeof(Callable), alignof(Callable));
				usingExternalMemory = true;
			}
			ptr = tempPtr;
			new(ptr, impl::FnPlacementNewTag{}) Callable(static_cast<Callable&&>(in));

			invokeFn = [](void const* ptrIn, Us... args) -> Ret {
				return (*reinterpret_cast<Callable const*>(ptrIn))(args...);
			};
		}

		Ret Invoke(Us... args) const
		{
			void const* tempPtr;
			if (usingExternalMemory)
				tempPtr = ptr;
			else
				tempPtr = buffer;
			DENGINE_IMPL_CONTAINERS_ASSERT(invokeFn);
			return invokeFn(tempPtr, args...);
		}

	private:
		Alloc* alloc = nullptr;
		bool usingExternalMemory = false;
		static constexpr int bufferSize = sizeof(void*) * 2;
		union
		{
			void* ptr;
			char buffer[bufferSize];
		};

		Ret(*invokeFn)(void const* ptr, Us...) = nullptr;
	};

	template<class T, class Alloc>
	Fn<T, Alloc> MakeFn(Alloc& alloc)
	{
		return Fn<T, Alloc>(alloc);
	}
}