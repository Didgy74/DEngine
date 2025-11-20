#pragma once

#include <DEngine/Gui/impl/Assert.hpp>

#include <vector>

import DEngine.FixedWidthTypes;
import DEngine.Gui.AllocRef;
import DEngine.Gui.SizeHint;
import DEngine.Std.Vec;
import DEngine.Gui.Utility;
import DEngine.Math.Vector;
import DEngine.Std.BumpAllocator;
import DEngine.Std.Opt;
import DEngine.Std.Span;
import DEngine.Std.Trait;
import DEngine.Std.Utility;

namespace DEngine::Gui {
	class Widget;
	class Layer;

	struct RectPair {
		Rect widgetRect;
		Rect visibleRect;
	};

	/*
	Some general description of the RectCollection is as follows:

	While the UI tree of Widgets is highly abstract, the RectCollection represents
	all resolved state once the UI tree has been measured and evaluated. The RectCollection
	is principally only valid within the context of a single event, or until the UI tree
	has been invalidated. Conceptually, the RectCollection primarily concerns itself
	with the screenspace assigned to Widgets, some basic metadata for the Widgets,
	and some additional utilities to speed up/cache event handling.

	The RectCollection is built during the measuring and layouting steps that happen before
	(most) events. Most events will require a RectCollection that has been resolved in order to
	execute.

	The RectCollection is primarily aimed to encompass exactly one UI surface. This includes
	any floating layers in front of that UI surface. This means some Widgets are allowed to be
	outside the primary boundary of the UI surface but only if they are not part of the rearmost
	layer. Such layers commonly include dropdown menus or context menus, things that would
	often be implemented as child windows. Note: The Context class is deprecated and the
	Layer class too, their semantics are unclear and will be removed.

	The RectCollection stores the Widget-rectangles relatives to a configurable anchor point,
	with origin in the top left. This anchor can be translated after layouting. This enables the
	mobile-usecase where we want to translate the entire UI after layouting, which is useful for
	mobile where on-screen touchscreen pops up and we want to translate the UI such that a
	specific Widget comes into view.

	The RectCollection is also used for usecases such as: Rendering, directional navigation,
	cursor/touch events, and accessibility generation.

	Each Widget in the RectCollection has an optional primary RectPair describing local rectangle
	and the visible rect. This may not exist in the case where the Widget was determined not be
	visible during layouting phase.

	During the measuring phase, the RectCollection has limited functionality and is encapsulated
	as the SizeHintPusher class. Similarly, during the layouting phase, the RectCollection is
	encapsulated as the RectPusher class.

	Each Widget node in the RectCollection may attach custom data of any type. The type of this
	CustomData is defined by each Widget, and is handed back to the Widget as a type-erased
	handle in later phases. The RectCollection has its own allocator specifically for storing
	custom-data that you may use for any nested containers within the custom-data struct. The
	allocator and custom data is valid until the RectCollection is reset or destroyed.

	The RectCollection must be reset ahead of any measuring phases. The measuring phase must
	happen before the layout phase. It is not valid to modify the UI tree in during or in-between
	measuring and layouting phases. The RectCollection is invalidated if the associated UI tree is
	invalidated.

	One Widget can have:
	- Exactly one SizeHint
	- Exactly one CustomData (Any type)
	- Exactly one invocable flag (boolean), that describes whether the Widget can be invoked.
		- This is important for directional navigation.
	- Exactly one optional RectPair
	- Multiple secondary RectPairs, allocated during layout via RectPusher::AddSecondaryEntries.
	They enable a single Widget to expose multiple navigable/hittable sub-regions (e.g. individual
	buttons in a ButtonGroup) without requiring separate Widget tree nodes.
	Limitation: All secondary entries for a given Widget must be allocated contiguously in
	one AddSecondaryEntries call. Interleaving allocations from different Widgets is not
	supported.

	Access is phase-gated:
	- SizeHintPusher: measuring phase — AddEntry, SetSizeHint, AttachCustomData
	- RectPusher: layout phase — SetLocalRectPair, AddSecondaryEntries, SetSecondaryRectPair,
	  mutable GetCustomData
	- RectCollection (const): dispatch/render phase — read-only access to all data

	Some design rationale:
	- The RectCollection allows us to cache several calculations such as text-measuring and
	layouting.
	- We can use it in order to run events while treating the UI tree as immutable.
	- Having multiple secondary RectPairs per Widget is particularly beneficial for directional
	navigation because it allows us to navigate as if we have many multiple Widgets while we only
	really have one.

	Some concerns for the future:
	- We need to be able to re-order rendering to optimize rendering performance. I.e
	draw opaques front-to-back and then non-opaques back-to-front.
	- The semantics of visible Rect is unclear in some edge-cases. Like sometimes it must be
	unbound, but other times it cannot be. Additionally, it's unclear if we can even determine
	visible rect until after layouting phase.
	- It's unclear how the configurable anchor point behaves for floating layers.
	- We need additional validation that the RectCollection is resolved in the correct order,
	and that it is completely resolved before any events that rely on it are processed.
	- It's important that the RectCollection is optimized for fast lookup, and also spatial
	memory locality, as we intend to do lots of lookups during event dispatching. Also
	we expect to touch most of the RectCollection in a predictable order.
	- The term iterator might be wrong because these are never meant to allow you to iterate
	the data-structure, but rather give efficient access to specific items.
	- Rather than "invocable" a better term might be "navigation focusable".
	- Need to research whether local rect must always be set, even if considered invisible
	at layouting stage, because moving the anchor could potentially bring the widget into
	view.
	*/
	class RectCollection {
	public:
		using AllocRefT = AllocRef;

		// The allocator of the RectCollection, used for storing custom data.
		[[nodiscard]] AllocRefT Alloc() noexcept { return alloc; }

		// Valid after the measuring phase. Refers to a Widget's primary entry.
		// Should always be valid after the SizeHint gathering stage.
		struct Iterator {
			Iterator() = default;
			// TODO: Is this even safe to expose?
			explicit Iterator(uSize index) : index{ index } {}
		private:
			uSize index = invalidIndex;
			static constexpr uSize invalidIndex = static_cast<uSize>(-1);
			friend RectCollection;
		};
		using Iter = Iterator;

		// Valid after the layout phase. Refers to a secondary RectPair slot.
		// Allocated via RectPusher::AddSecondaryEntries
		struct SecondaryIter {
			SecondaryIter() = default;
			// TODO: Is this even safe to expose?
			explicit SecondaryIter(uSize index) : index{ index } {}
		private:
			uSize index = invalidIndex;
			static constexpr uSize invalidIndex = static_cast<uSize>(-1);
			friend RectCollection;
		};

		// This is an iterator that should be used specifically for identifying focused Widget.
		// It will point at a specific Widget in the RectCollection, or one of the secondary Rects
		// for that Widget.
		struct FocusIter {
			FocusIter() = default;
		private:
			uSize index = invalidIndex;
			static constexpr uSize invalidIndex = static_cast<uSize>(-1);
			Std::Opt<uSize> secondaryIndex;
		};

		class SizeHintPusher {
		public:
			// This is only valid to create if the RectCollection is in resetted state
			explicit SizeHintPusher(RectCollection& collection) noexcept
				: m_collection{ &collection }
			{
				DENGINE_IMPL_GUI_ASSERT(m_collection != nullptr);
			}

			/**
				The allocator of the RectCollection, used for storing custom data.
			 */
			[[nodiscard]] AllocRefT Alloc() noexcept;

			[[nodiscard]] Iter Push(
				Widget const& widget,
				SizeHint const& sizeHint,
				bool invokable);

			template<class T> requires Std::Trait::isMoveConstructible<T>
			[[nodiscard]] Iter PushWithCustom(
				Widget const& widget,
				SizeHint const& sizeHint,
				bool invokable,
				T&& customData)
			{
				auto const& entry = this->AddEntry(widget);
				this->SetSizeHint(entry, sizeHint, invokable);
				this->AttachCustomData(entry, static_cast<T&&>(customData));
				return entry;
			}

			[[nodiscard]] Iter AddEntry(Widget const& widget) { return Collection().AddEntry(widget); }
			[[nodiscard]] Iter AddEntry(Layer const& layer) { return Collection().AddEntry(layer); }

			// TODO: We should eventually pass a struct most likely
			void SetSizeHint(Iter const& it, SizeHint const& sizeHint, bool invokable) {
				Collection().SetSizeHint(it, sizeHint, invokable);
			}

			template<class T>
			T& AttachCustomData(Iter const& it, T&& input) requires Std::Trait::isMoveConstructible<T> {
				return Collection().AttachCustomData(it, static_cast<T&&>(input));
			}

			[[nodiscard]] bool IncludeRendering() const noexcept { return Collection().BuiltForRendering(); }

			[[nodiscard]] bool GetIsInvokable(Iter const& it) const { return Collection().GetIsInvokable(it); }

		private:
			[[nodiscard]] RectCollection& Collection() noexcept {
				DENGINE_IMPL_GUI_ASSERT(m_collection != nullptr);
				return *m_collection;
			}
			[[nodiscard]] RectCollection const& Collection() const noexcept {
				DENGINE_IMPL_GUI_ASSERT(m_collection != nullptr);
				return *m_collection;
			}
			RectCollection* m_collection = nullptr;
		};

		class RectPusher {
		public:
			// This is only valid to create if the RectCollection is done with measuring phase
			explicit RectPusher(RectCollection& collection) noexcept
				: m_collection{ &collection }
			{
				DENGINE_IMPL_GUI_ASSERT(m_collection != nullptr);
			}

			// The allocator of the RectCollection, used for storing custom data.
			[[nodiscard]] AllocRefT Alloc() noexcept { return Collection().Alloc(); }

			[[nodiscard]] Iter GetEntry(Widget const& widget) noexcept {
				auto temp = Collection().GetEntry(widget);
				DENGINE_IMPL_GUI_ASSERT(temp.HasValue());
				return temp.Value();
			}
			[[nodiscard]] Std::Opt<Iter> GetEntry(
				Widget const& widget,
				Std::Opt<Iter> const& iter) const
			{
				return Collection().GetEntry(widget, iter);
			}
			[[nodiscard]] Iter GetEntry(Layer const& layer) noexcept {
				auto temp = Collection().GetEntry(layer);
				DENGINE_IMPL_GUI_ASSERT(temp.HasValue());
				return temp.Value();
			}

			void SetLocalRectPair(Iter const& it, RectPair const& rect) {
				Collection().SetLocalRect(it, rect);
			}

			[[nodiscard]] Std::Opt<RectPair> GetLocalRectPair(Widget const& widget) const {
				return Collection().GetLocalRect(widget);
			}

			[[nodiscard]] SizeHint const& GetSizeHint(Iter const& it) const {
				return Collection().GetSizeHint(it);
			}
			[[nodiscard]] SizeHint const* GetSizeHint(Widget const& widget) const {
				return Collection().GetSizeHint(widget);
			}
			[[nodiscard, deprecated]] SizeHint const* GetSizeHint(Layer const& layer) const {
				return Collection().GetSizeHint(layer);
			}

			template<class T>
			[[nodiscard]] T* GetCustomData(Widget const& widget) {
				return Collection().GetCustomData<T>(widget);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData(Widget const& widget) const {
				return Collection().GetCustomData<T>(widget);
			}
			template<class T>
			[[nodiscard, deprecated]] T* GetCustomData(Layer const& layer) {
				return Collection().GetCustomData<T>(layer);
			}
			template<class T>
			[[nodiscard, deprecated]] T const* GetCustomData(Layer const& layer) const {
				return Collection().GetCustomData<T>(layer);
			}
			template<class T>
			[[nodiscard]] T* GetCustomData(Iter const& it) {
				return Collection().GetCustomData<T>(it);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData(Iter const& it) const {
				return Collection().GetCustomData<T>(it);
			}
			template<class T>
			T& AttachCustomData(Iter const& it, T&& input) {
				return Collection().AttachCustomData(it, static_cast<T&&>(input));
			}

			[[nodiscard]] bool IncludeRendering() const noexcept { return Collection().BuiltForRendering(); }

			/*
			// Allocates `count` secondary RectPair slots contiguously for the given primary entry.
			// The Widget must store the returned Vec in its CustomData and call
			// SetSecondaryRectPair for each slot during BuildChildRects.
			// All slots for a given Widget must be allocated in a single call.
			//
			// TODO: This should likely just take a list of values right away
			//
			// TODO: It might make sense to have an overload that does not return any
			// iterators, meaning no need for the allocation.
			[[nodiscard]] Std::Vec<SecondaryIter, Gui::AllocRef> AddSecondaryEntries(
				Iter const& owner,
				uSize count,
				Gui::AllocRef transientAlloc)
			{
				return Collection().AddSecondaryEntries(owner, count, transientAlloc);
			}

			void SetSecondaryRectPair(SecondaryIter const& it, RectPair const& rect) {
				Collection().SetSecondaryRect(it, rect);
			}

			[[nodiscard]] RectPair GetSecondaryLocalRectPair(SecondaryIter const& it) const {
				return Collection().GetSecondaryLocalRect(it);
			}
			*/

			void SetSecondaryItems(
				Iter const& owner,
				Std::Span<RectPair const> const& items)
			{
				Collection().SetSecondaryItems(owner, items);
			}

		private:
			[[nodiscard]] RectCollection& Collection() noexcept {
				DENGINE_IMPL_GUI_ASSERT(m_collection != nullptr);
				return *m_collection;
			}
			[[nodiscard]] RectCollection const& Collection() const noexcept {
				DENGINE_IMPL_GUI_ASSERT(m_collection != nullptr);
				return *m_collection;
			}
			RectCollection* m_collection = nullptr;
		};

		RectCollection() = default;
		RectCollection(RectCollection const&) = delete;
		RectCollection(RectCollection&&) = delete;

		RectCollection& operator=(RectCollection const&) = delete;
		RectCollection& operator=(RectCollection&&) = delete;

		[[nodiscard]] Std::Opt<Iter> GetEntry(Widget const& widget) const { return GetEntry(&widget); }
		[[nodiscard]] Std::Opt<Iter> GetEntry(
			Widget const& widget,
			Std::Opt<Iter> const& iter) const
		{
			return GetEntry(&widget, iter);
		}
		[[nodiscard]] Std::Opt<Iter> GetEntry(Layer const& layer) const { return GetEntry(&layer); }

		[[nodiscard]] SizeHint const& GetSizeHint(Iter const& it) const {
			DENGINE_IMPL_GUI_ASSERT(it.index < this->m_sizeHints.size());
			return this->m_sizeHints[it.index];
		}
		[[nodiscard]] SizeHint const* GetSizeHint(Widget const& widget) const { return GetSizeHint(&widget); }
		[[nodiscard]] SizeHint const* GetSizeHint(Layer const& layer) const { return GetSizeHint(&layer); }

		[[nodiscard]] bool GetIsInvokable(Iter const& it) const {
			return GetMetadata(it).invokable;
		}
		[[nodiscard]] uSize GetSecondaryCount(Iter const& it) const {
			return GetMetadata(it).secondaryCount;
		}

		[[nodiscard]] Std::Opt<RectPair> GetRect(Widget const& widget) const { return GetRect(&widget); }
		[[nodiscard]] Std::Opt<RectPair> GetLocalRect(Widget const& widget) const { return GetLocalRect(&widget); }
		[[nodiscard]] Std::Opt<RectPair> GetRect(
			Widget const& widget,
			Std::Opt<Iter> const& iterOpt) const;
		[[nodiscard, deprecated]] Std::Opt<RectPair> GetRect(Layer const& layer) const { return GetRect(&layer); }
		// TODO: This also needs to return an opt, because even if there is an entry
		// in the RectCollection for a given Widget, it is possible that it is holding
		// a SizeHint, but there is no RectPair registered. (I.e it didn't end up in the
		// UI tree)
		[[nodiscard]] RectPair GetLocalRect(Iter const& it) const {
			DENGINE_IMPL_GUI_ASSERT(it.index < rects.size());
			return rects[it.index];
		}
		// TODO: This also needs to return an opt, because even if there is an entry
		// in the RectCollection for a given Widget, it is possible that it is holding
		// a SizeHint, but there is no RectPair registered. (I.e it didn't end up in the
		// UI tree)
		[[nodiscard]] RectPair GetRect(Iter const& it) const {
			DENGINE_IMPL_ASSERT(this->treeOffsetAndWindowExtentSet);
			auto temp = GetLocalRect(it);
			temp.widgetRect = temp.widgetRect.WithOffset(this->m_treeOffset);
			temp.visibleRect = temp.visibleRect
				.WithOffset(this->m_treeOffset);
				//.GetIntersect({ {}, this->m_windowExtent });
			return temp;
		}

		[[nodiscard]] RectPair GetSecondaryItem(Iterator const& primaryIter, int index) const {
			DENGINE_IMPL_GUI_ASSERT(primaryIter.index < m_metadatas.size());
			auto const& metadata = m_metadatas[primaryIter.index];
			DENGINE_IMPL_GUI_ASSERT(metadata.secondaryStart + index < m_secondaryRects.size());
			auto const& rect = m_secondaryRects[metadata.secondaryStart + index];
			return rect;
		}

		// Secondary rect read access — valid after layout phase.
		/*
		[[nodiscard]] RectPair GetSecondaryLocalRect(SecondaryIter const& it) const {
			DENGINE_IMPL_GUI_ASSERT(it.index < m_secondaryRects.size());
			return m_secondaryRects[it.index];
		}
		[[nodiscard]] RectPair GetSecondaryRect(SecondaryIter const& it) const {
			DENGINE_IMPL_ASSERT(this->treeOffsetAndWindowExtentSet);
			auto temp = GetSecondaryLocalRect(it);
			temp.widgetRect = temp.widgetRect.WithOffset(this->m_treeOffset);
			temp.visibleRect = temp.visibleRect.WithOffset(this->m_treeOffset);
			return temp;
		}
		*/

		template<class T>
		[[nodiscard]] T const* GetCustomData(Widget const& widget) const { return GetCustomData<T>(&widget); }
		template<class T>
		[[nodiscard]] T const* GetCustomData(Layer const& in) const { return GetCustomData<T>(&in); }
		template<class T>
		[[nodiscard]] T const* GetCustomData(Iter const& it) const
		{
			DENGINE_IMPL_GUI_ASSERT(it.index < m_customData.size());
			return reinterpret_cast<T const*>(m_customData[it.index].ptr);
		}

		[[nodiscard]] bool BuiltForRendering() const { return containsRendering; }

		struct CustomData {
			void* ptr = nullptr;
			uSize allocSize = 0;
			using DestroyFnT = void(*)(void* ptr);
			DestroyFnT destructorFn = nullptr;
		};

		void Prepare(bool includeRendering);

		void Clear();

		// TODO: This is definitely not the cleanest way to expose this function.
		// This needs to be called after assigning Rects, and before doing dispatching the
		// event that reads from RectCollection.
		void SetTreeOffsetAndWindowExtent(Math::Vec2Int treeOffset, Extent windowExtent) {
			DENGINE_IMPL_GUI_ASSERT(this->treeOffsetAndWindowExtentSet == false);
			this->m_treeOffset = treeOffset;
			this->m_windowExtent = windowExtent;
			this->treeOffsetAndWindowExtentSet = true;
		}

		~RectCollection() {
			Clear();
		}

	protected:
		// Used for custom data.
		Std::FrameAlloc alloc;

		bool containsRendering = false;
		bool treeOffsetAndWindowExtentSet = false;
		Math::Vec2Int m_treeOffset = {};
		Extent m_windowExtent = {};

		// All per-widget std::vectors must have the same length!
		// Makes it easier when debugging
		union PointerUnion {
			Widget const* widget;
			Layer const* layer;
			void const* voidPtr;
		};
		// Holds pointers to all relevant widgets in the hierarchy
		std::vector<PointerUnion> m_widgets;
		// Corresponds with the widgets vector.
		std::vector<SizeHint> m_sizeHints;
		// Per-widget metadata stored in the RectCollection.
		// Set during the measuring phase via SetSizeHint
		//
		// TODO: Profile whether it makes sense to store some of the members in their own
		// lists.
		struct WidgetMetadata {
			bool invokable = false;
			u64 secondaryStart = 0;
			u64 secondaryCount = 0;
			// TODO: This should only be used during debug builds!
			// This helps us ensure that the local RectPair is only set once per
			// item.
			bool localRectAssigned = false;
		};
		std::vector<WidgetMetadata> m_metadatas;
		// Corresponds with the widgets vector.
		std::vector<RectPair> rects;

		// Flat array of secondary RectPairs, appended during layout traversal.
		// Per-widget range (start + count) is stored in m_metadata.
		std::vector<RectPair> m_secondaryRects;
		std::vector<CustomData> m_customData;

		[[nodiscard]] Std::Opt<uSize> FindIndex(void const* ptr) const noexcept;

		[[nodiscard]] bool PtrExists(void const* ptr) const noexcept;

		[[nodiscard]] Std::Opt<Iter> GetEntry(void const* ptr, Std::Opt<Iter> const& iter) const;

		[[nodiscard]] Std::Opt<Iter> GetEntry(void const* ptr) const;

		[[nodiscard]] SizeHint const* GetSizeHint(void const* ptr) const;

		[[nodiscard]] Std::Opt<RectPair> GetLocalRect(void const* ptr) const {
			auto indexOpt = FindIndex(ptr);
			if (!indexOpt.Has())
				return Std::nullOpt;
			auto const& temp = rects[indexOpt.Value()];
			//if (temp.widgetRect.IsNothing() || temp.visibleRect.IsNothing())
				//return Std::nullOpt;
			return temp;
		}

		[[nodiscard]] Std::Opt<RectPair> GetRect(void const* ptr) const {
			DENGINE_IMPL_GUI_ASSERT(this->treeOffsetAndWindowExtentSet);
			auto tempRectOpt = GetLocalRect(ptr);
			if (!tempRectOpt.Has())
				return Std::nullOpt;

			auto temp = tempRectOpt.Get();
			temp.widgetRect = temp.widgetRect.WithOffset(this->m_treeOffset);
			temp.visibleRect = temp.visibleRect
				.WithOffset(this->m_treeOffset)
				.GetIntersect({ {}, this->m_windowExtent });
			return temp;
		}

		[[nodiscard]] WidgetMetadata const& GetMetadata(Iter const& it) const {
			DENGINE_IMPL_GUI_ASSERT(it.index < this->m_metadatas.size());
			return this->m_metadatas[it.index];
		}

		// Mutable GetCustomData — accessible via RectPusher (nested class) only.
		template<class T>
		[[nodiscard]] T* GetCustomData(Widget const& widget) { return GetCustomData<T>(&widget); }
		template<class T>
		[[nodiscard, deprecated]] T* GetCustomData(Layer const& in) { return GetCustomData<T>(&in); }
		template<class T>
		[[nodiscard]] T* GetCustomData(Iter const& it) {
			DENGINE_IMPL_GUI_ASSERT(it.index < m_customData.size());
			return reinterpret_cast<T*>(m_customData[it.index].ptr);
		}
		template<class T>
		[[nodiscard]] T* GetCustomData(void const* ptr) {
			auto temp = FindIndex(ptr);
			if (!temp.Has())
				return nullptr;
			return reinterpret_cast<T*>(m_customData[temp.Value()].ptr);
		}
		template<class T>
		[[nodiscard]] T const* GetCustomData(void const* ptr) const {
			auto temp = FindIndex(ptr);
			if (temp.HasValue())
				return reinterpret_cast<T const*>(m_customData[temp.Value()].ptr);
			else
				return nullptr;
		}

		// Write methods — accessible only through SizeHintPusher and RectPusher
		// (nested classes have access to protected/private members of the enclosing class).

		[[nodiscard]] Iter AddEntry(Widget const& widget) { return AddEntry(&widget); }
		[[nodiscard]] Iter AddEntry(Layer const& layer) { return AddEntry(&layer); }

		[[nodiscard]] Iter AddEntry(void const* ptr);

		void SetSizeHint(Iter const& it, SizeHint const& sizeHint, bool invokable) {
			DENGINE_IMPL_GUI_ASSERT(it.index < this->m_sizeHints.size());
			this->m_sizeHints[it.index] = sizeHint;
			this->m_metadatas[it.index].invokable = invokable;
		}

		// Sets the local RectPair of the item. This is before the treeOffset and final
		// windowRect is set.
		void SetLocalRect(Iter const& it, RectPair const& rect) {
			DENGINE_IMPL_GUI_ASSERT(it.index < rects.size());
			DENGINE_IMPL_GUI_ASSERT(!rect.widgetRect.IsNothing());
			DENGINE_IMPL_GUI_ASSERT(!rect.visibleRect.IsNothing());
			DENGINE_IMPL_GUI_ASSERT(m_metadatas[it.index].localRectAssigned == false);
			m_metadatas[it.index].localRectAssigned = true;
			rects[it.index] = rect;
		}

		template<class T>
		T& AttachCustomData(Iter const& it, T&& input)
			requires (Std::Trait::isMoveConstructible<T> && !Std::Trait::isConst<T>)
		{
			DENGINE_IMPL_GUI_ASSERT(it.index < this->m_widgets.size());
			auto& customDataItem = m_customData[it.index];

			// Make sure we haven't set the custom data before.
			DENGINE_IMPL_GUI_ASSERT(customDataItem.ptr == nullptr);

			// Technically this can fail. Should probably handle that...
			auto* returnPtr = static_cast<T*>(Alloc().Alloc(sizeof(T), alignof(T)));
			new(returnPtr, Std::placementNewTag) T(static_cast<T&&>(input));

			// Assert that we don't already have some data here.
			customDataItem.ptr = returnPtr;
			customDataItem.destructorFn = [](void* ptr) {
				static_cast<T*>(ptr)->~T();
			};

			return *returnPtr;
		}

		// Allocates `count` secondary slots contiguously in m_secondaryRects.
		// Records the range in the owner's WidgetMetadata.
		// Returns a bump-allocated Vec of SecondaryIters for the caller to store in CustomData.
		[[nodiscard]] Std::Vec<SecondaryIter, AllocRefT> AddSecondaryEntries(
			Iter const& owner,
			uSize count,
			Gui::AllocRef transientAlloc)
		{
			DENGINE_IMPL_GUI_ASSERT(owner.index < m_metadatas.size());
			DENGINE_IMPL_GUI_ASSERT(m_metadatas[owner.index].secondaryCount == 0);

			auto const startIdx = m_secondaryRects.size();
			m_secondaryRects.resize(startIdx + count);
			//m_secondaryRectAssigned.resize(startIdx + count, false);

			m_metadatas[owner.index].secondaryStart = (u32)startIdx;
			m_metadatas[owner.index].secondaryCount = (u32)count;

			auto result = Std::NewVec_Reserve<SecondaryIter>(transientAlloc, count);
			result.Resize(count);
			for (uSize i = 0; i < count; i += 1)
				result[i] = SecondaryIter{ startIdx + i };
			return result;
		}

		void SetSecondaryRect(SecondaryIter const& it, RectPair const& rect) {
			DENGINE_IMPL_GUI_ASSERT(it.index < m_secondaryRects.size());
			//DENGINE_IMPL_GUI_ASSERT(m_secondaryRectAssigned[it.index] == false);
			//m_secondaryRectAssigned[it.index] = true;
			m_secondaryRects[it.index] = rect;
		}

		void SetSecondaryItems(
			Iter const& owner,
			Std::Span<RectPair const> items)
		{
			DENGINE_IMPL_GUI_ASSERT(owner.index < m_metadatas.size());
			DENGINE_IMPL_GUI_ASSERT(m_metadatas[owner.index].secondaryCount == 0);
			auto count = items.Size();

			auto const startIdx = m_secondaryRects.size();
			m_secondaryRects.resize(startIdx + count);

			m_metadatas[owner.index].secondaryStart = (u32)startIdx;
			m_metadatas[owner.index].secondaryCount = (u32)count;

			for (uSize i = 0; i < count; i++) {
				m_secondaryRects[startIdx + i] = items[i];
			}
		}
	};

	struct FindBestNavItem_ReturnT {
		int index = 0;
		Widget* widget = nullptr;
		RectCollection::Iterator childIter;
	};
	[[nodiscard]] Std::Opt<FindBestNavItem_ReturnT> FindBestNavItem(
		Rect const& focusWidgetRect,
		f32 navAngle,
		RectCollection const& rectColl,
		Std::Span<Widget*> navItems);
} // namespace DEngine::Gui
