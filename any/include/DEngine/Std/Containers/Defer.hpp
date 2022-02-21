#pragma once

namespace DEngine::Std
{
	template<class Runnable>
	class Defer
	{
	public:
		Defer(Runnable const& in) : runnable{ in } {}

		~Defer()
		{
			if (!done)
			{
				runnable();
				done = true;
			}
		}

	private:
		Runnable const& runnable;
		bool done = false;
	};
}