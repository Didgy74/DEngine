#pragma once

namespace DEngine::Std
{
	template<class Runnable>
	class Defer
	{
	public:
		Defer() = default;
		Defer(Runnable const& in) noexcept : runnable{ &in } {}
		Defer(Defer const&) = delete;
		Defer(Defer&&) = delete;

		Defer& operator=(Defer const&) = delete;
		Defer& operator=(Defer&&) = delete;

		~Defer()
		{
			if (!done)
			{
				(*runnable)();
				done = true;
			}
		}

	private:
		Runnable const* runnable = nullptr;
		bool done = false;
	};
}