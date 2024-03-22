#pragma once

#include <DEngine/Std/Containers/impl/Assert.hpp>

namespace DEngine::Std
{
	template<class ReturnT, class ...ArgsT>
	class FnRef;

	template<class ReturnT, class ...ArgsT>
	class FnRef<ReturnT(ArgsT...)>
	{
	public:
		template<class Callable>
		FnRef(Callable const& in)
		{
			actualFn = &in;
			wrapperFn = [](void const* actualFnIn, ArgsT... argsIn) {
				auto const& fn = *static_cast<Callable const*>(actualFnIn);
				return fn(argsIn...);
			};
		}

		ReturnT Invoke(ArgsT... args) const
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(wrapperFn);
			DENGINE_IMPL_CONTAINERS_ASSERT(actualFn);
			return wrapperFn(actualFn, args...);
		}

		ReturnT operator()(ArgsT... args) const {
			return Invoke(args...);
		}

	private:
		void const* actualFn = nullptr;
		using WrapperFnT = ReturnT(*)(void const*, ArgsT...);
		WrapperFnT wrapperFn = nullptr;
	};

}