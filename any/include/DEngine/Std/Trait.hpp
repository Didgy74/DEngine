#pragma once

namespace DEngine::Std::Trait::impl
{
	template<unsigned int index, class T, class...Us>
	struct At_Impl;
	template<class T, class... Us>
	struct At_Impl<0, T, Us...> { using Type = T; };
	template<unsigned int index, class T, class... Us>
	struct At_Impl { using Type = typename At_Impl<index - 1, Us...>::Type; };
	// This is just a wrapper to add the static_assert of the index against the parameter pack length.
	template<unsigned int index, class... Ts>
	struct At
	{
		static_assert(index < sizeof...(Ts), "Tried to index further than the parameter pack.");
		using Type = typename At_Impl<index, Ts...>::Type;
	};

	template<unsigned int index, unsigned int missingTypeCount, class Or, class... Ts>
	struct AtOr;
	template<unsigned int index, class Or, class... Ts>
	struct AtOr<index, 0, Or, Ts...> { using Type = typename At<index, Ts...>::Type; };
	template<unsigned int index, unsigned int missingTypeCount, class Or, class... Ts>
	struct AtOr { using Type = typename AtOr<index, missingTypeCount - 1, Or, Ts..., Or>::Type; };

	template<bool valueIn>
	struct BoolValue { static constexpr bool value = valueIn; };

	template<class T, class... Ts>
	struct Front { using Type = T; };

	template<bool expr, class T, class U>
	struct Conditional;
	template<class T, class U>
	struct Conditional<true, T, U> { using Type = T; };
	template<class T, class U>
	struct Conditional<false, T, U> { using Type = U; };

	template <class T, class... Ts>
	struct IndexOf;
	template <class T, class... Ts>
	struct IndexOf<T, T, Ts...> { static constexpr unsigned int value = 0; };
	template <class T, class U, class... Ts>
	struct IndexOf<T, U, Ts...> { static constexpr unsigned int value = 1 + IndexOf<T, Ts...>::value; };
	
	template<class T>
	struct IsConst : public BoolValue<false> {};
	template<class T>
	struct IsConst<T const> : public BoolValue<true> {};

	template<class T>
	struct IsRef : public BoolValue<false> {};
	template<class T>
	struct IsRef<T&> : public BoolValue<true> {};
	template<class T>
	struct IsRef<T&&> : public BoolValue<true> {};

	template<class T>
	struct IsRValueRef : public BoolValue<false> {};
	template<class T>
	struct IsRValueRef<T&&> : public BoolValue<true> {};

	template<class T, class U>
	struct IsSame : public BoolValue<false> {};
	template<class T>
	struct IsSame<T, T> : public BoolValue<true> {};

	template <int a, int... values>
	struct Max;
	template <int arg>
	struct Max<arg> { static constexpr int value = arg; };
	template <int a, int b, int... values>
	struct Max<a, b, values...> { static constexpr int value = a > b ? Max<a, values...>::value : Max<b, values...>::value; };
	template <int a, int... values>
	struct Min;
	template <int arg>
	struct Min<arg> { static constexpr int value = arg; };
	template <int a, int b, int... values>
	struct Min<a, b, values...> { static constexpr int value = a < b ? Min<a, values...>::value : Min<b, values...>::value; };

	template<class T>
	struct RemoveConst { using Type = T; };
	template<class T>
	struct RemoveConst<T const> { using Type = T; };

	template<class T>
	struct RemoveRef { using Type = T; };
	template<class T>
	struct RemoveRef<T&> { using Type = T; };
	template<class T>
	struct RemoveRef<T&&> { using Type = T; };

	template<class T>
	struct RemoveCVRef { using Type = typename RemoveConst<typename RemoveRef<T>::Type>::Type; };
}

namespace DEngine::Std::Trait
{
	template<unsigned int index, class... Ts>
	using At = typename impl::At<index, Ts...>::Type;
	// Returns the type at the index, or returs Or if the index is out of bounds.
	template<unsigned int index, class Or, class... Ts>
	using AtOr = typename impl::AtOr<index, impl::Max<0, (int)index - (int)sizeof...(Ts) + 1>::value, Or, Ts...>::Type;

	template<bool expr, class T, class U>
	using Conditional = typename impl::Conditional<expr, T, U>::Type;
	template<bool expr, class T, class U>
	using Cond = Conditional<expr, T, U>;

	template<class T, class ...Ts>
	constexpr bool existsInPack = (impl::IsSame<T, Ts>::value || ...);

	template<class T, class... Ts>
	constexpr unsigned int indexOf = impl::IndexOf<T, Ts...>::value;

	template<class Base, class Derived>
	static constexpr bool isBaseOf = __is_base_of(Base, Derived);

	template<class T>
	constexpr bool isConst = impl::IsConst<T>::value;

	template<class T>
	constexpr bool isFloatingPoint = impl::IsSame<T, float>::value || impl::IsSame<T, double>::value;

	template<class T>
	constexpr bool isRef = impl::IsRef<T>::value;

	template<class T>
	constexpr bool isRValueRef = impl::IsRValueRef<T>::value;

	template<class T, class U>
	constexpr bool isSame = impl::IsSame<T, U>::value;

	template<int... values>
	constexpr int max = impl::Max<values...>::value;
	template<int... values>
	constexpr int min = impl::Min<values...>::value;
	
	template<class T>
	using RemoveConst = typename impl::RemoveConst<T>::Type;
	
	template<class T>
	using RemoveCVRef = typename impl::RemoveCVRef<T>::Type;

	template<class T>
	using RemoveRef = typename impl::RemoveRef<T>::Type;
}