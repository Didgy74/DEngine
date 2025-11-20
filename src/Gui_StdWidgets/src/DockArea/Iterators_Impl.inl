#pragma once

namespace DEngine::Gui::impl
{
	template<class DA_T>
	Rect DA_Layer_It_Result_Inner<DA_T>::BuildLayerRect(Rect const& widgetRect) const {
		auto const& layers = DA_GetLayers(dockArea);
		return DA_BuildLayerRect(
			{ layers.data(), layers.size() },
			layerIndex,
			widgetRect);
	}

	template<class DA_T>
	DA_Layer_It_Result_Inner<DA_T>::DA_Layer_It_Result_Inner(
		DA_T& dockArea,
		int layerIndex,
		int layerCount) :
			dockArea{ dockArea },
			ppParentPtrToNode { &DA_GetLayers(dockArea)[layerIndex].root },
			node { **ppParentPtrToNode },
			layerIndex { layerIndex },
			layerCount { layerCount } {}

	template<class DA_T>
	DA_LayerIt_Inner<DA_T> DA_LayerItPair_Inner<DA_T>::begin() const {
		DA_LayerIt_Inner<DA_T> returnVal { .dockArea = dockArea };
		returnVal.currIndex = startIndex;
		returnVal.endIndex = endIndex;
		returnVal.reverse = reverse;
		returnVal.layerCount = (int)DA_GetLayers(dockArea).size();
		return returnVal;
	}

	template<class DA_T, bool includeRect>
	void DA_NodeIt_Inner<DA_T, includeRect>::Increment()
	{
		DENGINE_IMPL_GUI_ASSERT(nextNodePtr);
		NodePtrPtrT<DA_T> tempNodePtr = nextNodePtr;
		Rect tempRect = nextRect;

		// If we are a split-node, we want to move down its
		// A branch.
		if ((*tempNodePtr)->GetNodeType() == NodeType::Split)
		{
			auto& currSplitNode = static_cast<SplitNodeT<DA_T>&>(**tempNodePtr);
			StackElement newElement = {};
			newElement.nodePtr = tempNodePtr;
			newElement.rect = tempRect;
			stack.PushBack(newElement);
			tempNodePtr = &currSplitNode.a;
			// Create the rect of this child
			auto childRects = currSplitNode.BuildChildRects(tempRect);
			tempRect = childRects[0];
		}
		else
		{
			// We are a windownode. Jump upwards,
			// if we are the parents A branch, we want to go into its B branch.

			StackElement prevStackElement = {};
			SplitNodeT<DA_T>* prevSplitNode = nullptr;
			while (!stack.Empty())
			{
				prevStackElement = stack[stack.Size() - 1];
				prevSplitNode = static_cast<SplitNodeT<DA_T>*>(*prevStackElement.nodePtr);
				if (tempNodePtr == &prevSplitNode->a)
				{
					tempNodePtr = &prevSplitNode->b;
					// Create the rect of this child
					auto childRects = prevSplitNode->BuildChildRects(prevStackElement.rect);
					tempRect = childRects[1];
					break;
				}
				else
				{
					DENGINE_IMPL_GUI_ASSERT(tempNodePtr == &prevSplitNode->b);
					tempNodePtr = prevStackElement.nodePtr;
					tempRect = prevStackElement.rect;
					stack.EraseBack();
				}
			}
			if (stack.Empty())
				tempNodePtr = nullptr;
		}

		nextNodePtr = tempNodePtr;
		nextRect = tempRect;
	}

	template<class DA_T, bool includeRect>
	void DA_NodeIt_Inner2<DA_T, includeRect>::Increment()
	{
		DENGINE_IMPL_GUI_ASSERT(nextNodePtr);
		NodePtrT<DA_T> tempNodePtr = nextNodePtr;
		Rect tempRect = nextRect;

		// If we are a split-node, we want to move down its
		// A branch.
		if (tempNodePtr->GetNodeType() == NodeType::Split)
		{
			auto& currSplitNode = static_cast<SplitNodeT<DA_T>&>(*tempNodePtr);
			StackElement newElement = {};
			newElement.nodePtr = tempNodePtr;
			newElement.rect = tempRect;
			stack.PushBack(newElement);
			tempNodePtr = currSplitNode.a;
			// Create the rect of this child
			auto childRects = currSplitNode.BuildChildRects(tempRect);
			tempRect = childRects[0];
		}
		else
		{
			// We are a windownode. Jump upwards,
			// if we are the parents A branch, we want to go into its B branch.
			StackElement prevStackElement = {};
			SplitNodeT<DA_T>* prevSplitNode = nullptr;
			while (!stack.Empty())
			{
				prevStackElement = stack[stack.Size() - 1];
				prevSplitNode = static_cast<SplitNodeT<DA_T>*>(prevStackElement.nodePtr);
				if (tempNodePtr == prevSplitNode->a)
				{
					tempNodePtr = prevSplitNode->b;
					// Create the rect of this child
					auto childRects = prevSplitNode->BuildChildRects(prevStackElement.rect);
					tempRect = childRects[1];
					break;
				}
				else
				{
					DENGINE_IMPL_GUI_ASSERT(tempNodePtr == prevSplitNode->b);
					tempNodePtr = prevStackElement.nodePtr;
					tempRect = prevStackElement.rect;
					stack.EraseBack();
				}
			}
			if (stack.Empty())
				tempNodePtr = nullptr;
		}

		nextNodePtr = tempNodePtr;
		nextRect = tempRect;
	}
}