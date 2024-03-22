#include "BackendData.hpp"

using namespace DEngine::Application::impl::Backend;

HRESULT TestAccessProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) {
	if (pRetVal == nullptr) {
		return E_INVALIDARG;
	}

	POINT offset = {};
	ScreenToClient(this->Win32Hwnd(), &offset);
	x += offset.x;
	y += offset.y;

	// Check if we hit any item
	for (int i = 0; i < (int)this->children.size(); i++) {
		auto* child = this->children[i];

		bool isInside = true;
		auto rectOpt = child->GetRect();
		if (!rectOpt.Has()) {
			continue;
		}

		auto const& rect = rectOpt.Get();
		isInside = x >= rect.posX;
		isInside = isInside && x < rect.posX + rect.width;
		isInside = isInside && y >= rect.posY;
		isInside = isInside && y < rect.posY + rect.height;
		if (isInside) {
			child->AddRef();
			*pRetVal = child;
			return S_OK;
		}
	}
	*pRetVal = nullptr;
	return S_OK;
}

HRESULT TestAccessProvider::Navigate(
	NavigateDirection direction,
	IRawElementProviderFragment** pRetVal)
{
	if (pRetVal == nullptr) return E_INVALIDARG;

	if (direction == NavigateDirection_FirstChild) {
		if (!this->children.empty()) {
			*pRetVal = this->children[0];
		}
	} else if (direction == NavigateDirection_LastChild) {
		if (!this->children.empty()) {
			*pRetVal = this->children[this->children.size() - 1];
		}
	}
	if (*pRetVal != nullptr) {
		(*pRetVal)->AddRef();
		return S_OK;
	}

	*pRetVal = nullptr;
	return S_OK;
}