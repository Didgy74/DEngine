#pragma once

namespace DEngine::Std::Trait::impl
{
	template<bool valueIn>
	struct BoolValue { static constexpr bool value = valueIn; };

	template<typename T, typename... Ts>
	struct Front { using Type = T; };

	template<bool expr, typename T, typename U>
	struct Conditional;
	template<typename T, typename U>
	struct Conditional<true, T, U> { using Type = T; };
	template<typename T, typename U>
	struct Conditional<false, T, U> { using Type = U; };

	template<typename T>
	struct IsConst : public BoolValue<false> {};
	template<typename T>
	struct IsConst<T const> : public BoolValue<true> {};

	template<typename T, typename U>
	struct IsSame : public BoolValue<false> {};
	template<typename T>
	struct IsSame<T, T> : public BoolValue<true> {};

	template<typename T>
	struct RemoveConst { using Type = T; };
	template<typename T>
	struct RemoveConst<T const> { using Type = T; };

	template<typename T>
	struct RemoveRef { using Type = T; };
	template<typename T>
	struct RemoveRef<T&> { using Type = T; };
	template<typename T>
	struct RemoveRef<T&&> { using Type = T; };

	template<typename T>
	struct RemoveCVRef { using Type = typename RemoveConst<typename RemoveRef<T>::Type>::Type; };

}

namespace DEngine::Std::Trait
{
	template<bool expr, typename T, typename U>
	using Conditional = typename impl::Conditional<expr, T, U>::Type;
	template<bool expr, typename T, typename U>
	using Cond = Conditional<expr, T, U>;

	template<typename... Ts>
	using Front = typename impl::Front<Ts...>::Type;

	template<typename T>
	constexpr bool IsConst = impl::IsConst<T>::value;

	template<typename T, typename U>
	constexpr bool IsSame = impl::IsSame<T, U>::value;

	template<typename T>
	using RemoveConst = typename impl::RemoveConst<T>::Type;
	
	template<typename T>
	using RemoveCVRef = typename impl::RemoveCVRef<T>::Type;

	template<typename T>
	using RemoveRef = typename impl::RemoveRef<T>::Type;

}