#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/Utility.hpp>

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
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
		using AllocT = Std::FrameAlloc;
		AllocT& Alloc() noexcept { return alloc; }

		// Do NOT store over time. Only valid as
		// long as no entries have been added/removed from the collection.
		struct Iterator
		{
		private:
			explicit Iterator(uSize index) : index{ index } {}
			uSize index;

			friend RectCollection;
		};
		using It = Iterator;

		class SizeHintPusher
		{
		public:
			explicit SizeHintPusher(RectCollection& collection) noexcept :
				collection{ &collection }
			{}

			AllocT& Alloc() noexcept { return Collection().Alloc(); }

			It AddEntry(Widget const& widget) { return Collection().AddEntry(widget); }
			It AddEntry(Layer const& layer) { return Collection().AddEntry(layer); }
			void SetSizeHint(It const& it, SizeHint const& sizeHint)
			{
				return Collection().SetSizeHint(it, sizeHint);
			}
			It Push(Widget const& widget, SizeHint const& sizeHint)
			{
				return Collection().PushSizeHint(widget, sizeHint);
			}
			void Push(Layer const& layer, SizeHint const& sizeHint)
			{
				Collection().PushSizeHint(layer, sizeHint);
			}

			Std::ByteSpan AttachCustomData(It const& it, uSize customDataSize)
			{
				return Collection().AttachCustomData(it, customDataSize);
			}
			template<class T>
			T* AttachCustomData(It const& it) requires Std::Trait::isDefaultConstructible<T>
			{
				return Collection().AttachCustomData<T>(it);
			}

			template<class T>
			T& AttachCustomData(It const& it, T&& input) requires Std::Trait::isMoveConstructible<T>
			{
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
			explicit RectPusher(RectCollection& collection) noexcept :
				collection{ &collection }
			{}

			[[nodiscard]] It GetEntry(Widget const& widget) noexcept
			{
				return Collection().GetEntry(widget);
			}
			[[nodiscard]] It GetEntry(Layer const& layer) noexcept
			{
				return Collection().GetEntry(layer);
			}

			void Push(Widget const& widget, RectPair const& rect)
			{
				Collection().PushRect(widget, rect);
			}
			void Push(Layer const& layer, RectPair const& rect)
			{
				Collection().PushRect(layer, rect);
			}

			[[nodiscard]] auto const& GetSizeHint(Widget const& widget) const
			{
				return Collection().GetSizeHint(widget);
			}
			[[nodiscard]] auto const& GetSizeHint(Layer const& layer) const
			{
				return Collection().GetSizeHint(layer);
			}

			[[nodiscard]] Std::Span<char> GetCustomData(Widget const& widget) { return Collection().GetCustomData(widget); }
			template<class T>
			[[nodiscard]] T* GetCustomData2(Widget const& widget)
			{
				return Collection().GetCustomData2<T>(widget);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData2(Widget const& widget) const
			{
				return Collection().GetCustomData2<T>(widget);
			}
			template<class T>
			[[nodiscard]] T* GetCustomData2(Layer const& layer)
			{
				return Collection().GetCustomData2<T>(layer);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData2(Layer const& layer) const
			{
				return Collection().GetCustomData2<T>(layer);
			}
			template<class T>
			[[nodiscard]] T* GetCustomData2(It const& it)
			{
				return Collection().GetCustomData2<T>(it);
			}
			template<class T>
			[[nodiscard]] T const* GetCustomData2(It const& it) const
			{
				return Collection().GetCustomData2<T>(it);
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
		It GetEntry(Widget const& widget) const { return GetEntry(&widget); }
		It GetEntry(Layer const& layer) const { return GetEntry(&layer); }
		void SetSizeHint(It const& it, SizeHint const& sizeHint)
		{
			DENGINE_IMPL_GUI_ASSERT(it.index < sizeHints.size());

			sizeHints[it.index] = sizeHint;
		}

		It PushSizeHint(Widget const& widget, SizeHint const& sizeHint) { return PushSizeHint(&widget, sizeHint); }
		It PushSizeHint(Layer const& layer, SizeHint const& sizeHint) { return PushSizeHint(&layer, sizeHint); }
		[[nodiscard]] SizeHint const& GetSizeHint(Widget const& widget) const { return GetSizeHint(&widget); }
		[[nodiscard]] SizeHint const& GetSizeHint(Layer const& layer) const { return GetSizeHint(&layer); }

		void PushRect(Widget const& widget, RectPair const& rect) { return PushRect(&widget, rect); }
		void PushRect(Layer const& layer, RectPair const& rect) { return PushRect(&layer, rect); }
		[[nodiscard]] RectPair const& GetRect(Widget const& widget) const { return GetRect(&widget); }
		[[nodiscard]] RectPair const& GetRect(Layer const& layer) const { return GetRect(&layer); }

		[[nodiscard]] Std::ByteSpan AttachCustomData(It const& it, uSize customDataSize)
		{
			DENGINE_IMPL_GUI_ASSERT(customDataSize != 0);
			DENGINE_IMPL_GUI_ASSERT(it.index < widgets.size());

			auto const unalignedOffset = customData.size();
			auto const alignedOffset = Math::CeilToMultiple(unalignedOffset, customDataAlignment);
			DENGINE_IMPL_GUI_ASSERT((uSize)(customData.data() + alignedOffset) % customDataAlignment == 0);
			// Resize custom data vector
			customData.resize(alignedOffset + customDataSize);

			auto& dataView = customDataViews[it.index];
			dataView.offset = alignedOffset;
			dataView.size = customDataSize;

			return { customData.data() + dataView.offset, dataView.size };
		}

		template<class T>
		T& AttachCustomData(It const& it, T&& input) requires Std::Trait::isMoveConstructible<T>
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

		[[nodiscard]] Std::Span<char> GetCustomData(Widget const& widget) { return GetCustomData(&widget); }
		[[nodiscard]] Std::Span<char const> GetCustomData(Widget const& widget) const { return GetCustomData(&widget); }
		template<class T>
		[[nodiscard]] T const* GetCustomData2(Widget const& widget) const { return GetCustomData2<T>(&widget); }
		template<class T>
		[[nodiscard]] T* GetCustomData2(Widget const& widget) { return GetCustomData2<T>(&widget); }
		template<class T>
		[[nodiscard]] T const* GetCustomData2(It const& it) const
		{
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



		struct CustomDataView
		{
			uSize offset = 0;
			uSize size = 0;
		};
		// Corresponds with the widgets vector.
		std::vector<CustomDataView> customDataViews;

		using CustomDataT = char;
		static constexpr uSize customDataAlignment = 8;
		std::vector<CustomDataT> customData;

		void Prepare(bool includeRendering)
		{
			widgets.clear();
			sizeHints.clear();
			rects.clear();
			customDataViews.clear();
			customData.clear();

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
			customDataViews.clear();
			customData.clear();

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

		[[nodiscard]] uSize FindIndex(void const* ptr) const noexcept
		{
			auto index = static_cast<uSize>(-1);
			auto const widgetCount = widgets.size();
			for (uSize i = 0; i < widgetCount; i += 1)
			{
				if (widgets[i].voidPtr == ptr)
				{
					index = i;
					break;
				}
			}
			DENGINE_IMPL_GUI_ASSERT(index != -1);
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

			customDataViews.push_back({});
			return It{ widgets.size() - 1 };
		}

		[[nodiscard]] It GetEntry(void const* ptr) const
		{
			// Check that the widget has not already been inserted
			DENGINE_IMPL_GUI_ASSERT(PtrExists(ptr));
			return It{ FindIndex(ptr) };
		}

		// Returns the buffer of custom data, if requested.
		It PushSizeHint(void const* ptr, SizeHint const& sizeHint)
		{
			// Check that the widget has not already been inserted
			DENGINE_IMPL_GUI_ASSERT(!PtrExists(ptr));

			widgets.emplace_back(PointerUnion{ .voidPtr = ptr });
			sizeHints.emplace_back(sizeHint);
			rects.push_back({});
			customData2.push_back({});

			customDataViews.push_back({});
			return It{ sizeHints.size() - 1 };
		}

		[[nodiscard]] SizeHint const& GetSizeHint(void const* ptr) const
		{
			return sizeHints[FindIndex(ptr)];
		}

		void PushRect(void const* ptr, RectPair const& rect)
		{
			rects[FindIndex(ptr)] = rect;
		}

		[[nodiscard]] RectPair const& GetRect(void const* ptr) const
		{
			return rects[FindIndex(ptr)];
		}

		[[nodiscard]] Std::Span<char> GetCustomData(void const* ptr)
		{
			auto const& view = customDataViews[FindIndex(ptr)];
			return {
				customData.data() + view.offset,
				view.size };
		}

		[[nodiscard]] Std::Span<char const> GetCustomData(void const* ptr) const
		{
			auto const& view = customDataViews[FindIndex(ptr)];
			return {
				customData.data() + view.offset,
				view.size };
		}

		template<class T>
		[[nodiscard]] T* GetCustomData2(void const* ptr)
		{
			return reinterpret_cast<T*>(customData2[FindIndex(ptr)].ptr);
		}
		template<class T>
		[[nodiscard]] T* GetCustomData2(It const& it)
		{
			return reinterpret_cast<T*>(customData2[it.index].ptr);
		}
		template<class T>
		[[nodiscard]] T const* GetCustomData2(void const* ptr) const
		{
			return reinterpret_cast<T const*>(customData2[FindIndex(ptr)].ptr);
		}

	};
}