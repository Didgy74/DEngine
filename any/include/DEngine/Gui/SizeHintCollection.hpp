#pragma once

#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/Utility.hpp>

#include <vector>

namespace DEngine::Gui
{
	class Widget;

	struct RectCombo
	{
		Rect widgetRect = {};
		Rect visibleRect = {};
	};

	class SizeHintCollection
	{
	public:
		class Pusher
		{
		public:
			explicit Pusher(SizeHintCollection& collection) noexcept :
				collection{ &collection }
			{}

			void PushSizeHint(Widget const& widget, SizeHint const& sizeHint)
			{
				collection->PushSizeHint(widget, sizeHint);
			}
		private:
			SizeHintCollection* collection = nullptr;
		};

		class Pusher2
		{
		public:
			explicit Pusher2(SizeHintCollection& collection) noexcept :
				collection{ &collection }
			{}

			void PushRect(Widget const& widget, RectCombo const& rect)
			{
				collection->PushRect(widget, rect);
			}

			[[nodiscard]] SizeHint const& GetSizeHint(Widget const& widget) const
			{
				return collection->GetSizeHint(widget);
			}

		private:
			SizeHintCollection* collection = nullptr;
		};

		void PushSizeHint(Widget const& widget, SizeHint const& sizeHint)
		{
			widgets.push_back(&widget);
			sizeHints.push_back(sizeHint);
		}

		[[nodiscard]] SizeHint const& GetSizeHint(Widget const& widget) const
		{
			auto index = static_cast<uSize>(-1);
			for (uSize i = 0; i < widgets.size(); i += 1)
			{
				if (widgets[i] == &widget)
				{
					index = i;
					break;
				}
			}
			DENGINE_IMPL_GUI_ASSERT(index != -1);
			return sizeHints[index];
		}

		void PushRect(Widget const& widget, RectCombo const& rect)
		{
			auto index = static_cast<uSize>(-1);
			for (uSize i = 0; i < widgets.size(); i += 1)
			{
				if (widgets[i] == &widget)
				{
					index = i;
					break;
				}
			}
			DENGINE_IMPL_GUI_ASSERT(index != -1);

			rects[index] = rect;
		}

		[[nodiscard]] RectCombo const& GetRect(Widget const& widget) const
		{
			auto index = static_cast<uSize>(-1);
			for (uSize i = 0; i < widgets.size(); i += 1)
			{
				if (widgets[i] == &widget)
				{
					index = i;
					break;
				}
			}
			DENGINE_IMPL_GUI_ASSERT(index != -1);

			return rects[index];
		}

		std::vector<Widget const*> widgets;
		std::vector<SizeHint> sizeHints;
		std::vector<RectCombo> rects;

		void Clear()
		{
			widgets.clear();
			sizeHints.clear();
			rects.clear();
		}
	};

	struct RectCollection
	{
		explicit RectCollection(SizeHintCollection const& in) noexcept :
			collection{ &in }
		{}

		[[nodiscard]] RectCombo const& GetRect(Widget const& widget) const
		{
			return collection->GetRect(widget);
		}

	private:
		SizeHintCollection const* collection = nullptr;
	};
}