#pragma once

#include "DEngine/Int.hpp"
#include "DEngine/Utility.hpp"
#include "DEngine/Containers/Assert.hpp"

namespace DEngine::Containers
{
	template<typename T>
	class Optional
	{
		bool m_hasValue = false;
		union InnerData
		{
			u8 bytePadding;
			T value;
			
			InnerData(u8 padding) noexcept : bytePadding(padding) {}
			InnerData(const T& in) : value(in) {}
			InnerData(T&& in) : value(Util::move(in)) {}
		};
		InnerData m_innerData;

	public:
		[[nodiscard]] inline Optional() noexcept :
			m_hasValue(false),
			m_innerData(u8(0)) 
		{}

		[[nodiscard]] inline Optional(const T& value) noexcept :
			m_hasValue(true),
			m_innerData(value)
		{}

		[[nodiscard]] inline Optional(T&& value) noexcept :
			m_hasValue(true),
			m_innerData(Util::move(value))
		{}

		[[nodiscard]] inline bool hasValue() const noexcept;

		[[nodiscard]] inline const T& value() const noexcept;

		[[nodiscard]] inline T& value() noexcept;
	};

	template<typename T>
	using Opt = Optional<T>;

	template<typename T>
	inline bool Optional<T>::hasValue() const noexcept
	{
		return m_hasValue;
	}

	template<typename T>
	inline const T& Optional<T>::value() const noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(m_hasValue == true, "Tried to deference Optional-variable without value.");
		return m_innerData.value;
	}

	template<typename T>
	inline T& Optional<T>::value() noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(m_hasValue == true, "Tried to deference Optional-variable without value.");
		return m_innerData.value;
	}
}

namespace DEngine
{
	namespace Cont = Containers;
}