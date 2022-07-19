#pragma once

#include <vector>

namespace DEngine::Std {
	template<class ReturnT, class... Ts>
	class FnVec;

	template<class ReturnT, class... Ts>
	class FnVec<ReturnT(Ts...)> {


	};
}