#pragma once

namespace DEngine::Containers
{
	template<typename T, typename U>
	struct Pair
	{
		T a{};
		U b{};
	};
}


namespace DEngine
{
	namespace Cont = Containers;
}