#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Trait.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

struct StackLayout::Impl
{
	// Contains references to stuff like Extent width and height based on
	// the StackLayout direction. This is so we don't have
	// to care whether the StackLayout is horizontal or vertical
	template<class T>
	struct StackLayoutDirRefs
	{
		T& dir;
		T& nonDir;
	};
	[[nodiscard]] static auto GetDirRefs(StackLayout::Direction dir, Extent& in) noexcept
		-> StackLayoutDirRefs<u32>
	{
		if (dir == StackLayout::Direction::Horizontal)
			return { in.width, in.height };
		else
			return { in.height, in.width };
	}

	[[nodiscard]] static auto GetDirRefs(StackLayout::Direction dir, Extent const& in) noexcept
		-> StackLayoutDirRefs<u32 const>
	{
		if (dir == StackLayout::Direction::Horizontal)
			return { in.width, in.height };
		else
			return { in.height, in.width };
	}

	[[nodiscard]] static auto GetDirRefs(StackLayout::Direction dir, Math::Vec2Int& in) noexcept
		-> StackLayoutDirRefs<decltype(in.x)>
	{
		if (dir == StackLayout::Direction::Horizontal)
			return { in.x, in.y };
		else
			return { in.y, in.x };
	}

	[[nodiscard]] static bool GetDirExpand(StackLayout::Direction dir, SizeHint const& sizeHint) noexcept
	{
		if (dir == StackLayout::Direction::Horizontal)
			return sizeHint.expandX;
		else
			return sizeHint.expandY;
	}

	[[nodiscard]] static Extent GetSizeHintExtentSum(
		StackLayout::Direction dir,
		Std::Span<SizeHint const> const& sizeHints) noexcept
	{
		Extent returnVal = {};

		auto returnValRefs = GetDirRefs(dir, returnVal);

		for (auto const& childSizeHint : sizeHints)
		{
			auto const childRefs = GetDirRefs(dir, childSizeHint.minimum);
			returnValRefs.dir += childRefs.dir;
			returnValRefs.nonDir = Math::Max(returnValRefs.nonDir, childRefs.nonDir);
		}

		return returnVal;
	}

	// Returns the sum length of non-expanded child-SizeHints
	[[nodiscard]] static u32 GetNonExpandedChildrenSumDirLength(
		StackLayout::Direction dir,
		Std::Span<SizeHint const> const& sizeHints) noexcept
	{
		u32 returnVal = 0;
		for (auto const& childSizeHint : sizeHints)
		{
			if (!GetDirExpand(dir, childSizeHint))
			{
				auto const childRefs = GetDirRefs(dir, childSizeHint.minimum);
				returnVal += childRefs.dir;
			}
		}
		return returnVal;
	}

	[[nodiscard]] static u32 GetExpandChildrenCount(
		StackLayout::Direction dir,
		Std::Span<SizeHint const> const& sizeHints) noexcept
	{
		u32 returnValue = 0;
		for (auto const& childSizeHint : sizeHints)
		{
			if (GetDirExpand(dir, childSizeHint))
				returnValue += 1;
		}
		return returnValue;
	}

	struct SizeAlgorithmInput
	{
		u32 totalUsableSize;
		u32 totalLengthSum;
		u32 remainingLength;
		u32 nonExpandingLengthSum;
		u32 totalExpandingCount;
		u32 elementMinimumLength;
		bool expand;
		bool isLast;
	};

	static u32 SizeAlgorithm(SizeAlgorithmInput const& params)
	{
		// If we can fit all elements, scale them down according to
		// their sizes relative to each other.
		if (params.totalUsableSize < params.totalLengthSum)
		{
			if (params.isLast)
				return params.remainingLength;
			else
			{
				f32 scale = (f32)params.totalUsableSize / (f32)params.totalLengthSum;
				return (u32)Math::Round((f32)params.elementMinimumLength * scale);
			}
		}
		else
		{
			// We can fit all widgets with space to spare. Don't scale up
			// columns that do not want to expand.
			if (!params.expand)
				return params.elementMinimumLength;
			else
			{
				auto lengthLeftForExpanding = params.totalUsableSize - params.nonExpandingLengthSum;
				return (u32)Math::Round((f32)(lengthLeftForExpanding) / (f32)params.totalExpandingCount);
			}
		}
	};

	[[nodiscard]] static auto BuildChildRects(
		Rect const& parentRect,
		StackLayout::Direction dir,
		u32 spacing,
		Std::Span<SizeHint const> sizeHints,
		AllocRef const& transientAlloc)
	{
		auto const childCount = (int)sizeHints.Size();
		if (childCount <= 0)
			return Std::NewVec<Rect>(transientAlloc);

		auto parentExtent = GetDirRefs(dir, parentRect.extent);

		Extent usableExtent_Inner = {};
		auto usableExtent = GetDirRefs(dir, usableExtent_Inner);
		usableExtent.dir = parentExtent.dir - spacing * (childCount - 1);
		usableExtent.nonDir = parentExtent.nonDir;

		auto const sumChildMinExtent_Inner = GetSizeHintExtentSum(dir, sizeHints);
		auto const sumChildMinExtent = GetDirRefs(dir, sumChildMinExtent_Inner);
		// Holds the minimum length of non-children in the direction dir
		auto const sumNonExpandedChildMinDir = GetNonExpandedChildrenSumDirLength(dir, sizeHints);
		u32 const expandedChildCount = GetExpandChildrenCount(dir, sizeHints);

		auto returnVal = Std::NewVec<Rect>(transientAlloc);
		returnVal.Resize(childCount);

		auto remainingLength = usableExtent.dir;
		auto childPos = parentRect.position;
		auto const childPosRefs = GetDirRefs(dir, childPos);
		for (int i = 0; i < childCount; i += 1)
		{
			auto& childRect = returnVal[i];
			childRect.position = childPos;

			auto childExtent = GetDirRefs(dir, childRect.extent);
			auto const& childSizeHint_Inner = sizeHints[i];
			bool const childDirExpand = GetDirExpand(dir, childSizeHint_Inner);
			auto childSizeHintExtent = GetDirRefs(dir, childSizeHint_Inner.minimum);

			Impl::SizeAlgorithmInput temp = {};
			temp.totalUsableSize = usableExtent.dir;
			temp.totalLengthSum = sumChildMinExtent.dir;
			temp.elementMinimumLength = childSizeHintExtent.dir;
			temp.totalExpandingCount = expandedChildCount;
			temp.remainingLength = remainingLength;
			temp.nonExpandingLengthSum = sumNonExpandedChildMinDir;
			temp.isLast = i == childCount - 1;
			temp.expand = childDirExpand;

			childExtent.dir = Impl::SizeAlgorithm(temp);

			// In the non-dir direction, children are always assigned the entire length.
			childExtent.nonDir = usableExtent.nonDir;

			remainingLength -= childExtent.dir;
			childPosRefs.dir += (i32)childExtent.dir + spacing;
		}

		return returnVal;
	}

	// Gives you the modified index based on the insertion/remove jobs.
	// Return nullOpt if this index was deleted.
	[[nodiscard]] static Std::Opt<uSize> GetModifiedWidgetIndex(
		Std::Span<StackLayout::InsertRemoveJob const> jobs,
		uSize index) noexcept
	{
		uSize returnVal = index;
		for (auto const& job: jobs)
		{
			switch (job.type)
			{
				case StackLayout::InsertRemoveJob::Type::Insert:
				{
					if (job.insert.index <= returnVal)
						returnVal += 1;
					break;
				}
				case StackLayout::InsertRemoveJob::Type::Remove:
				{
					if (job.remove.index == returnVal)
						return Std::nullOpt;
					else if (job.remove.index < returnVal)
						returnVal -= 1;
					break;
				}
				default:
					DENGINE_IMPL_UNREACHABLE();
					break;
			}
		}

		return returnVal;
	}

	struct Iterator_Result {
		Widget& child;
	};
	struct Iterator_End {};
	struct Iterator {
		StackLayout* layout = nullptr;
		Widget* nextChild = nullptr;
		int currIndex = 0;
		int len = 0;
		explicit Iterator(StackLayout& in) {
			layout = &in;
			len = (int)in.children.size();

			Increment();
		}

		void Increment()
		{
			Std::Opt<uSize> actualIndexOpt;
			for (; currIndex < len && !actualIndexOpt.Has(); currIndex++) {
				actualIndexOpt = GetModifiedWidgetIndex(
					{ layout->insertionJobs.data(), layout->insertionJobs.size() },
					currIndex);
			}

			if (actualIndexOpt.Has())
				nextChild = layout->children[actualIndexOpt.Get()].Get();
			else
				nextChild = nullptr;
		}

		auto operator*() const {
			return Iterator_Result{
				.child = *nextChild,
			};
		}
		void operator++() {
			Increment();
		}
		[[nodiscard]] bool operator!=(Iterator_End const&) const noexcept {
			return nextChild;
		}

		~Iterator() {
			if constexpr (!Std::Trait::isConst<decltype(*layout)>) {
				layout->currentlyIterating = false;
				layout->insertionJobs.clear();
			}
		}
	};
	struct IteratorPair {
		StackLayout* layout = nullptr;
		auto begin() {
			return Iterator(*layout);
		}
		auto end() {
			return Iterator_End{};
		}
	};

	[[nodiscard]] static auto BuildItPair(StackLayout& layout) {
		return IteratorPair{
			.layout = &layout
		};
	}
};

namespace DEngine::Gui::impl
{
	// Generates the rect for the inner StackLayout Rect with padding taken into account
	[[nodiscard]] static Rect BuildInnerRect(
		Rect const& widgetRect,
		u32 padding) noexcept
	{
		Rect returnVal = widgetRect;
		returnVal = returnVal.Reduce(padding);
		return returnVal;
	}
}

StackLayout::~StackLayout()
{
}

uSize StackLayout::ChildCount() const
{
	return (uSize)children.size();
}

Widget& StackLayout::At(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];

	DENGINE_IMPL_GUI_ASSERT(child);

	return *child;
}

Widget const& StackLayout::At(uSize index) const
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];

	DENGINE_IMPL_GUI_ASSERT(child);

	return *child;
}

void StackLayout::AddWidget(Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(in);

	children.emplace_back(static_cast<Std::Box<Widget>&&>(in));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = InsertRemoveJob::Insert::pushBackIndex;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

Std::Box<Widget> StackLayout::ExtractChild(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	auto& item = children[index];
	auto returnVal = static_cast<Std::Box<Widget>&&>(item);

	children.erase(children.begin() + index);

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Remove;
		newJob.remove.index = index;
		insertionJobs.emplace_back(newJob);
	}

	return returnVal;
}

void StackLayout::InsertWidget(uSize index, Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	DENGINE_IMPL_GUI_ASSERT(in);

	children.insert(children.begin() + index, static_cast<Std::Box<Widget>&&>(in));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = index;
		insertionJobs.emplace_back(newJob);
	}
}

void StackLayout::RemoveItem(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	children.erase(children.begin() + index);

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Remove;
		newJob.remove.index = index;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

void StackLayout::ClearChildren()
{
	children.clear();
}

SizeHint StackLayout::GetSizeHint2(GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	int const childCount = children.size();

	SizeHint returnVal = {};

	auto childSizeHints = Std::NewVec<SizeHint>(params.transientAlloc);
	childSizeHints.Resize(childCount);
	for (int i = 0; i < childCount; i += 1)
		childSizeHints[i] = children[i]->GetSizeHint2(params);

	returnVal.minimum = Impl::GetSizeHintExtentSum(
		direction,
		childSizeHints.ToSpan());

	auto returnValRefs = Impl::GetDirRefs(direction, returnVal.minimum);

	// Add spacing
	if (childCount > 0)
		returnValRefs.dir += spacing * (childCount - 1);

	// Add padding
	returnValRefs.dir += padding * 2;
	returnValRefs.nonDir += padding * 2;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal);
	return returnVal;
}

void StackLayout::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;

	uSize const childCount = children.size();

	auto childEntries = Std::NewVec<RectCollection::It>(transientAlloc);
	childEntries.Resize(childCount);
	for (uSize i = 0; i < childCount; i += 1)
		childEntries[i] = pusher.GetEntry(*children[i]);

	auto childSizeHints = Std::NewVec<SizeHint>(transientAlloc);
	childSizeHints.Resize(childCount);
	for (uSize i = 0; i < childCount; i += 1)
		childSizeHints[i] = pusher.GetSizeHint(childEntries[i]);

	auto const innerRect = impl::BuildInnerRect(widgetRect, padding);

	auto const childRects = Impl::BuildChildRects(
		innerRect,
		direction,
		spacing,
		childSizeHints.ToSpan(),
		transientAlloc);

	for (uSize i = 0; i < childCount; i += 1)
	{
		auto& child = *children[i];
		pusher.SetRectPair(childEntries[i], { childRects[i], visibleRect });
		child.BuildChildRects(
			params,
			childRects[i],
			visibleRect);
	}
}

void StackLayout::CursorExit(Context& ctx)
{
	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		child.CursorExit(ctx);
	}
}

bool StackLayout::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;

	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		auto* childRects = rectColl.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRects);
		child.CursorMove(
			params,
			childRects->widgetRect,
			childRects->visibleRect,
			occluded);
	}

	return
		occluded ||
		PointIsInAll(params.event.position, { widgetRect, visibleRect });
}

bool StackLayout::CursorPress2(
	CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumedIn)
{
	auto const& rectColl = params.rectCollection;

	bool consumed = consumedIn;

	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		auto* childRects = rectColl.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRects);

		bool newResult = child.CursorPress2(
			params,
			childRects->widgetRect,
			childRects->visibleRect,
			consumed);
		consumed = consumed || newResult;
	}

	if (PointIsInAll(params.cursorPos, { widgetRect, visibleRect }))
		consumed = true;

	return consumed;
}

bool StackLayout::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;

	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		auto* childRects = rectColl.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRects);
		child.TouchMove2(
			params,
			childRects->widgetRect,
			childRects->visibleRect,
			occluded);
	}

	return
		occluded ||
		PointIsInAll(params.event.position, { widgetRect, visibleRect });
}

bool StackLayout::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumedIn)
{
	auto const& rectColl = params.rectCollection;

	bool consumed = consumedIn;

	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		auto* childRects = rectColl.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRects);

		bool newResult = child.TouchPress2(
			params,
			childRects->widgetRect,
			childRects->visibleRect,
			consumed);
		consumed = consumed || newResult;
	}

	if (PointIsInAll(params.event.position, { widgetRect, visibleRect }))
		consumed = true;

	return consumed;
}

void StackLayout::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectColl = params.rectCollection;

	auto const innerRect = impl::BuildInnerRect(widgetRect, padding);
	if (Rect::Intersection(innerRect, visibleRect).IsNothing())
		return;

	// Only draw the quad of the layout if the alpha of the color is high enough to be visible.
	if (color.w > 0.01f) {
		params.drawInfo.PushFilledQuad(innerRect, color);
	}

	auto const childCount = children.size();
	for (uSize childIndex = 0; childIndex < childCount; childIndex += 1)
	{
		auto const indexOpt = Impl::GetModifiedWidgetIndex(
			{ insertionJobs.data(), insertionJobs.size() },
			childIndex);
		if (!indexOpt.HasValue())
			continue;

		auto const& child = *children[indexOpt.Value()];
		auto const* childRectPairPtr = rectColl.GetRect(child);
		DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
		auto const& childRects = *childRectPairPtr;
		child.Render2(
			params,
			childRects.widgetRect,
			childRects.visibleRect);
	}
}

void StackLayout::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		child.TextInput(
			ctx,
			transientAlloc,
			event);
	}
}

void StackLayout::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	for (auto const& iter : Impl::BuildItPair(*this)) {
		auto& child = iter.child;
		child.EndTextInputSession(
			ctx,
			transientAlloc,
			event);
	}
}