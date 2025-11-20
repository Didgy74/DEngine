#include <DEngine/Gui/RectCollection.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

RectCollection::AllocRefT RectCollection::SizeHintPusher::Alloc() noexcept {
	return Collection().Alloc();
}

RectCollection::Iter RectCollection::SizeHintPusher::Push(
	Widget const& widget,
	SizeHint const& sizeHint,
	bool invokable)
{
	auto const& entry = this->AddEntry(widget);
	this->SetSizeHint(entry, sizeHint, invokable);
	return entry;
}

Std::Opt<uSize> RectCollection::FindIndex(void const* ptr) const noexcept {
	constexpr auto invalidIndex = static_cast<uSize>(-1);
	auto const widgetCount = this->m_widgets.size();
	for (uSize i = 0; i < widgetCount; i += 1) {
		if (this->m_widgets[i].voidPtr == ptr) {
			return i;
		}
	}
	return Std::nullOpt;
}

bool RectCollection::PtrExists(void const* ptr) const noexcept {
	auto const widgetCount = m_widgets.size();
	for (uSize i = 0; i < widgetCount; i += 1) {
		if (m_widgets[i].voidPtr == ptr)
			return true;
	}
	return false;
}

RectCollection::Iter RectCollection::AddEntry(void const* ptr) {
	// Check that the widget has not already been inserted
	DENGINE_IMPL_GUI_ASSERT(!PtrExists(ptr));

	this->m_widgets.emplace_back(PointerUnion{ .voidPtr = ptr });
	this->m_sizeHints.push_back({});
	this->rects.push_back({});
	this->m_customData.push_back({});
	this->m_metadatas.push_back({});

	return Iter{ this->m_widgets.size() - 1 };
}

Std::Opt<RectCollection::Iter> RectCollection::GetEntry(
	void const* ptr,
	Std::Opt<Iter> const& iter) const
{
	if (iter.Has())
		return iter;
	return GetEntry(ptr);
}

Std::Opt<RectCollection::Iter> RectCollection::GetEntry(void const* ptr) const {
	auto temp = FindIndex(ptr);
	if (!temp.Has())
		return Std::nullOpt;
	return Iter{ temp.Value() };
}

SizeHint const* RectCollection::GetSizeHint(void const* ptr) const {
	auto temp = FindIndex(ptr);
	if (!temp.Has())
		return nullptr;
	return &this->m_sizeHints[temp.Value()];
}

Std::Opt<RectPair> RectCollection::GetRect(
	Widget const& widget,
	Std::Opt<Iter> const& iterOpt) const
{
	if (iterOpt.Has()) {
		return GetRect(iterOpt.Value());
	}
	return GetRect(widget);
}

void RectCollection::Prepare(bool includeRendering) {
	this->m_widgets.clear();
	this->m_sizeHints.clear();
	this->rects.clear();
	this->m_metadatas.clear();
	this->m_secondaryRects.clear();
	//this->m_secondaryRectAssigned.clear();

	for (auto& item : m_customData) {
		if (item.ptr != nullptr) {
			item.destructorFn(item.ptr);
			alloc.Free(item.ptr, item.allocSize);
		}
	}
	m_customData.clear();

	containsRendering = includeRendering;

	alloc.Reset();

	treeOffsetAndWindowExtentSet = false;
}

void RectCollection::Clear() {
	treeOffsetAndWindowExtentSet = false;
	m_treeOffset = {};
	m_windowExtent = {};

	this->m_widgets.clear();
	this->m_sizeHints.clear();
	this->rects.clear();
	this->m_secondaryRects.clear();
	//this->m_secondaryRectAssigned.clear();

	for (auto& item : m_customData) {
		if (item.ptr != nullptr) {
			item.destructorFn(item.ptr);
			alloc.Free(item.ptr, item.allocSize);
		}
	}
	m_customData.clear();
}
