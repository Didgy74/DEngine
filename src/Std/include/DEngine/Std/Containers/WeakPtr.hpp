#pragma once

namespace DEngine::Std
{
	template<class T>
	class WeakPtr
	{
		WeakPtr() = default;
		WeakPtr(WeakPtr const& in) noexcept : data{ in.data } {}
		WeakPtr(WeakPtr&& in) noexcept : data{ in.data } { in.data = nullptr; }
		WeakPtr(T* ptr) noexcept : data{ ptr } {}

		WeakPtr& operator=(WeakPtr const& in) noexcept
		{
			if (this == &in)
				return *this;
			data = in.data;
		}
		WeakPtr& operator=(WeakPtr&& in) noexcept
		{
			if (this == &in)
				return *this;
			data = in.data;
			in.data = nullptr;
		}
		WeakPtr& operator=(T* ptr) noexcept { data = ptr; }

		[[nodiscard]] T* Get() noexcept { return data; }
		[[nodiscard]] T const* Get() const noexcept { return data; }

	private:
		T* data = nullptr;
	};
}