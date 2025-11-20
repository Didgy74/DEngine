

#include <array>
#include <iostream>
#include <vector>

using namespace DEngine;

struct TestParams {
	f32 navAngle = {};
	Gui::Rect focusRect = {};
	std::vector<Gui::Rect> rectCandidates;

	int expectedOutIndex = 0;
};
bool testFn(TestParams const& params) {
	auto hitResultOpt = Gui::FindBestNavItem2(
		params.focusRect,
		params.navAngle,
		{
			(int)params.rectCandidates.size(),
			[&](int index) -> Std::Opt<Gui::Rect> { return params.rectCandidates[index]; }});
	if (hitResultOpt.Has()) {
		auto hitResult = hitResultOpt.Get();
		if (hitResult.index == params.expectedOutIndex) {
			return true;
		}
	}
	return false;
}

bool testAll(Std::Span<TestParams const> tests) {
	for (auto const& test : tests) {
		if (!testFn(test)) {
			return false;
		}
	}
	return true;
}

int main() {

	try {
		std::vector listOfTestData = {
			TestParams {
				.navAngle = 0,
				.focusRect = Gui::Rect { 0, 10, 30,  10 },
				.rectCandidates = {
					Gui::Rect { 0, 0, 10,  10 },
					Gui::Rect { 0, 10, 10,  10 },
					Gui::Rect { 0, 20, 10,  10 },
				},
				.expectedOutIndex = 1,
			},
			TestParams {
				.navAngle = 0,
				.focusRect = { { 398, 91 }, { 602, 84 } },
				.rectCandidates = {
					{{ 398, 0 }, { 101, 84 } },
					{{ 499, 0 }, { 173, 84 } },
					{{ 672, 0 }, { 135, 84 } },
				},
				.expectedOutIndex = 1,
			},
		};

		if (!testAll({ listOfTestData.data(), listOfTestData.size() })) {
			return 1;
		}

	} catch (std::exception const& e) {
		std::cerr << "Exception thrown during tests";
		return -1;
	}

	return 0;
}