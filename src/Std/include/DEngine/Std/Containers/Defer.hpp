#pragma once

import DEngine.Std.Utility;

namespace DEngine::Std {
	template<class Runnable>
	class Defer {
	public:
		explicit Defer(Runnable&& in) noexcept
			: runnable(Std::Move(in))
		{}

		Defer() = delete;
		Defer(Defer const&) = delete;
		Defer& operator=(Defer const&) = delete;
		Defer(Defer&& other) = delete;
		Defer& operator=(Defer&&) = delete;

		~Defer() {
			if (active)
				runnable();
		}

	private:
		Runnable runnable;
		bool active = true;
	};
}