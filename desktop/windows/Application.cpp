#include <DEngine/impl/AppAssert.hpp>
#include <DEngine/impl/Application.hpp>

#include "BackendData.hpp"

#include <iostream>
#include <string>
#include <new>
#include <thread>


extern int dengine_impl_main(int argc, char** argv);

namespace DEngine::Application::impl::Backend {
	BackendData* pGlobalBackendData = nullptr;


	void Handle_WM_SIZING(WPARAM wParam, LPARAM lParam, BackendData& backendData, HWND hwnd, WindowID windowId) {
		RECT clientRect = {};
		GetClientRect(hwnd, &clientRect);
		auto clientWidth = clientRect.right - clientRect.left;
		auto clientHeight = clientRect.bottom - clientRect.top;

		/*
		PushEventJob_ThreadSafe(backendData, [=](Context& ctx) {
			BackendInterface::UpdateWindowSize(
				ctx.GetImplData(),
				windowId,
				{ (u32)clientWidth, (u32)clientHeight },
				0, 0,
				{ (u32)clientWidth, (u32)clientHeight });
		});
		 */
	}

	void Handle_WM_SIZE(WPARAM wParam, LPARAM lParam, BackendData& backendData, HWND hwnd, WindowID windowId) {

		auto clientWidth = LOWORD(lParam);
		auto clientHeight = HIWORD(lParam);

		if (wParam == SIZE_RESTORED) {
			PushEventJob_ThreadSafe(backendData, [=](Context& ctx) {
				BackendInterface::UpdateWindowSize(
					ctx.GetImplData(),
					windowId,
					{(u32) clientWidth, (u32) clientHeight},
					0, 0,
					{(u32) clientWidth, (u32) clientHeight});
			});
		}
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto implDataPtr = (Context::Impl*) GetWindowLongPtr(hwnd, 0);
		if (implDataPtr == nullptr) {
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		auto& implData = *implDataPtr;
		auto& backendData = *(BackendData*) implData.backendData;
		auto windowIdOpt = implData.GetWindowId(hwnd);
		if (!windowIdOpt.Has()) {
			DENGINE_IMPL_UNREACHABLE();
		}
		auto windowId = windowIdOpt.Get();

		auto& perWindowDataBackend = (PerWindowData&)implData.GetWindowBackend(hwnd);

		switch (msg) {
			case WM_DPICHANGED: {
				PushEventJob_ThreadSafe(backendData,
					[=](Context& ctx) {
						BackendInterface::WindowContentScale(ctx.GetImplData(), windowId,
															 (float) HIWORD(wParam) / 96);
					});
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_CHAR: {
				if (backendData.textStore != nullptr) {
					TextStoreTest::Handle_WM_CHAR(
						*backendData.textStore,
						backendData,
						windowId,
						(u64) wParam);
				}
				break;
			}

			case WM_SIZING: {
				//HandleWindowSizingEvent(wParam, lParam, backendData, hwnd, windowId);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_ENTERSIZEMOVE: {
				std::cout << "Window enter sizemove event" << std::endl;
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}
			case WM_EXITSIZEMOVE: {
				std::cout << "Window exit sizemove event" << std::endl;
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_SIZE: {
				Handle_WM_SIZE(wParam, lParam, backendData, hwnd, windowId);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_DISPLAYCHANGE: {
				// There is no event for checking if the windows ACTUAL dpi, only the content scale.
				// So we always query to see if the display has changed whenever we move.
				auto dc = GetDC(hwnd);
				auto newDpiX = (f32) GetDeviceCaps(dc, HORZRES) / ((f32) GetDeviceCaps(dc, HORZSIZE) / 25.4f);
				auto newDpiY = (f32) GetDeviceCaps(dc, VERTRES) / ((f32) GetDeviceCaps(dc, VERTSIZE) / 25.4f);
				ReleaseDC(hwnd, dc);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_WINDOWPOSCHANGED: {
				auto* pWinPos = (WINDOWPOS*) lParam;
				auto winPosX = pWinPos->cx;
				auto winPosY = pWinPos->cy;
				PushEventJob_ThreadSafe(backendData,
					[=](Context& ctx) {
						BackendInterface::UpdateWindowPosition(
							ctx.GetImplData(),
							windowId,
							{ winPosX, winPosY });
					});
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_MOUSEMOVE: {
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				PushEventJob_ThreadSafe(backendData,
										[=](Context& ctx) {
											BackendInterface::UpdateCursorPosition(ctx.GetImplData(), windowId, {x, y});
										});
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}
			case WM_LBUTTONDOWN: {
				BackendInterface::UpdateButton(implData, windowId, Button::LeftMouse, true);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}
			case WM_LBUTTONUP: {
				BackendInterface::UpdateButton(implData, windowId, Button::LeftMouse, false);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}


			case WM_CLOSE:
				return DefWindowProc(hwnd, msg, wParam, lParam);
			case WM_DESTROY:
				return DefWindowProc(hwnd, msg, wParam, lParam);
			case WM_GETOBJECT: {
				if ((DWORD)lParam == UiaRootObjectId && perWindowDataBackend.accessProvider != nullptr) { // UI Automation request
					return backendData.uiAutomationFnPtrs.UiaReturnRawElementProvider(
						hwnd,
						wParam,
						lParam,
						perWindowDataBackend.accessProvider);
				}
				break;
			}
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

std::string GetLastErrorAsString() {
	DWORD error = GetLastError();
	if (error == 0)
		return "No error";

	char* outputBuffer = nullptr;
	auto size = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		error,
		0, // Default language
		(LPTSTR) &outputBuffer,
		0,
		nullptr);

	if (size == 0) {
		return "Failed to format error message.";
	}

	std::string msg(outputBuffer, size);

	// Free the buffer allocated by FormatMessage
	LocalFree(outputBuffer);

	return msg;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	using namespace DEngine;
	using namespace DEngine::Application::impl::Backend;

	pGlobalBackendData = new BackendData();
	auto& backendData = *pGlobalBackendData;

	SetThreadDescription(
		GetCurrentThread(),
		L"WinMain");

	auto currThreadId = GetCurrentThreadId();

	bool setDpiAwareResult = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	if (!setDpiAwareResult) {
		// error
	}

	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	// Register the window class
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpszClassName = "MyWindowClass";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.cbWndExtra = sizeof(PerWindowData*);

	auto registerClassResult = RegisterClassEx(&wc);
	if (registerClassResult == 0) {
		std::cout << "Error when registering class" << std::endl;
	}

	auto uiAutomationCoreLib = LoadLibraryA("UIAutomationCore.dll");
	if (uiAutomationCoreLib == nullptr) {
		std::abort();
	}
	auto uiaReturnRawElementProviderFn = GetProcAddress(uiAutomationCoreLib, "UiaReturnRawElementProvider");
	if (uiaReturnRawElementProviderFn == nullptr) {
		std::abort();
	}
	auto uiaHostProviderFromHwndFn = GetProcAddress(uiAutomationCoreLib, "UiaHostProviderFromHwnd");
	if (uiaHostProviderFromHwndFn == nullptr) {
		std::abort();
	}

	// Load the Vulkan library
	auto vulkanLibrary = LoadLibraryA("vulkan-1.dll");
	if (vulkanLibrary == nullptr) {
		// error
	}
	// Get a pointer to vkGetInstanceProcAddr
	auto vkGetInstanceProcAddrFn = GetProcAddress(vulkanLibrary, "vkGetInstanceProcAddr");
	if (vkGetInstanceProcAddrFn == nullptr) {
		// error
	}
	//FreeLibrary(vulkanLibrary);

	auto hResult = HResult_Helper::Ok;

	// Initialize COM
	auto comInitResult = (HResult_Helper)CoInitialize(nullptr);
	if (comInitResult != HResult_Helper::Ok) {
		std::abort();
	}

	backendData.winMainThreadId_ = currThreadId;
	backendData.instanceHandle = hInstance;
	backendData.mainWindowClass = registerClassResult;
	backendData.nCmdShow = nCmdShow;
	backendData.pfn_vkGetInstanceProcAddr = (void*)vkGetInstanceProcAddrFn;
	backendData.uiAutomationFnPtrs.UiaReturnRawElementProvider = (BackendData::UiaReturnRawElementProvider_FnT)uiaReturnRawElementProviderFn;
	backendData.uiAutomationFnPtrs.UiaHostProviderFromHwnd = (BackendData::UiaHostProviderFromHwnd_FnT)uiaHostProviderFromHwndFn;


	auto initWaitFuture = backendData.initWaitPromise.get_future();

	auto otherThread = std::thread{[]() {
		dengine_impl_main(0, nullptr);
	}};

	// We wait until the other thread has called init on our system.
	auto& implData = *initWaitFuture.get();

	MSG msg = {};
	while (true) {
		// We always wait for a new result
		int getResult = GetMessage(&msg, nullptr, 0, 0);
		if (getResult >= 0) {
			if ((CustomMessageEnum) msg.message == CustomMessageEnum::CustomMessage) {
				// Handle our custom message?
				u64 jobId = (u64) msg.wParam;
				// Find the job and execute it.
				std::scoped_lock lock{backendData.customJobsLock};
				auto& customJobs = backendData.customJobs;
				// Find the job
				auto iterator = Std::FindIf(customJobs.items.begin(), customJobs.items.end(),
											[&](auto const& item) { return item.id == jobId; });
				DENGINE_IMPL_ASSERT(iterator != customJobs.items.end());
				{
					auto& item = *iterator;
					item.consumeFn(item.data, implData);
				}
				customJobs.items.erase(iterator);

			} else {
				// We got a message
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (getResult == 0) {
				// We got the quit signal.
				DENGINE_IMPL_UNREACHABLE();
			}
		} else { // getResult < 0
			// We got an error
			DENGINE_IMPL_UNREACHABLE();
		}
	}

	return 0;
}

using namespace DEngine;
using namespace DEngine::Application;

void* Application::impl::Backend::Initialize(Context& ctx, Context::Impl& implData) {

	implData.cursorOpt = CursorData{};

	auto& backendData = *pGlobalBackendData;
	backendData.initWaitPromise.set_value(&implData);

	return pGlobalBackendData;
}

void Application::impl::Backend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	void* pBackendData,
	bool waitForEvents,
	u64 timeoutNs) {
	// This needs to run the application-loop

	// We check if there's any jobs for us
	auto& backendData = *(BackendData*) pBackendData;

	std::scoped_lock lock{backendData.eventJobsLock};
	if (!backendData.queuedEventCallbacks.IsEmpty()) {
		backendData.queuedEventCallbacks.Consume(ctx);
	}
}

void Application::impl::Backend::Destroy(void* data) {
}

void Application::impl::Backend::RunOnBackendThread(void* pBackendData, RunOnBackendThread_JobItem item) {
	auto& backendData = *(BackendData*) pBackendData;

	u64 jobId = 0;
	{
		std::scoped_lock lock{backendData.customJobsLock};
		auto& customJobs = backendData.customJobs;
		jobId = customJobs.customJobIdTracker;
		customJobs.customJobIdTracker++;
		customJobs.items.emplace_back(CustomJobItem{
			.id = jobId,
			.data = item.data,
			.consumeFn = item.consumeFn,
		});
	}
	while (true) {
		// Posting can fail in some cases, like when the message-queue is full, so we just keep trying until it works.
		auto postResult = PostThreadMessageA(
			backendData.WinMainThreadId_ThreadSafe(),
			(UINT) CustomMessageEnum::CustomMessage,
			jobId,
			0);
		// The result is nonzero if it succeeds.
		if (postResult != 0)
			break;
	}
}

auto Application::impl::Backend::NewWindow_NotAsync(
	Context::Impl& implData,
	void* pBackendData,
	Std::Span<char const> const& title,
	Extent extent) -> Std::Opt<NewWindow_ReturnT> {
	// This function MUST run on the message polling thread.
	auto& backendData = *(BackendData*) pBackendData;
	DENGINE_IMPL_APPLICATION_ASSERT(backendData.WinMainThreadId_ThreadSafe() == GetCurrentThreadId());

	auto winInstance = backendData.instanceHandle;
	auto winCmdShowFlag = backendData.nCmdShow;

	// The Win32 API creates our window with our outer size in mind.
	// Our parameters are for the inner framebuffer size,
	// so we need to adjust our extents.
	RECT tempRect = {0, 0, (LONG) extent.width, (LONG) extent.height};
	AdjustWindowRect(&tempRect, WS_OVERLAPPEDWINDOW, FALSE);
	auto windowOuterWidth = tempRect.right - tempRect.left;
	auto windowOuterHeight = tempRect.bottom - tempRect.top;

	// Create the window
	auto hwnd = CreateWindow(
		"MyWindowClass",
		title.Data(),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, windowOuterWidth, windowOuterHeight,
		nullptr,
		nullptr,
		winInstance,
		nullptr);
	if (hwnd == nullptr) {
		return Std::nullOpt;
	}
	// This enables Text Services Framework on this window??? idk
	auto setEditStyleFlags = EditStyleFlag_Helper(SES_USECTF | SES_EX_MULTITOUCH);
	SendMessage(hwnd, EM_SETEDITSTYLE, (DWORD) setEditStyleFlags, (DWORD) setEditStyleFlags);
	SetWindowLongPtr(hwnd, 0, (LONG_PTR) &implData);

	ShowWindow(hwnd, winCmdShowFlag);

	WINDOWINFO windowInfo = {};
	windowInfo.cbSize = sizeof(WINDOWINFO);
	auto getWindowResult = GetWindowInfo(hwnd, &windowInfo);
	if (getWindowResult == false) {
		return Std::nullOpt;
	}

	auto* perWindowData = new PerWindowData;
	perWindowData->hwnd = hwnd;
	perWindowData->accessProvider = new TestAccessProvider;
	perWindowData->accessProvider->perWindowData = perWindowData;
	perWindowData->accessProvider->pBackendData = &backendData;

	NewWindow_ReturnT returnVal = {};
	returnVal.windowData.extent.width = windowInfo.rcClient.right - windowInfo.rcClient.left;
	returnVal.windowData.extent.height = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
	returnVal.windowData.position.x = windowInfo.rcClient.left;
	returnVal.windowData.position.y = windowInfo.rcClient.top;
	{
		// GetDpiForWindow does not return physical DPI, so we gotta query the monitor to approximate.
		auto dc = GetDC(hwnd);
		auto resX = GetDeviceCaps(dc, HORZRES);
		auto resY = GetDeviceCaps(dc, VERTRES);
		auto mmX = GetDeviceCaps(dc, HORZSIZE);
		auto mmY = GetDeviceCaps(dc, VERTSIZE);
		ReleaseDC(hwnd, dc);
		auto inchesX = (f32) mmX / 25.4f;
		auto inchesY = (f32) mmY / 25.4f;
		returnVal.windowData.dpiX = (f32) resX / inchesX;
		returnVal.windowData.dpiY = (f32) resY / inchesY;
	}
	{
		// This code gets the content scale?
		// The DPI is always relative to 96 DPI, kinda dumb but okay.
		auto dpi = GetDpiForWindow(hwnd);
		returnVal.windowData.contentScale = (f32) dpi / 96.f;
	}
	returnVal.windowData.orientation = Orientation::Landscape;
	returnVal.windowData.visibleExtent.width = windowInfo.rcWindow.right - windowInfo.rcWindow.left;
	returnVal.windowData.visibleExtent.height = windowInfo.rcWindow.bottom - windowInfo.rcWindow.top;
	returnVal.windowData.isMinimized = IsIconic(hwnd);

	WindowID windowId = {};
	// Insert the window into storage?
	{
		std::scoped_lock idLock{implData.windowsLock};
		windowId = (WindowID) implData.windowIdTracker;
		implData.windowIdTracker++;
		implData.windows.push_back(Context::Impl::WindowNode{
			.id = windowId,
			.windowData = returnVal.windowData,
			.events = {},
			.backendData = Std::Box{perWindowData},
		});
	}

	returnVal.windowId = windowId;

	return returnVal;
}

void Application::impl::Backend::DestroyWindow(
	Context::Impl& implData,
	void* platformHandle,
	Context::Impl::WindowNode const& windowNode) {

}

void Application::impl::Backend::Log(
	Context::Impl& implData,
	LogSeverity severity,
	Std::Span<char const> const& msg) {
	std::cout.write(msg.Data(), (std::streamsize) msg.Size()) << std::endl;
}

bool Application::impl::Backend::StartTextInputSession(
	Context::Impl& implData,
	WindowID windowId,
	void* backendDataIn,
	SoftInputFilter inputFilter,
	Std::Span<char const> const& textInput) {
	// We gotta run this on the WinMain thread
	RunOnBackendThread_Wait(backendDataIn, [&](Context::Impl& ctx) {
		auto& backendData = *(BackendData*) ctx.backendData;
		DENGINE_IMPL_APPLICATION_ASSERT(backendData.WinMainThreadId_ThreadSafe() == GetCurrentThreadId());

		HWND hwnd = {};
		{
			auto windowsLock = std::scoped_lock{ctx.windowsLock};
			auto windowIter = Std::FindIf(
				ctx.windows.begin(), ctx.windows.end(),
				[&](auto const& item) { return item.id == windowId; });
			DENGINE_IMPL_APPLICATION_ASSERT(windowIter != ctx.windows.end());
			auto& windowNode = *windowIter;
			auto& perWindowData = *(PerWindowData*) windowNode.backendData.Get();
			hwnd = perWindowData.hwnd;
		}

		auto hResult = HResult_Helper::Ok;

		ITfThreadMgr* threadMgr = nullptr;
		ITfThreadMgr2* threadMgr2 = nullptr;
		ITfSourceSingle* threadMgrSource = nullptr;
		{
			hResult = (HResult_Helper) CoCreateInstance(
				CLSID_TF_ThreadMgr,
				nullptr,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&threadMgr));
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}
			void* tempPtr = nullptr;

			hResult = (HResult_Helper) threadMgr->QueryInterface(IID_ITfThreadMgr2, &tempPtr);
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}
			threadMgr2 = (ITfThreadMgr2*) tempPtr;
		}

		TfClientId clientId = {};
		hResult = (HResult_Helper) threadMgr2->Activate(&clientId);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		ITfDocumentMgr* docMgr = nullptr;
		hResult = (HResult_Helper) threadMgr2->CreateDocumentMgr(&docMgr);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		auto textStore = new TextStoreTest;
		textStore->backendData = &backendData;
		textStore->_windowId = windowId;
		textStore->_hwnd = hwnd;
		textStore->innerText.resize(textInput.Size());
		for (int i = 0; i < (int) textInput.Size(); i++) {
			textStore->innerText[i] = textInput[i];
		}
		textStore->selectionStart = (int) textInput.Size();
		textStore->selectionCount = 0;


		ITfContext* tsfCtx = nullptr;
		TfEditCookie editCookie = {};
		hResult = (HResult_Helper) docMgr->CreateContext(
			clientId,
			0,
			(ITextStoreACP2*) textStore,
			&tsfCtx,
			&editCookie);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		void* tsfCtxSourceTemp = nullptr;
		hResult = (HResult_Helper) tsfCtx->QueryInterface(IID_ITfSource, &tsfCtxSourceTemp);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}
		auto tsfCtxSource = (ITfSource*) tsfCtxSourceTemp;

		DWORD adviseSinkIdentifier = {};
		hResult = (HResult_Helper) tsfCtxSource->AdviseSink(
			IID_ITfTextEditSink,
			(ITfTextEditSink*)textStore,
			&adviseSinkIdentifier);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		hResult = (HResult_Helper) docMgr->Push(tsfCtx);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		if (textStore->currentSink != nullptr) {
			auto& sink = *textStore->currentSink;
			hResult = (HResult_Helper) sink.OnStartEditTransaction();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			TS_TEXTCHANGE textChangeRange = {};
			textChangeRange.acpStart = 0;
			textChangeRange.acpOldEnd = 0;
			textChangeRange.acpNewEnd = (LONG) textInput.Size();
			hResult = (HResult_Helper) sink.OnTextChange(0, &textChangeRange);
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			hResult = (HResult_Helper) sink.OnSelectionChange();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			hResult = (HResult_Helper) sink.OnLayoutChange(TS_LC_CREATE, 0);
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			hResult = (HResult_Helper) sink.OnEndEditTransaction();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}
		}

		hResult = (HResult_Helper) threadMgr2->SetFocus(docMgr);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		ITfDocumentMgr* oldDocMgr = nullptr;
		hResult = (HResult_Helper) threadMgr->AssociateFocus(hwnd, docMgr, &oldDocMgr);
		if (hResult != HResult_Helper::Ok) {
			std::abort();
		}

		backendData.threadMgr = threadMgr;
		backendData.threadMgr2 = threadMgr2;
		backendData.clientId = clientId;
		backendData.docMgr = docMgr;
		backendData.textStore = textStore;
	});

	return true;
}

void Application::impl::Backend::UpdateTextInputConnection(
	void* backendDataIn,
	u64 selIndex,
	u64 selCount,
	Std::Span<u32 const> newText)
{
	// Copy the data and then run it on
	std::vector<u32> textStorage { newText.begin(), newText.end() };
	RunOnBackendThread2(backendDataIn, [
		newText = Std::Move(textStorage),
		selIndex,
		selCount](Context::Impl& ctx)
	{
		auto& backendData = *(BackendData*)ctx.backendData;
		DENGINE_IMPL_ASSERT(backendData.textStore != nullptr);

		auto& textStore = *backendData.textStore;

		DEngine_ReplaceText(
			(int)textStore.innerText.size(),
			[&] { return textStore.innerText.data(); },
			[&](auto newSize) { return textStore.innerText.resize(newSize); },
			selIndex,
			selCount,
			{ (int)newText.size(), [&](auto i) { return (char)newText[i]; } });

		auto hResult = HResult_Helper::Ok;

		if (textStore.currentSink != nullptr) {
			auto& sink = *textStore.currentSink;
			hResult = (HResult_Helper)sink.OnStartEditTransaction();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			TS_TEXTCHANGE textChangeRange = {};
			textChangeRange.acpStart = (LONG)selIndex;
			textChangeRange.acpOldEnd = (LONG)selIndex + selCount;
			textChangeRange.acpNewEnd = (LONG)selIndex + newText.size();
			hResult = (HResult_Helper)sink.OnTextChange(0, &textChangeRange);
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			hResult = (HResult_Helper) sink.OnEndEditTransaction();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}
		}
	});
}

void Application::impl::Backend::UpdateTextInputConnectionSelection(void* backendDataIn, u64 selIndex, u64 selCount) {
	RunOnBackendThread2(backendDataIn, [=](Context::Impl& ctx) {
		auto& backendData = *(BackendData*)ctx.backendData;
		DENGINE_IMPL_ASSERT(backendData.textStore != nullptr);

		auto& textStore = *backendData.textStore;
		textStore.selectionStart = selIndex;
		textStore.selectionCount = selCount;

		auto hResult = HResult_Helper::Ok;

		if (textStore.currentSink != nullptr) {
			auto& sink = *textStore.currentSink;
			hResult = (HResult_Helper)sink.OnStartEditTransaction();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

 			hResult = (HResult_Helper)sink.OnSelectionChange();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}

			hResult = (HResult_Helper) sink.OnEndEditTransaction();
			if (hResult != HResult_Helper::Ok) {
				std::abort();
			}
		}
	});
}

void Application::impl::Backend::StopTextInputSession(
	Context::Impl& implData,
	void* backendData)
{

}

void Application::impl::Backend::UpdateAccessibility(
	Context::Impl& implData,
	void* pBackendDataIn,
	WindowID windowId,
	Std::RangeFnRef<AccessibilityUpdateElement> const& range,
	Std::ConstByteSpan textData) {
	auto& backendData = *(BackendData*) pBackendDataIn;
	auto& windowNode = *implData.GetWindowNode(windowId);
	auto& perWindowData = (PerWindowData & ) * windowNode.backendData.Get();

	perWindowData.accessText.resize(textData.Size());
	std::memcpy(
		perWindowData.accessText.data(),
		textData.Data(),
		perWindowData.accessText.size());

	auto& outElements = perWindowData.accessElements;
	outElements.clear();
	outElements.reserve(range.Size());
	for (auto const& item: range) {
		outElements.push_back(item);
	}

	perWindowData.accessProvider->children.clear();
	// Build the the fragment objects
	for (int i = 0; i < (int) outElements.size(); i++) {
		auto const& item = outElements[i];

		auto* out = new TestAccessFragment;
		out->root = perWindowData.accessProvider;
		out->index = i;
		perWindowData.accessProvider->children.push_back(out);
	}
}

Std::StackVec<char const*, 5> Application::GetRequiredVkInstanceExtensions() noexcept {
	Std::StackVec<char const*, 5> returnVal = {};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_win32_surface");
	return returnVal;
}

Application::FileInputStream::FileInputStream() {
	static_assert(sizeof(std::FILE*) <= sizeof(FileInputStream::m_buffer));
}

Application::FileInputStream::FileInputStream(char const* path) {
	Open(path);
}

Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept {
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));
}

Application::FileInputStream::~FileInputStream() {
	Close();
}

Application::FileInputStream& Application::FileInputStream::operator=(FileInputStream&& other) noexcept {
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&this->m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));

	return *this;
}

bool Application::FileInputStream::Seek(i64 offset, SeekOrigin origin) {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	int posixOrigin = 0;
	switch (origin) {
		case SeekOrigin::Current:
			posixOrigin = SEEK_CUR;
			break;
		case SeekOrigin::Start:
			posixOrigin = SEEK_SET;
			break;
		case SeekOrigin::End:
			posixOrigin = SEEK_END;
			break;
	}
	int result = fseek(file, (long) offset, posixOrigin);
	return result == 0;
}

bool Application::FileInputStream::Read(char* output, u64 size) {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	size_t result = std::fread(output, 1, (size_t) size, file);
	return result == (size_t) size;
}

Std::Opt<u64> Application::FileInputStream::Tell() const {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return {};

	long result = ftell(file);
	if (result == long(-1))
		// Handle error
		return {};
	else
		return Std::Opt{static_cast<u64>(result)};
}

bool Application::FileInputStream::IsOpen() const {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	return file != nullptr;
}

bool Application::FileInputStream::Open(char const* path) {
	Close();
	std::FILE* file = std::fopen(path, "rb");
	std::memcpy(&m_buffer[0], &file, sizeof(std::FILE*));
	return file != nullptr;
}

void Application::FileInputStream::Close() {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file != nullptr)
		std::fclose(file);

	std::memset(&m_buffer[0], 0, sizeof(std::FILE*));
}
