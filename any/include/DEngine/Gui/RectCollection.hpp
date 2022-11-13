#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/AllocRef.hpp>

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Trait.hpp>

#include <DEngine/Math/Common.hpp>

#include <vector>
#include <cstring>

namespace DEngine::Gui
{
	class Widget;
	class Layer;

	struct RectPair
	{
		Rect widgetRect;
		Rect visibleRect;
	};

	class RectCollection
	{
	public:
		using AllocRefT = AllocRef;
		AllocRefT Alloc() noexcept { return alloc; }

		// Should always be valid after the SizeHint
		// gathering stage
		struct Iterator
		{
			Iterator() = default;
			explicit Iterator(uSize index) : index{ index } {}
		private:
			uSize index = invalidIndex;
			static constexpr uSize invalidIndex = static_cast<uSize>(-1);

			friend RectCollection;
		};
		using It = Iterator;

		class SizeHintPusher
		{
		public:
			explicit SizeHintPusher(RectCollection& collection) noexcept :
				collection{ &collection }
			{}

			[[nodiscard]] AllocRefT Alloc() noexcept { return Collection().Alloc(); }

			It AddEntry(Widget const& widget) { return Collection().AddEntry(widget); }
			It AddEntry(Layer const& layer) { return Collection().AddEntry(layer); }
			void SetSizeHint(It const& it, SizeHint const& sizeHint) {
				return Collection().SetSizeHint(it, sizeHint);
			}

			template<class T>
			T* AttachCustomData(It const& it) requires Std::Trait::isDefaultConstructible<T> {
				return Collection().AttachCustomData<T>(it);
			}
			template<class T>
			T& AttachCustomData(It const& it, T&& input) requires Std::Trait::isMoveConstructible<T> {
				return Collection().AttachCustomData(it, static_cast<T&&>(input));
			}

			[[nodiscard]] bool IncludeRendering() const noexcept { return Collection().BuiltForRendering(); }

		private:
			[[nodiscard]] RectCollection& Collection() noexcept { return *collection; }
			[[nodiscard]] RectCollection const& Collection() const noexcept { return *collection; }
			RectCollection* collection = nullptr;
		};

		class RectPusher
		{
		public:
			explicit RectPusher(RectCollection& collection) noexcept : collection{ &collection } {}

			[[nodiscard]] AllocRefT Alloc() noexcept { return Collection().Alloc(); }

			[[nodiscard]] It GetEntry(Widget const& widget) noexcept {
				auto temp = Collection().GetEntry(widget);
				DENGINE_IMPL_GUI_ASSERT(temp.HasValue());
				return temp.Value();
			}
			[[nodiscard]] It GetEntry(Layer const& layer) noexcept {
				auto temp = Collection().GetEntry(layer);
				DENGINE_IMPL_GUI_ASSERT(temp.HasValue());
				return temp.Value();
			}

			void SetRectPair(It const& it, RectPair const& rect) {
				Collection().SetRect(it, rect);
			}
			[[nodiscard]] RectPair const* RectPair(Widget const& widget) const {
				return Collection().GetRect(widget);
			}

			[[nodiscard]] SizeHint const& GetSizeHint(It const& it) const {
				return Collection().GetSizeHint(it);
			}
			[[nodiscard]] SizeHint const* GetSizeHint(Widget const& widget) const {
				return Collection().GetSizeHint(widget);
			}
			[[nodiscard]] SizeHint const* GetSizeHint(Layer const& layer) const {
				return Collection().GetSizeHint(layer);
			}

			template<class T>
			[[nodiscard]] T* GetCustomData2(Widget const& widget) {
				return Collection().GetCustomData2<T>(widget);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData2(Widget const& widget) const {
				return Collection().GetCustomData2<T>(widget);
			}
			template<class T>
			[[nodiscard]] T* GetCustomData2(Layer const& layer) {
				return Collection().GetCustomData2<T>(layer);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData2(Layer const& layer) const {
				return Collection().GetCustomData2<T>(layer);
			}
			template<class T>
			[[nodiscard]] T* GetCustomData2(It const& it) {
				return Collection().GetCustomData2<T>(it);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData2(It const& it) const {
				return Collection().GetCustomData2<T>(it);
			}
			template<class T>
			T& AttachCustomData(It const& it, T&& input) {
				return Collection().AttachCustomData(it, static_cast<T&&>(input));
			}

			[[nodiscard]] bool IncludeRendering() const noexcept { return Collection().BuiltForRendering(); }

		private:
			[[nodiscard]] RectCollection& Collection() noexcept { return *collection; }
			[[nodiscard]] RectCollection const& Collection() const noexcept { return *collection; }
			RectCollection* collection = nullptr;
		};

		RectCollection() = default;
		RectCollection(RectCollection const&) = delete;
		RectCollection(RectCollection&&) = delete;

		RectCollection& operator=(RectCollection const&) = delete;
		RectCollection& operator=(RectCollection&&) = delete;

		It AddEntry(Widget const& widget) { return AddEntry(&widget); }
		It AddEntry(Layer const& layer) { return AddEntry(&layer); }
		[[nodiscard]] Std::Opt<It> GetEntry(Widget const& widget) const { return GetEntry(&widget); }
		[[nodiscard]] Std::Opt<It> GetEntry(Layer const& layer) const { return GetEntry(&layer); }


		void SetSizeHint(It const& it, SizeHint const& sizeHint) {
			DENGINE_IMPL_GUI_ASSERT(it.index < sizeHints.size());
			sizeHints[it.index] = sizeHint;
		}
		[[nodiscard]] SizeHint const& GetSizeHint(It const& it) const {
			DENGINE_IMPL_GUI_ASSERT(it.index < sizeHints.size());
			return sizeHints[it.index];
		}
		[[nodiscard]] SizeHint const* GetSizeHint(Widget const& widget) const { return GetSizeHint(&widget); }
		[[nodiscard]] SizeHint const* GetSizeHint(Layer const& layer) const { return GetSizeHint(&layer); }

		void SetRect(It const& it, RectPair const& rect) {
			DENGINE_IMPL_GUI_ASSERT(it.index < rects.size());
			rects[it.index] = rect;
		}
		[[nodiscard]] RectPair const* GetRect(Widget const& widget) const { return GetRect(&widget); }
		[[nodiscard]] RectPair const* GetRect(Layer const& layer) const { return GetRect(&layer); }
		[[nodiscard]] RectPair const& GetRect(It const& it) const {
			DENGINE_IMPL_GUI_ASSERT(it.index < rects.size());
			return rects[it.index];
		}

		template<class T>
		T& AttachCustomData(It const& it, T&& input)
			requires (Std::Trait::isMoveConstructible<T> && !Std::Trait::isConst<T>)
		{
			DENGINE_IMPL_GUI_ASSERT(it.index < widgets.size());
			DENGINE_IMPL_GUI_ASSERT(!customData2[it.index].ptr);

			// Technically this can fail. Should probably handle that...
			auto* returnPtr = static_cast<T*>(Alloc().Alloc(sizeof(T), alignof(T)));
			new(returnPtr, Std::placementNewTag) T(static_cast<T&&>(input));

			auto& customDataItem = customData2[it.index];
			// Assert that we don't already have some data here.
			DENGINE_IMPL_GUI_ASSERT(!customDataItem.ptr);
			customDataItem.ptr = returnPtr;
			customDataItem.destructorFn = [](void* ptr) { static_cast<T*>(ptr)->~T(); };

			return *returnPtr;
		}

		template<class T>
		[[nodiscard]] T const* GetCustomData2(Widget const& widget) const { return GetCustomData2<T>(&widget); }
		template<class T>
		[[nodiscard]] T* GetCustomData2(Widget const& widget) { return GetCustomData2<T>(&widget); }
		template<class T>
		[[nodiscard]] T const* GetCustomData2(It const& it) const
		{
			DENGINE_IMPL_GUI_ASSERT(it.index < customData2.size());
			return reinterpret_cast<T const*>(customData2[it.index].ptr);
		}

		[[nodiscard]] bool BuiltForRendering() const { return containsRendering; }

		// Makes it easier when debugging
		union PointerUnion
		{
			Widget const* widget;
			Layer const* layer;
			void const* voidPtr;
		};
		// Holds pointers to all relevant widgets in the hierarchy
		std::vector<PointerUnion> widgets;
		// Corresponds with the widgets vector.
		std::vector<SizeHint> sizeHints;
		// Corresponds with the widgets vector.
		std::vector<RectPair> rects;

		struct CustomData2
		{
			void* ptr = nullptr;
			using DestroyFnT = void(*)(void* ptr);
			DestroyFnT destructorFn = nullptr;
		};
		std::vector<CustomData2> customData2;

		void Prepare(bool includeRendering)
		{
			widgets.clear();
			sizeHints.clear();
			rects.clear();

			for (auto& item : customData2)
			{
				if (item.ptr)
				{
					item.destructorFn(item.ptr);
					alloc.Free(item.ptr);
				}
			}
			customData2.clear();

			containsRendering = includeRendering;

			alloc.Reset();
		}

		void Clear()
		{
			widgets.clear();
			sizeHints.clear();
			rects.clear();

			for (auto& item : customData2)
			{
				item.destructorFn(item.ptr);
				alloc.Free(item.ptr);
			}
			customData2.clear();
		}

		~RectCollection()
		{
			Clear();
		}

	protected:
		Std::FrameAlloc alloc;

		bool containsRendering = false;

		[[nodiscard]] Std::Opt<uSize> FindIndex(void const* ptr) const noexcept
		{
			constexpr auto invalidIndex = static_cast<uSize>(-1);
			auto index = invalidIndex;
			auto const widgetCount = widgets.size();
			for (uSize i = 0; i < widgetCount; i += 1)
			{
				if (widgets[i].voidPtr == ptr)
				{
					index = i;
					break;
				}
			}
			if (index == invalidIndex)
				return Std::nullOpt;
			else
				return index;
		}

		[[nodiscard]] bool PtrExists(void const* ptr) const noexcept
		{
			auto const widgetCount = widgets.size();
			for (uSize i = 0; i < widgetCount; i += 1)
			{
				if (widgets[i].voidPtr == ptr)
					return true;
			}
			return false;
		}

		[[nodiscard]] It AddEntry(void const* ptr)
		{
			// Check that the widget has not already been inserted
			DENGINE_IMPL_GUI_ASSERT(!PtrExists(ptr));

			widgets.emplace_back(PointerUnion{ .voidPtr = ptr });
			sizeHints.push_back({});
			rects.push_back({});
			customData2.push_back({});

			return It{ widgets.size() - 1 };
		}

		[[nodiscard]] Std::Opt<It> GetEntry(void const* ptr) const
		{
			// Check that the widget has not already been inserted
			auto temp = FindIndex(ptr);
			if (temp.HasValue())
				return It{ temp.Value() };
			else
				return Std::nullOpt;
		}

		[[nodiscard]] SizeHint const* GetSizeHint(void const* ptr) const
		{
			auto temp = FindIndex(ptr);
			if (temp.HasValue())
				return &sizeHints[temp.Value()];
			else
				return nullptr;
		}

		[[nodiscard]] RectPair const* GetRect(void const* ptr) const
		{
			auto temp = FindIndex(ptr);
			if (temp.HasValue())
				return &rects[temp.Value()];
			else
				return nullptr;
		}

		template<class T>
		[[nodiscard]] T* GetCustomData2(void const* ptr)
		{
			auto temp = FindIndex(ptr);
			if (temp.HasValue())
				return reinterpret_cast<T*>(customData2[temp.Value()].ptr);
			else
				return nullptr;
		}
		template<class T>
		[[nodiscard]] T* GetCustomData2(It const& it)
		{
			DENGINE_IMPL_GUI_ASSERT(it.index < customData2.size());
			return reinterpret_cast<T*>(customData2[it.index].ptr);
		}
		template<class T>
		[[nodiscard]] T const* GetCustomData2(void const* ptr) const
		{
			auto temp = FindIndex(ptr);
			if (temp.HasValue())
				return reinterpret_cast<T const*>(customData2[temp.Value()].ptr);
			else
				return nullptr;
		}
	};
}