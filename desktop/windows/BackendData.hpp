#pragma once

#include <DEngine/impl/Application.hpp>
#include <DEngine/Std/Trait.hpp>

#include <Windows.h>
#include <winuser.h>
#include <uiautomation.h>
#include <UIAutomationCore.h>
#include <msctf.h>
#include <Inputscope.h>
#include <textstor.h>
#include <tsattrs.h>
#include <olectl.h>
#include <Richedit.h>

#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <future>
#include <format>
#include <iostream>


namespace DEngine::Application::impl::Backend {
	enum class HResult_Helper {
		Ok = S_OK,
		E_InvalidArg = E_INVALIDARG,
		E_Pointer = E_POINTER,
		E_NoInterface = E_NOINTERFACE,
		REGDB_E_ClassNotReg = REGDB_E_CLASSNOTREG,
		Class_E_NoAggregation = CLASS_E_NOAGGREGATION,
		E_AccessDenied = E_ACCESSDENIED,
		E_Unexpected = E_UNEXPECTED,
	};

	enum class EditStyleFlag_Helper {
		UseCtf = SES_USECTF,
		Ex_MultiTouch = SES_EX_MULTITOUCH,
	};

	struct PerWindowData;
	struct TextStoreTest;
	struct TestAccessProvider;
	struct TestAccessFragment;

	enum class CustomMessageEnum {
		CustomMessage = WM_APP + 1,
	};

	struct CustomJobItem {
		u64 id = 0;
		void* data = nullptr;
		using ConsumeFnT = void (*)(void*, Context::Impl&);
		ConsumeFnT consumeFn = nullptr;
	};

	struct BackendData {
		using UiaReturnRawElementProvider_FnT = LRESULT(WINAPI*)(HWND hwnd, WPARAM wParam, LPARAM lParam, IRawElementProviderSimple* el);
		using UiaHostProviderFromHwnd_FnT = HRESULT(WINAPI*)(HWND hwnd, IRawElementProviderSimple** ppProvider);
		struct UiAutomationFnPtrs {
			UiaReturnRawElementProvider_FnT UiaReturnRawElementProvider = nullptr;
			UiaHostProviderFromHwnd_FnT UiaHostProviderFromHwnd = nullptr;
		};
		UiAutomationFnPtrs uiAutomationFnPtrs = {};


		DWORD winMainThreadId_ = 0;

		// Thread safe, this is set during init and never changed.
		auto WinMainThreadId_ThreadSafe() const noexcept { return winMainThreadId_; }

		std::promise<Context::Impl*> initWaitPromise;

		// We need some kind of job system
		std::mutex eventJobsLock;
		Std::FnScratchList<Context&> queuedEventCallbacks;

		std::mutex customJobsLock;
		struct CustomJobs {
			u64 customJobIdTracker = 0;
			std::vector<CustomJobItem> items;
		};
		CustomJobs customJobs;

		HINSTANCE instanceHandle = {};
		int nCmdShow = {};
		ATOM mainWindowClass = {};
		void* pfn_vkGetInstanceProcAddr = nullptr;

		// TSF stuff
		ITfThreadMgr* threadMgr = nullptr;
		ITfThreadMgr2* threadMgr2 = nullptr;
		TfClientId clientId = {};
		ITfDocumentMgr* docMgr = nullptr;
		TextStoreTest* textStore = nullptr;
	};

	template<class CallableT>
	inline void PushEventJob_ThreadSafe(
		BackendData& backendData,
		CallableT&& callable)
	{
		std::scoped_lock lock { backendData.eventJobsLock };
		backendData.queuedEventCallbacks.Push(Std::Move(callable));
	}

	// Every window holds a pointer to an instance of this struct in their extra data
	// area.
	struct PerWindowData : public WindowBackendData {
		std::vector<AccessibilityUpdateElement> accessElements;
		std::string accessText;
		HWND hwnd = {};
		HMONITOR win32monitor = {};

		TestAccessProvider* accessProvider = nullptr;

		[[nodiscard]] void* GetRawHandle() override { return hwnd; }

		[[nodiscard]] void const* GetRawHandle() const override { return hwnd; }
	};




	struct TestAccessProvider :
		public IRawElementProviderSimple,
		public IRawElementProviderFragmentRoot,
		public IRawElementProviderFragment
	{
		PerWindowData* perWindowData = nullptr;
		BackendData* pBackendData = nullptr;
		ULONG m_refCount = 1; // Start with one reference
		std::vector<TestAccessFragment*> children;

		[[nodiscard]] auto Win32Hwnd() const { return perWindowData->hwnd; }
		[[nodiscard]] auto const& BackendData() const { return *this->pBackendData; }


		//
		// IUnknown start
		//
		HRESULT QueryInterface(REFIID riid, void** ppvObject) override {
			DENGINE_IMPL_ASSERT(__uuidof(IUnknown) == IID_IUnknown);
			if (ppvObject == nullptr) {
				return E_INVALIDARG;
			}

			IUnknown* outPtr = nullptr;
			if (riid == IID_IUnknown) {
				outPtr = (IUnknown*) (IRawElementProviderFragment*) this;
			} else if (riid == IID_IRawElementProviderSimple) {
				outPtr = static_cast<IRawElementProviderSimple*>(this);
			} else if (riid == IID_IRawElementProviderFragmentRoot) {
				outPtr = static_cast<IRawElementProviderFragmentRoot*>(this);
			} else if (riid == IID_IRawElementProviderFragment) {
				outPtr = static_cast<IRawElementProviderFragment*>(this);
			}
			if (outPtr != nullptr) {
				outPtr->AddRef();
				*ppvObject = outPtr;
				return S_OK;
			}

			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}

		ULONG AddRef() override {
			return InterlockedIncrement(&m_refCount);
		}

		ULONG Release() override {
			long val = InterlockedDecrement(&m_refCount);
			if (val == 0) {
				DENGINE_IMPL_UNREACHABLE();
			}
			return val;
		}
		//
		// IUnknown end
		//

		HRESULT get_ProviderOptions(ProviderOptions* pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = ProviderOptions_ServerSideProvider;
			return S_OK;
		}

		HRESULT GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			} else if (propertyId == UIA_NamePropertyId) {
				// Return the name (label) of this control
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(L"Root");
				return S_OK;
			}

			pRetVal->vt = VT_EMPTY;
			return S_OK;
		}

		HRESULT get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override {
			return this->BackendData().uiAutomationFnPtrs.UiaHostProviderFromHwnd(Win32Hwnd(), pRetVal);
		}

		HRESULT GetFocus(IRawElementProviderFragment** pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) override;


		//
		// IRawElementProviderFragment
		//
		HRESULT Navigate(
			NavigateDirection direction,
			IRawElementProviderFragment** pRetVal) override;

		// UI Automation gets this value from the host window provider, so supply NULL here.
		/*
		 	Ref:
			https://github.com/microsoftarchive/msdn-code-gallery-microsoft/blob/master/
			Official%20Windows%20Platform%20Sample/UI%20Automation%20fragment%20provider%20sample/%5BC%2B%2B%5D-UI%20
		 	Automation%20fragment%20provider%20sample/C%2B%2B/ListProvider.cpp#L207
		 */
		HRESULT GetRuntimeId(SAFEARRAY** pRetVal) override {
			if (pRetVal == nullptr) return E_INVALIDARG;
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT get_BoundingRectangle(UiaRect* pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			POINT offset = {};

			auto result = ClientToScreen(this->Win32Hwnd(), &offset);
			if (!result) {
				// return some error.
			}

			RECT outRect = {};
			GetClientRect(this->Win32Hwnd(), &outRect);

			pRetVal->left = offset.x;
			pRetVal->top = offset.y;
			pRetVal->width = outRect.right;
			pRetVal->height = outRect.bottom;

			return S_OK;
		}

		HRESULT GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override {
			// This could should likely always return null for our implementation.
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT SetFocus() override {
			return S_OK;
		}

		// We need to ourselves since we are the root.
		HRESULT get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = static_cast<IRawElementProviderFragmentRoot*>(this);
			this->AddRef();
			return S_OK;
		}
		//
		// IRawElementProviderFragment end
		//

	};

	struct TestAccessFragment :
		public IRawElementProviderFragment,
		public IRawElementProviderSimple {
		TestAccessProvider* root = nullptr;
		int index = 0;

		ULONG m_refCount = 1; // Start with one reference

		struct Rect {
			int posX = 0;
			int posY = 0;
			int width = 0;
			int height = 0;
		};

		[[nodiscard]] Std::Opt<Rect> GetRect() const {
			Rect out = {};
			if (index < 0 || index >= root->perWindowData->accessElements.size())
				return Std::nullOpt;
			auto& element = root->perWindowData->accessElements[index];
			out.posX = element.posX;
			out.posY = element.posY;
			out.width = element.width;
			out.height = element.height;
			return out;
		}

		[[nodiscard]] auto Win32Hwnd() const { return this->root->Win32Hwnd(); }

		//
		// IUnknown start
		//
		HRESULT QueryInterface(REFIID riid, void** ppvObject) override {
			if (ppvObject == nullptr) {
				return E_INVALIDARG;
			}

			IUnknown* outPtr = nullptr;

			if (riid == IID_IUnknown) {
				outPtr = (IUnknown*) (IRawElementProviderFragment*) this;
			} else if (riid == IID_IRawElementProviderSimple) {
				outPtr = static_cast<IRawElementProviderSimple*>(this);
			} else if (riid == IID_IRawElementProviderFragment) {
				outPtr = static_cast<IRawElementProviderFragment*>(this);
			}

			if (outPtr != nullptr) {
				outPtr->AddRef();
				*ppvObject = outPtr;
				return S_OK;
			}

			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}

		ULONG AddRef() override {
			return InterlockedIncrement(&m_refCount);
		}

		ULONG Release() override {
			long val = InterlockedDecrement(&m_refCount);
			if (val == 0) {
				DENGINE_IMPL_UNREACHABLE();
			}
			return val;
		}
		//
		// IUnknown end
		//

		//
		// IRawElementProviderSimple start
		//
		HRESULT GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT get_ProviderOptions(ProviderOptions* pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = ProviderOptions_ServerSideProvider;
			return S_OK;
		}

		// Implementation of IRawElementProviderSimple::get_HostRawElementProvider.
		// Gets the UI Automation provider for the host window.
		// Return NULL. because the these items are not top level.
		HRESULT get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override {
			if (pRetVal == nullptr) return E_INVALIDARG;
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override {
			if (pRetVal == nullptr) return E_INVALIDARG;

			pRetVal->vt = VT_EMPTY;

			if (propertyId == UIA_NamePropertyId) {
				// Find the text-span of this fragment.
				auto const& element = this->root->perWindowData->accessElements[this->index];
				Std::Span textSpan = {
					&this->root->perWindowData->accessText[element.textStart],
					(uSize) element.textCount};
				std::wstring outString{textSpan.Data(), textSpan.Data() + textSpan.Size()};
				// Return the name (label) of this control
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(outString.c_str());
			} else if (propertyId == UIA_AutomationIdPropertyId) {
				std::string tempStr = std::format("Widget index #{}", this->index);
				std::wstring outString{tempStr.begin(), tempStr.end()};
				pRetVal->vt = VT_BSTR;
				pRetVal->bstrVal = SysAllocString(outString.c_str());
			} else if (propertyId == UIA_IsContentElementPropertyId) {
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
			} else if (propertyId == UIA_IsControlElementPropertyId) {
				pRetVal->vt = VT_BOOL;
				pRetVal->boolVal = VARIANT_TRUE;
			} else if (propertyId == UIA_ControlTypePropertyId) {
				pRetVal->vt = VT_I4;
				pRetVal->intVal = UIA_TextControlTypeId;
			}

			return S_OK;
		}
		//
		// IRawElementProviderSimple end
		//

		HRESULT Navigate(
			NavigateDirection direction,
			IRawElementProviderFragment** pRetVal) override {
			if (pRetVal == nullptr) return E_INVALIDARG;

			if (direction == NavigateDirection_Parent) {
				*pRetVal = this->root;
				this->root->AddRef();
				return S_OK;
			}
			if (direction == NavigateDirection_NextSibling) {
				if (index + 1 < this->root->children.size()) {
					*pRetVal = this->root->children[index + 1];
					(*pRetVal)->AddRef();
					return S_OK;
				}
			}
			if (direction == NavigateDirection_PreviousSibling) {
				if (index > 0) {
					*pRetVal = this->root->children[index - 1];
					(*pRetVal)->AddRef();
					return S_OK;
				}
			}

			*pRetVal = nullptr;
			return S_OK;
		}

		// Children like this need to return an array that begins with UiaAppendRuntimeId
		HRESULT GetRuntimeId(SAFEARRAY** pRetVal) override {
			if (pRetVal == nullptr)
				return E_INVALIDARG;

			auto safeArray = SafeArrayCreateVector(VT_I4, 0, 2);
			if (safeArray == nullptr) {
				*pRetVal = nullptr;
				return E_OUTOFMEMORY;
			}
			i32 arrayToPut[2] = {UiaAppendRuntimeId, this->index};
			for (long i = 0; i < 2; i++) {
				SafeArrayPutElement(safeArray, &i, &(arrayToPut[i]));
			}

			*pRetVal = safeArray;
			return S_OK;
		}

		HRESULT get_BoundingRectangle(UiaRect* pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}

			auto rectOpt = GetRect();
			if (!rectOpt.Has()) {
				return E_NOT_VALID_STATE;
			}
			auto const& rect = rectOpt.Get();

			POINT offset = {};
			auto result = ClientToScreen(this->Win32Hwnd(), &offset);
			if (!result) {
				// return some error.
			}
			pRetVal->left = rect.posX + offset.x;
			pRetVal->top = rect.posY + offset.y;
			pRetVal->width = rect.width;
			pRetVal->height = rect.height;
			return S_OK;
		}

		HRESULT GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override {
			*pRetVal = nullptr;
			return S_OK;
		}

		HRESULT SetFocus() override {
			return S_OK;
		}

		HRESULT get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override {
			if (pRetVal == nullptr) {
				return E_INVALIDARG;
			}
			*pRetVal = this->root;
			this->root->AddRef();
			return S_OK;
		}
	};

	struct InputScopeTest : public ITfInputScope {
		ULONG m_refCount = 1; // Start with one reference

		//
		// IUnknown start
		//
		HRESULT QueryInterface(REFIID riid, void** ppvObject) override {
			if (ppvObject == nullptr) {
				return E_INVALIDARG;
			}

			IUnknown* outPtr = nullptr;
			if (riid == IID_IUnknown || riid == IID_ITfInputScope) {
				outPtr = (IUnknown*) (ITfInputScope*) this;
			}

			if (outPtr != nullptr) {
				outPtr->AddRef();
				*ppvObject = outPtr;
				return S_OK;
			} else {
				*ppvObject = nullptr;
				return E_NOINTERFACE;
			}
		}

		ULONG AddRef() override {
			return InterlockedIncrement(&this->m_refCount);
		}

		ULONG Release() override {
			long val = InterlockedDecrement(&this->m_refCount);
			if (val == 0) {
				delete this;
			}
			return val;
		}
		//
		// IUnknown end
		//

		//
		// ITfInputScope start
		//
		HRESULT GetInputScopes(
			InputScope** out_inputScopes,
			UINT* out_count) override
		{
			if (out_count == nullptr || out_inputScopes == nullptr) {
				return E_INVALIDARG;
			}
			*out_inputScopes = (InputScope*)CoTaskMemAlloc(sizeof(InputScope) * 1);
			// Check if allocation failed.
			if (!*out_inputScopes) {
				*out_count = 0;
				return E_OUTOFMEMORY;
			}

			auto outInputScopeArray = *out_inputScopes;

			outInputScopeArray[0] = IS_TEXT;

			*out_count = 1;
			return S_OK;
		}

		HRESULT GetPhrase(BSTR** ppbstrPhrases, UINT* pcCount) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT GetRegularExpression(BSTR* pbstrRegExp) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT GetSRGS(BSTR* pbstrSRGS) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT GetXML(BSTR* pbstrXML) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}
		//
		// ITfInputScope end
		//
	};

	enum class TextStoreAdviseSink_Helper {
		TextChange = TS_AS_TEXT_CHANGE,
		SelChange = TS_AS_SEL_CHANGE,
		LayoutChange = TS_AS_LAYOUT_CHANGE,
		AttrChange = TS_AS_ATTR_CHANGE,
		StatusChange = TS_AS_STATUS_CHANGE,
		AllSinks = TS_AS_ALL_SINKS,
	};

	enum class TextStoreRequestLockFlags_Helper {
		ReadFlag = TS_LF_READ,
		ReadWriteFlag = TS_LF_READWRITE,
		SyncFlag = TS_LF_SYNC,
	};

	template<class GetTextPtrFnT, class ResizeStringFnT>
	void DEngine_ReplaceText(
		int currStringSize,
		GetTextPtrFnT const& getTextPtrFn,
		ResizeStringFnT const& resizeStringFn,
		u64 selIndex, u64 selCount,
		Std::RangeFnRef<Std::Trait::RemoveCVRef<decltype(*getTextPtrFn())>> const& newTextRange)
	{
		auto oldStringSize = currStringSize;
		auto sizeDiff = newTextRange.Size() - selCount;

		// First check if we need to expand our storage.
		if (sizeDiff > 0) {
			// We need to move all content behind the old substring
			// To the right.
			resizeStringFn(oldStringSize + sizeDiff);
			auto* textPtr = getTextPtrFn();
			auto end = selIndex + selCount - 1;
			for (auto i = oldStringSize - 1; i > end; i -= 1)
				textPtr[i + sizeDiff] = textPtr[i];
		} else if (sizeDiff < 0) {
			// We need to move all content behind the old substring
			// To the left.
			auto begin = selIndex + selCount;
			auto* textPtr = getTextPtrFn();
			for (auto i = begin; i < oldStringSize; i += 1)
				textPtr[i + sizeDiff] = textPtr[i];
			resizeStringFn(currStringSize + sizeDiff);
		}

		auto* textPtr = getTextPtrFn();
		for (auto i = 0; i < newTextRange.Size(); i += 1)
			textPtr[i + selIndex] = newTextRange.Invoke(i);
	}

	struct TextStoreTest :
		public ITextStoreACP2,
		public ITfContextOwnerCompositionSink,
		public ITfTextEditSink {
		ULONG m_refCount = 1; // Start with one reference

		void* backendData = nullptr;
		WindowID _windowId = WindowID::Invalid;
		[[nodiscard]] auto GetWindowId() const noexcept { return _windowId; }

		std::string innerText;
		u64 selectionStart = 0;
		u64 selectionCount = 0;


		[[nodiscard]] auto GetSelectionEnd() const noexcept { return this->selectionStart + this->selectionCount; }

		// Don't change
		HWND _hwnd = nullptr;
		[[nodiscard]] auto GetHwnd() const noexcept { return _hwnd; }


		ITextStoreACPSink* currentSink = nullptr;
		DWORD currentSinkMask = 0;
		// The type of current lock.
		//   0: No lock.
		//   TS_LF_READ: read-only lock.
		//   TS_LF_READWRITE: read/write lock.
		DWORD current_lock_type_ = 0;
		DWORD pendingAsyncLock = {};

		std::vector<TS_ATTRID> requestedAttrs;
		bool requestAttrWantFlag = false;

		static void Handle_WM_CHAR(
			TextStoreTest& textStore,
			BackendData& backendData,
			WindowID windowId,
			u64 characterIn)
		{
			auto character = (char)characterIn;

			if (character != '\b' && character < 32) {
				// The first 32 characters are ASCII control characters, and not used for text
				// Except for backspace thingy.
				return;
			}

			Std::Opt<TS_TEXTCHANGE> textChange;

			bool selectionChanged = false;
			auto oldSelIndex = textStore.selectionStart;
			auto oldSelCount = textStore.selectionCount;

			if (character == '\b') {
				PushEventJob_ThreadSafe(backendData,
					[=](Context& ctx) {
						BackendInterface::PushTextDeleteEvent(ctx.GetImplData(), windowId);
					});

				// We need to also update the selection.
				if (oldSelCount > 0) {
					PushEventJob_ThreadSafe(
						backendData,
						[=](Context& ctx) {
							BackendInterface::PushTextSelectionEvent(
								ctx.GetImplData(),
								windowId,
								oldSelIndex,
								0);
						});
				} else if (oldSelIndex > 0 && oldSelCount == 0) {
					PushEventJob_ThreadSafe(
						backendData,
						[=](Context& ctx) {
							BackendInterface::PushTextSelectionEvent(
								ctx.GetImplData(),
								windowId,
								oldSelIndex - 1,
								0);
						});
				}
			} else {
				// Run the actual text change.
				// We don't do this anymore
				/*
				DEngine_ReplaceText(
					(int)textStore.innerText.size(),
					[&]() { return textStore.innerText.data(); },
					[&](auto newSize) { textStore.innerText.resize(newSize); },
					oldSelIndex,
					oldSelCount,
					{ 1, [&](int i) { return character; }});
				*/

				PushEventJob_ThreadSafe(
					backendData,
					[=](Context& ctx) {
						BackendInterface::PushTextInputEvent(
							ctx.GetImplData(),
							windowId,
							oldSelIndex,
							oldSelCount,
							character);
					});

				// We add one to the selection, because we're inserting one character
				//textStore.selectionStart += 1;
				//textStore.selectionCount = 0;
				PushEventJob_ThreadSafe(
					backendData,
					[=](Context& ctx) {
						BackendInterface::PushTextSelectionEvent(
							ctx.GetImplData(),
							windowId,
							oldSelIndex + 1,
							0);
					});
			}

			/*
			if (textStore.currentSink != nullptr) {
				auto& sink = *textStore.currentSink;
				auto hResult = (HResult_Helper)sink.OnStartEditTransaction();
				if (hResult != HResult_Helper::Ok) {
					std::abort();
				}

				TS_TEXTCHANGE textChangeRange = {};
				textChangeRange.acpStart = (LONG)oldSelIndex;
				textChangeRange.acpOldEnd = (LONG)(oldSelIndex + oldSelCount);
				textChangeRange.acpNewEnd = (LONG)(oldSelIndex + 1);
				hResult = (HResult_Helper)sink.OnTextChange(0, &textChangeRange);
				if (hResult != HResult_Helper::Ok) {
					std::abort();
				}

				hResult = (HResult_Helper)sink.OnSelectionChange();
				if (hResult != HResult_Helper::Ok) {
					std::abort();
				}

				hResult = (HResult_Helper)sink.OnEndEditTransaction();
				if (hResult != HResult_Helper::Ok) {
					std::abort();
				}
			}
			*/
		}

		[[nodiscard]] auto HasReadLock() const noexcept {
			return
				((current_lock_type_ & TS_LF_READ) != 0) ||
				((current_lock_type_ & TS_LF_READWRITE) != 0);
		}

		[[nodiscard]] auto HasWriteLock() const noexcept {
			return (current_lock_type_ & TS_LF_READWRITE) != 0;
		}

		//
		// IUnknown start
		//
		HRESULT QueryInterface(REFIID riid, void** ppvObject) override {
			if (ppvObject == nullptr) {
				return E_INVALIDARG;
			}

			IUnknown* outPtr = nullptr;
			if (riid == IID_IUnknown || riid == IID_ITextStoreACP2) {
				outPtr = (IUnknown*) (ITextStoreACP2*) this;
			} else if (riid == IID_ITfContextOwnerCompositionSink) {
				outPtr = (ITfContextOwnerCompositionSink*) this;
			} else if (riid == IID_ITfTextEditSink) {
				outPtr = (ITfTextEditSink*) this;
			}

			if (outPtr != nullptr) {
				outPtr->AddRef();
				*ppvObject = outPtr;
				return S_OK;
			} else {
				*ppvObject = nullptr;
				return E_NOINTERFACE;
			}
		}

		ULONG AddRef() override {
			return InterlockedIncrement(&this->m_refCount);
		}

		ULONG Release() override {
			long val = InterlockedDecrement(&this->m_refCount);
			if (val == 0) {
				DENGINE_IMPL_UNREACHABLE();
			}
			return val;
		}
		//
		// IUnknown end
		//


		//
		// TextStoreTest start
		//
		HRESULT AdviseSink(REFIID iid, IUnknown* unknown, DWORD mask) override {
			if (this->currentSink != nullptr) {
				DENGINE_IMPL_UNREACHABLE();
			}

			if (iid != IID_ITextStoreACPSink)
				return E_INVALIDARG;

			void* tempPtr = nullptr;
			auto hResult = (HResult_Helper)unknown->QueryInterface(IID_ITextStoreACPSink, &tempPtr);
			if (hResult != HResult_Helper::Ok)
				return E_UNEXPECTED;

			this->currentSink = (ITextStoreACPSink*) tempPtr;
			this->currentSinkMask = mask;
			return S_OK;
		}

		HRESULT UnadviseSink(IUnknown* punk) override {
			DENGINE_IMPL_UNREACHABLE();
			return S_OK;
		}

		enum class RequestLock_LockFlags_Helper {
			Read = TS_LF_READ,
			ReadWrite = TS_LF_READWRITE,
			Sync = TS_LF_SYNC,
		};

		HRESULT RequestLock(DWORD newLockFlagsIn, HRESULT* out_hrSession) override {
			if (!this->currentSink)
				return E_FAIL;
			if (!out_hrSession)
				return E_INVALIDARG;
			if (this->HasReadLock()) {
				if ((newLockFlagsIn & TS_LF_SYNC) != 0) {
					// Can't lock synchronously.
					*out_hrSession = TS_E_SYNCHRONOUS;
					return S_OK;
				}
				// Queue the lock request.
				if (this->pendingAsyncLock != 0)
					std::abort();
				this->pendingAsyncLock = newLockFlagsIn;
				*out_hrSession = TS_S_ASYNC;
				return S_OK;
			}
			// Lock
			this->current_lock_type_ = newLockFlagsIn;
			// Grant the lock.
			*out_hrSession = this->currentSink->OnLockGranted((DWORD) current_lock_type_);
			// Unlock
			this->current_lock_type_ = 0;

			// Handles the pending lock requests.
			if (this->pendingAsyncLock != 0) {
				std::cout << "Granting a pending lock" << std::endl;
				this->current_lock_type_ = this->pendingAsyncLock;
				this->currentSink->OnLockGranted((DWORD) current_lock_type_);
			}
			this->pendingAsyncLock = 0;
			this->current_lock_type_ = 0;

			return S_OK;
		}

		HRESULT GetStatus(TS_STATUS* out_status) override {
			if (!out_status)
				return E_INVALIDARG;
			out_status->dwDynamicFlags =
				TS_SD_TKBAUTOCORRECTENABLE |
				TS_SD_TKBPREDICTIONENABLE;

			out_status->dwStaticFlags =
				TS_SS_TRANSITORY |
				TS_SS_NOHIDDENTEXT |
				TS_SS_TKBPREDICTIONENABLE |
				TS_SS_TKBAUTOCORRECTENABLE;
			return S_OK;
		}

		HRESULT QueryInsert(
			LONG acpTestStart,
			LONG acpTestEnd,
			ULONG cch,
			[[out]] LONG* pacpResultStart,
			/* out */ LONG* pacpResultEnd) override
		{
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT GetSelection(
			ULONG selectionIndex,
			ULONG selectionBufferSize,
			TS_SELECTION_ACP* out_selections,
			ULONG* out_fetchedCount) override
		{
			std::cout << "GetSelection" << std::endl;

			if (out_fetchedCount == nullptr)
				return E_INVALIDARG;
			if (selectionBufferSize != 0 && out_selections == nullptr)
				return E_INVALIDARG;
			if (!HasReadLock())
				return TS_E_NOLOCK;

			if (selectionIndex != TS_DEFAULT_SELECTION) {
				DENGINE_IMPL_UNREACHABLE();
			}
			*out_fetchedCount = 0;
			if ((selectionBufferSize > 0)) {
				out_selections[0].acpStart = (LONG)this->selectionStart;
				out_selections[0].acpEnd = (LONG)this->GetSelectionEnd();
				out_selections[0].style.ase = TS_AE_END;
				out_selections[0].style.fInterimChar = FALSE;
				*out_fetchedCount = 1;
			}
			return S_OK;
		}

		HRESULT SetSelection(ULONG ulCount, TS_SELECTION_ACP const* pSelection) override
		{
			std::cout << "SetSelection" << std::endl;

			if (!this->HasWriteLock()) {
				return TF_E_NOLOCK;
			}
			if (ulCount > 0 && pSelection == nullptr) {
				return E_INVALIDARG;
			}

			auto const& selectionIn = pSelection[0];

			auto windowId = this->GetWindowId();
			auto newSelIndex = selectionIn.acpStart;
			auto newSelCount = selectionIn.acpEnd - selectionIn.acpStart;
			//this->selectionStart = newSelIndex;
			//this->selectionCount = newSelCount;
			RunOnBackendThread2(
				this->backendData,
				[=](Context::Impl& implData) {
					BackendInterface::PushTextSelectionEvent(
						implData,
						windowId,
						(u64)newSelIndex,
						(u64)newSelCount);
				});



			return S_OK;
		}

		HRESULT GetText(
			LONG acpStart,
			LONG acpEnd,
			WCHAR* out_chars,
			ULONG charsOutCapacity,
			ULONG* out_charsCount,
			TS_RUNINFO* out_runInfos,
			ULONG runInfosOutCapacity,
			ULONG* out_runInfosCount,
			LONG* out_nextAcp) override
		{
			std::cout << "GetText || ";

			if (!this->HasReadLock()) {
				return TF_E_NOLOCK;
			}
			if (out_charsCount == nullptr || out_runInfosCount == nullptr || out_nextAcp == nullptr) {
				return E_INVALIDARG;
			}
			if (acpStart < 0) {
				return E_INVALIDARG;
			}
			if (acpEnd < -1) {
				return E_INVALIDARG;
			}
			if (acpEnd >= (LONG)this->innerText.size()) {
				DENGINE_IMPL_UNREACHABLE();
			}
			*out_charsCount = 0;
			*out_runInfosCount = 0;
			*out_nextAcp = 0;

			int internalTextSize = (int)this->innerText.size();
			int startIndex = Math::Min(internalTextSize, (int) acpStart);
			int endIndex = acpEnd;
			if (endIndex == -1)
				endIndex = internalTextSize;
			endIndex = Math::Min(endIndex, internalTextSize);
			auto charCount = endIndex - startIndex;
			charCount = Math::Max(0, Math::Min(charCount, (int)charsOutCapacity));
			endIndex = startIndex + charCount;

			*out_nextAcp = endIndex;

			// Copy over our characters
			if (charsOutCapacity > 0 && out_chars != nullptr) {
				for (int i = 0; i < charCount; i++) {
					out_chars[i] = (WCHAR)this->innerText[i + startIndex];
				}
				// Set the variable that says how many characters we have output.
				*out_charsCount = (ULONG)charCount;

				std::cout.write(&this->innerText[startIndex], charCount);
			}

			// For now, all our text is assumed to be visible, so we output that.
			if (runInfosOutCapacity > 0 && out_runInfos != nullptr) {
				auto& out = out_runInfos[0];
				out.type = TS_RT_PLAIN;
				out.uCount = charCount;
				*out_runInfosCount = 1;
			}

			std::cout << std::endl;
			return S_OK;
		}

		HRESULT SetText(
			DWORD dwFlags,
			LONG acpStart,
			LONG acpEnd,
			WCHAR const* newText,
			ULONG newTextCount,
			TS_TEXTCHANGE* out_change) override
		{
			std::cout << "SetText" << std::endl;
			if (!this->HasWriteLock())
				return TF_E_NOLOCK;
			if (out_change == nullptr)
				return E_INVALIDARG;
			if (newTextCount != 0 && newText == nullptr)
				return E_INVALIDARG;
			if (acpEnd < 0)
				DENGINE_IMPL_UNREACHABLE();
			if (acpStart < 0 || acpEnd > this->innerText.size()) {
				DENGINE_IMPL_UNREACHABLE();
				return TF_E_INVALIDPOS;
			}


			// We can't really set the text directly, because this does not update the glyph rects.
			/*
			DEngine_ReplaceText(
				(int)this->innerText.size(),
				[&]() { return this->innerText.data(); },
				[&](auto newSize) { this->innerText.resize(newSize); },
				acpStart,
				acpEnd - acpStart,
				{ (int)newTextCount, [&](int i) { return (char) newText[i]; }});
			out_change->acpStart = acpStart;
			out_change->acpOldEnd = acpEnd;
			out_change->acpNewEnd = acpStart + (LONG)newTextCount;
			*/
			out_change->acpStart = acpStart;
			out_change->acpOldEnd = acpEnd;
			out_change->acpNewEnd = acpStart + newTextCount;

			auto windowId = this->GetWindowId();
			std::vector<u32> outputText;
			outputText.resize(newTextCount);
			for (int i = 0; i < outputText.size(); i++) {
				outputText[i] = (u32)newText[i];
			}
			RunOnBackendThread2(this->backendData,
				[=, outputText = Std::Move(outputText)](Context::Impl& implData) {
					BackendInterface::PushTextInputEvent(
						implData,
						windowId,
						(u64)acpStart,
						(u64)acpEnd - acpStart,
						{outputText.data(), outputText.size()});
				});


			return S_OK;
		}

		HRESULT GetFormattedText(
			LONG acpStart,
			LONG acpEnd,
			IDataObject** ppDataObject) override {
			DENGINE_IMPL_UNREACHABLE();
			if (ppDataObject == nullptr)
				return E_INVALIDARG;
			return E_NOTIMPL;
		}

		HRESULT GetEmbedded(
			LONG acpPos,
			REFGUID rguidService,
			REFIID riid,
			IUnknown** ppunk) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT QueryInsertEmbedded(
			GUID const* pguidService,
			FORMATETC const* pFormatEtc,
			BOOL* pfInsertable) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT InsertEmbedded(
			DWORD dwFlags,
			LONG acpStart,
			LONG acpEnd,
			IDataObject* pDataObject,
			TS_TEXTCHANGE* pChange) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT InsertTextAtSelection(
			DWORD dwFlags,
			WCHAR const* pchText,
			ULONG cch,
			LONG* pacpStart,
			LONG* pacpEnd,
			TS_TEXTCHANGE* pChange) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}


		HRESULT InsertEmbeddedAtSelection(
			DWORD dwFlags,
			IDataObject* pDataObject,
			LONG* pacpStart,
			LONG* pacpEnd,
			TS_TEXTCHANGE* pChange) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT RequestSupportedAttrs(
			DWORD dwFlags,
			ULONG attrBufferSize,
			TS_ATTRID const* attrBuffer) override
		{
			if (attrBufferSize != 0 && attrBuffer == nullptr) {
				return E_INVALIDARG;
			}

			this->requestAttrWantFlag = false;
			if ((dwFlags & TS_ATTR_FIND_WANT_VALUE) != 0) {
				this->requestAttrWantFlag = true;
			}

			this->requestedAttrs.clear();
			for (int i = 0; i < (int)attrBufferSize; i++) {
				auto const& attr = attrBuffer[i];
				if (attr == GUID_PROP_INPUTSCOPE) {
					this->requestedAttrs.push_back(attr);
				}
			}

			return S_OK;
		}

		HRESULT RetrieveRequestedAttrs(
			ULONG attrValsCapacity,
			TS_ATTRVAL* out_AttrVals,
			ULONG* out_AttrValsFetched) override
		{
			if (!this->HasReadLock()) {
				return TF_E_NOLOCK;
			}
			if (out_AttrValsFetched == nullptr) {
				return E_INVALIDARG;
			}
			if (attrValsCapacity == 0) {
				*out_AttrValsFetched = 0;
				return S_OK;
			}

			// If we reached this branch, it means capacity is not zero, in which case the pointer can't be null.
			if (out_AttrVals == nullptr) {
				return E_INVALIDARG;
			}

			*out_AttrValsFetched = Math::Min((ULONG)this->requestedAttrs.size(), attrValsCapacity);
			for (int i = 0; i < *out_AttrValsFetched; i++) {
				auto& outAttr = out_AttrVals[i];
				auto const& requestedAttr = this->requestedAttrs[i];

				// This should be set to 0 when using TSF.
				outAttr.dwOverlapId = 0;
				outAttr.idAttr = requestedAttr;

				if (requestedAttr == GUID_PROP_INPUTSCOPE) {
					outAttr.varValue.vt = VT_UNKNOWN;
					outAttr.varValue.punkVal = new InputScopeTest;
				} else {
					outAttr.varValue.vt = VT_EMPTY;
				}
			}

			this->requestAttrWantFlag = false;
			this->requestedAttrs.clear();

			return S_OK;
		}

		HRESULT RequestAttrsAtPosition(
			LONG acpPos,
			ULONG attrBufferSize,
			TS_ATTRID const* attrBuffer,
			DWORD dwFlags) override
		{
			if (attrBufferSize != 0 && attrBuffer == nullptr) {
				return E_INVALIDARG;
			}

			this->requestAttrWantFlag = false;
			this->requestedAttrs.clear();
			for (int i = 0; i < (int)attrBufferSize; i++) {
				auto const& attr = attrBuffer[i];
				if (attr == GUID_PROP_INPUTSCOPE) {
					this->requestedAttrs.push_back(attr);
				}
			}

			return S_OK;
		}

		HRESULT RequestAttrsTransitioningAtPosition(
			LONG acpPos,
			ULONG cFilterAttrs,
			TS_ATTRID const* paFilterAttrs,
			DWORD dwFlags) override
		{
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT FindNextAttrTransition(
			LONG acpStart,
			LONG acpHalt,
			ULONG cFilterAttrs,
			TS_ATTRID const* paFilterAttrs,
			DWORD dwFlags,
			LONG* out_pAcpNext,
			BOOL* out_pFound,
			LONG* out_pFoundOffset) override
		{
			DENGINE_IMPL_UNREACHABLE();

			if (!out_pAcpNext || !out_pFound || !out_pFoundOffset)
				return E_INVALIDARG;
			*out_pAcpNext = 0;
			*out_pFound = FALSE;
			*out_pFoundOffset = 0;
			return S_OK;
		}

		HRESULT GetEndACP(/* [out] */ LONG* pacp) override {
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT GetActiveView(TsViewCookie* outPvcView) override {
			if (outPvcView == nullptr)
				return E_INVALIDARG;
			*outPvcView = 0;
			return S_OK;
		}

		HRESULT GetACPFromPoint(
			TsViewCookie vcView,
			POINT const* ptScreen,
			DWORD dwFlags,
			LONG* out_Acp) override
		{
			DENGINE_IMPL_UNREACHABLE();
			return E_NOTIMPL;
		}

		HRESULT GetTextExt(
			TsViewCookie vcView,
			LONG acpStart,
			LONG acpEnd,
			RECT* out_rect,
			BOOL* out_fClipped) override
		{
			if (out_rect != nullptr) {
				POINT point = {};
				auto success = ClientToScreen(this->GetHwnd(), &point);
				if (!success) {
					return E_UNEXPECTED;
				}
				out_rect->left = point.x;
				out_rect->right = point.x + 100;
				out_rect->top = point.y;
				out_rect->bottom = point.y + 100;
			}
			if (out_fClipped != nullptr) {
				*out_fClipped = false;
			}

			return S_OK;
		}

		HRESULT GetScreenExt(
			TsViewCookie vcView,
			RECT* out_rect) override
		{
			if (out_rect != nullptr) {
				POINT point = {};
				auto success = ClientToScreen(this->GetHwnd(), &point);
				if (!success) {
					return E_UNEXPECTED;
				}
				out_rect->left = point.x;
				out_rect->right = point.x + 100;
				out_rect->top = point.y;
				out_rect->bottom = point.y + 100;
			}

			return S_OK;
		}
		//
		// TextStoreTest end
		//

		//
		// ITfContextOwnerCompositionSink start
		//
		HRESULT OnStartComposition(ITfCompositionView* pComposition, BOOL* out_Ok) override {
			std::cout << "StartComposition" << std::endl;
			if (pComposition == nullptr || out_Ok == nullptr) {
				return E_INVALIDARG;
			}

			//DENGINE_IMPL_UNREACHABLE();

			*out_Ok = true;
			return S_OK;
		}

		HRESULT OnUpdateComposition(ITfCompositionView* pComposition, ITfRange* pRangeNew) override {
			//DENGINE_IMPL_UNREACHABLE();
			return S_OK;
		}

		HRESULT OnEndComposition(ITfCompositionView* pComposition) override {
			std::cout << "EndComposition" << std::endl;
			//DENGINE_IMPL_UNREACHABLE();
			return S_OK;
		}
		//
		// ITfContextOwnerCompositionSink end
		//

		//
		// ITfTextEditSink start
		//
		HRESULT OnEndEdit(
			ITfContext* pic,
			TfEditCookie ecReadOnly,
			ITfEditRecord* pEditRecord) override {
			return S_OK;
		}
		//
		// ITfTextEditSink end
		//
	};
}