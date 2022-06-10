#pragma once

#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/AllocRef.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Gui/impl/Assert.hpp>

#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/FrameAllocator.hpp>

namespace DEngine::Gui::impl
{
	// Get const or non-const DA_Node type based on input const
	template<class T>
	using LayerT = Std::Trait::Cond<Std::Trait::isConst<T>, Layer const, Layer>;
	template<class T>
	using NodeT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_Node const, DA_Node>;
	template<class T>
	using NodePtrT = Std::Trait::Cond<Std::Trait::isConst<T>, DA_Node const*, DA_Node*>;
	template<class T>
	using NodePtrPtrT =
		Std::Trait::Cond<Std::Trait::isConst<T>, DA_Node const* const*, DA_Node**>;
	template<class T>
	using SplitNodeT =
		Std::Trait::Cond<Std::Trait::isConst<T>, DA_SplitNode const, DA_SplitNode>;
	template<class T>
	using SplitNodePtrT =
		Std::Trait::Cond<Std::Trait::isConst<T>, DA_SplitNode const*, DA_SplitNode const>;
	template<class T>
	using WindowNodeT =
		Std::Trait::Cond<Std::Trait::isConst<T>, DA_WindowNode const, DA_WindowNode>;
}

namespace DEngine::Gui::impl
{
	struct DA_Layer_EndIt {};

	// DO NOT USE DIRECTLY
	template<class DA_T>
	struct DA_Layer_It_Result_Inner
	{
		DA_T& dockArea;
		NodePtrPtrT<DA_T> ppParentPtrToNode;
		NodeT<DA_T>& node;
		int layerIndex = 0;
		// Needs to be here for checking if we are the back layer.
		int layerCount = 0;

		[[nodiscard]] Rect BuildLayerRect(Rect const& widgetRect) const;

		DA_Layer_It_Result_Inner(DA_T& dockArea, int layerIndex, int layerCount);

		DA_Layer_It_Result_Inner(DA_Layer_It_Result_Inner<Std::Trait::RemoveConst<DA_T>> const& in)
				requires(Std::Trait::isConst<DA_T>) :
			dockArea { in.dockArea },
			ppParentPtrToNode { in.ppParentPtrToNode },
			node{ in.node },
			layerIndex{ in.layerIndex },
			layerCount{ in.layerCount } {}
	};
	using DA_Layer_It_Result = DA_Layer_It_Result_Inner<DockArea>;
	using DA_Layer_ConstIt_Result = DA_Layer_It_Result_Inner<DockArea const>;


	// DO NOT USE DIRECTLY
	template<class DA_T>
	struct DA_LayerIt_Inner
	{
		DA_T& dockArea;

		int layerCount = 0;
		int currIndex = 0;
		int endIndex = 0;
		bool reverse = false;

		~DA_LayerIt_Inner() = default;

		[[nodiscard]] auto operator*() const noexcept {
			return DA_Layer_It_Result_Inner<DA_T>{ dockArea, currIndex, layerCount };
		}

		[[nodiscard]] bool operator!=(DA_Layer_EndIt const& other) const noexcept {
			if (!reverse)
				return currIndex < endIndex;
			else
				return currIndex > endIndex;
		}

		void operator++() noexcept {
			if (!reverse)
				currIndex += 1;
			else
				currIndex -= 1;
		}
	};

	// DO NOT USE DIRECTLY
	template<class DA_T>
	struct DA_LayerItPair_Inner
	{
		DA_T& dockArea;
		uSize startIndex = 0;
		uSize endIndex = 0;
		bool reverse = false;

		[[nodiscard]] auto Reverse() const noexcept {
			DA_LayerItPair_Inner returnVal = *this;
			returnVal.startIndex = endIndex - 1;
			returnVal.endIndex = startIndex - 1;
			returnVal.reverse = !reverse;
			return returnVal;
		}

		[[nodiscard]] DA_LayerIt_Inner<DA_T> begin() const;

		[[nodiscard]] auto end() const noexcept {
			return DA_Layer_EndIt{};
		}
	};

	using DA_LayerItPair = DA_LayerItPair_Inner<DockArea>;
	using DA_LayerConstItPair = DA_LayerItPair_Inner<DockArea const>;

	// Iterates front to back
	[[nodiscard]] DA_LayerItPair DA_BuildLayerItPair(DockArea& dockArea) noexcept;
	// Iterates front to back
	[[nodiscard]] DA_LayerConstItPair DA_BuildLayerItPair(DockArea const& dockArea) noexcept;
}

namespace DEngine::Gui::impl
{
	struct DA_Node_EndIt {};

	template<class DA_T, bool includeRect>
	struct DA_NodeIt_Result_Inner
	{
		NodePtrPtrT<DA_T> parentPtrToNode;
		NodeT<DA_T>& node;
		Rect rect;
		// This tells us if we are the topmost node.
		// In simpler terms, it means this node has a SplitNode parent.
		bool hasSplitNodeParent = false;

		explicit DA_NodeIt_Result_Inner(
			NodePtrPtrT<DA_T> parentPtr,
			bool hasSplitNodeParent,
			Rect const& rect) :
				parentPtrToNode { parentPtr },
				node { **parentPtrToNode },
				rect { rect },
				hasSplitNodeParent { hasSplitNodeParent } {}
	};

	template<class DA_T>
	struct DA_NodeIt_Result_Inner<DA_T, false>
	{
		NodePtrPtrT<DA_T> parentPtrToNode;
		NodeT<DA_T>& node;
		bool hasSplitNodeParent = false;

		explicit DA_NodeIt_Result_Inner(NodePtrPtrT<DA_T> parentPtr, bool hasSplitNodeParent) :
			parentPtrToNode{ parentPtr },
			node{ **parentPtr },
			hasSplitNodeParent { hasSplitNodeParent } {}
	};

	// DO NOT USE DIRECTLY
	template<class DA_T, bool includeRect>
	struct DA_NodeIt_Inner
	{
		struct StackElement
		{
			// This always points to a split node.
			NodePtrPtrT<DA_T> nodePtr;
			Rect rect;
		};
		Std::Vec<StackElement, AllocRef> stack;
		Rect layerRect = {};
		NodePtrPtrT<DA_T> nextNodePtr = nullptr;
		Rect nextRect = {};

		explicit DA_NodeIt_Inner(
			NodePtrPtrT<DA_T> topNode,
			Rect layerRectIn,
			AllocRef transientAlloc) :
			stack{ transientAlloc },
			layerRect{ layerRectIn }
		{
			// Just reserve some memory.
			stack.Reserve(5);

			NodePtrPtrT<DA_T> tempNodePtr = topNode;
			Rect tempRect = layerRect;

			nextNodePtr = tempNodePtr;
			nextRect = tempRect;
		}

		[[nodiscard]] auto operator*() const noexcept {
			bool hasSplitNodeParent = !stack.Empty();
			if constexpr (includeRect)
				return DA_NodeIt_Result_Inner<DA_T, true>{
					nextNodePtr,
					hasSplitNodeParent,
					nextRect };
			else
				return DA_NodeIt_Result_Inner<DA_T, false>{
					nextNodePtr,
					hasSplitNodeParent };
		}

		[[nodiscard]] bool operator!=(DA_Node_EndIt const& other) const noexcept {
			// If our pointer is valid, keep iterating
			return nextNodePtr;
		}

		void operator++() noexcept
		{
			Increment();
		}

		void Increment();
	};

	template<class T, bool includeRect>
	struct DA_NodeIt_Pair_Inner
	{
		NodePtrPtrT<T> rootNode;
		Rect rootRect = {};
		AllocRef transientAlloc;

		[[nodiscard]] auto begin() const noexcept {
			return DA_NodeIt_Inner<T, includeRect>{ rootNode, rootRect, transientAlloc };
		}

		[[nodiscard]] auto end() const noexcept {
			return DA_Node_EndIt{};
		}
	};

	template<bool includeRect>
	using DA_Node_ItPair = DA_NodeIt_Pair_Inner<DA_Node, includeRect>;
	template<bool includeRect>
	using DA_Node_ConstItPair = DA_NodeIt_Pair_Inner<DA_Node const, includeRect>;

	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node** rootNode,
		Rect rootRect,
		AllocRef transientAlloc)
	{
		return DA_Node_ItPair<true>{
			.rootNode = rootNode,
			.rootRect = rootRect,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node** rootNode,
		AllocRef transientAlloc)
	{
		return DA_Node_ItPair<false>{
			.rootNode = rootNode,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node const* const* rootNode,
		Rect rootRect,
		AllocRef transientAlloc)
	{
		return DA_Node_ConstItPair<true>{
			.rootNode = rootNode,
			.rootRect = rootRect,
			.transientAlloc = transientAlloc };
	}

	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node const *const *rootNode,
		AllocRef transientAlloc)
	{
		return DA_Node_ConstItPair<false>{
			.rootNode = rootNode,
			.transientAlloc = transientAlloc };
	}







	template<class DA_T, bool includeRect>
	struct DA_NodeIt_Result_Inner2
	{
		NodeT<DA_T>& node;
		Rect rect;
		// This tells us if we are the topmost node.
		// In simpler terms, it means this node has a SplitNode parent.
		bool hasSplitNodeParent = false;

		explicit DA_NodeIt_Result_Inner2(
			NodePtrT<DA_T> nodePtr,
			bool hasSplitNodeParent,
			Rect const& rect) :
			node { *nodePtr },
			rect { rect },
			hasSplitNodeParent { hasSplitNodeParent } {}
	};

	template<class DA_T>
	struct DA_NodeIt_Result_Inner2<DA_T, false>
	{
		NodeT<DA_T>& node;
		bool hasSplitNodeParent = false;

		explicit DA_NodeIt_Result_Inner2(
			NodePtrT<DA_T> nodePtr,
			bool hasSplitNodeParent) :
			node{ *nodePtr },
			hasSplitNodeParent { hasSplitNodeParent } {}
	};
	// DO NOT USE DIRECTLY
	template<class DA_T, bool includeRect>
	struct DA_NodeIt_Inner2
	{
		struct StackElement
		{
			NodePtrT<DA_T> nodePtr;
			Rect rect;
		};
		Std::Vec<StackElement, AllocRef> stack;
		Rect layerRect = {};
		NodePtrT<DA_T> nextNodePtr = nullptr;
		Rect nextRect = {};

		explicit DA_NodeIt_Inner2(
			NodePtrT<DA_T> topNode,
			Rect layerRectIn,
			AllocRef const& transientAlloc) :
			stack{ transientAlloc },
			layerRect{ layerRectIn }
		{
			// Just reserve some memory.
			stack.Reserve(5);

			nextNodePtr = topNode;
			nextRect = layerRect;
		}

		[[nodiscard]] auto operator*() const noexcept {
			bool hasSplitNodeParent = !stack.Empty();
			if constexpr (includeRect)
				return DA_NodeIt_Result_Inner2<DA_T, true>{
					nextNodePtr,
					hasSplitNodeParent,
					nextRect };
			else
				return DA_NodeIt_Result_Inner2<DA_T, false>{
					nextNodePtr,
					hasSplitNodeParent };
		}

		[[nodiscard]] bool operator!=(DA_Node_EndIt const& other) const noexcept {
			// If our pointer is valid, keep iterating
			return nextNodePtr;
		}

		void operator++() noexcept
		{
			Increment();
		}

		void Increment();
	};
	template<class T, bool includeRect>
	struct DA_NodeIt_Pair_Inner2
	{
		NodePtrT<T> rootNode;
		Rect rootRect = {};
		AllocRef transientAlloc;
		[[nodiscard]] auto begin() const noexcept {
			return DA_NodeIt_Inner2<T, includeRect>{
				rootNode,
				rootRect,
				transientAlloc };
		}
		[[nodiscard]] auto end() const noexcept {
			return DA_Node_EndIt{};
		}
	};
	template<bool includeRect>
	using DA_Node_ItPair2 = DA_NodeIt_Pair_Inner2<DA_Node, includeRect>;
	template<bool includeRect>
	using DA_Node_ConstItPair2 = DA_NodeIt_Pair_Inner2<DA_Node const, includeRect>;
	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node const& rootNode,
		Rect const& rootRect,
		AllocRef transientAlloc)
	{
		return DA_Node_ConstItPair2<true>{
			.rootNode = &rootNode,
			.rootRect = rootRect,
			.transientAlloc = transientAlloc };
	}
	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node& rootNode,
		Rect const& rootRect,
		AllocRef transientAlloc)
	{
		return DA_Node_ItPair2<true>{
			.rootNode = &rootNode,
			.rootRect = rootRect,
			.transientAlloc = transientAlloc };
	}
	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node const& rootNode,
		AllocRef transientAlloc)
	{
		return DA_Node_ConstItPair2<false>{
			.rootNode = &rootNode,
			.transientAlloc = transientAlloc };
	}
	[[nodiscard]] inline auto DA_BuildNodeItPair(
		DA_Node& rootNode,
		AllocRef transientAlloc)
	{
		return DA_Node_ItPair2<false>{
			.rootNode = &rootNode,
			.transientAlloc = transientAlloc };
	}
}