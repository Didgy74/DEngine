#include <DEngine/Platform/PlatformAssert.hpp>
#include <DEngine/Platform/PlatformImpl.hpp>

#include "BackendData.hpp"

#include <Xinput.h>

#include <iostream>
#include <string>
#include <thread>

extern int dengine_impl_main(int argc, char const* const* argv);

using namespace DEngine;
using namespace DEngine::Platform;

namespace XPlatformToBackend = ::DEngine::Platform::impl::XPlatformToBackend;

namespace DEngine::Platform::impl::Backend {
	BackendData* pGlobalBackendData = nullptr;

	void MainPollingLoop(Context::Impl& implData, BackendData& backendData);

	void Handle_WM_KEY(WPARAM wParam, LPARAM lParam, bool down, BackendData& backendData, HWND hwnd, WindowID windowId) {
		auto VK_KEY_toPlatformButton = [](WPARAM wParam) -> Std::Opt<Platform::Button> {
			using B = Platform::Button;
			switch (wParam) {

				case VK_LEFT: return B::Left;
				case VK_RIGHT: return B::Right;
				case VK_UP: return B::Up;
				case VK_DOWN: return B::Down;

				case VK_SPACE: return B::Space;
				case VK_RETURN: return B::Enter;
				case VK_TAB: return B::Tab;
				case VK_ESCAPE: return B::Escape;
				default: return Std::nullOpt;
			}
		};

		auto btnOpt = VK_KEY_toPlatformButton(wParam);
		if (!btnOpt.Has())
			return;
		auto btn = btnOpt.Get();
		PushEventJob_ThreadSafe(
			backendData,
			[=](Context& ctx) {
				BackendInterface::UpdateButton(
					ctx.GetImplData(),
					windowId,
					btn,
					down);
			});
	}

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
					{ (u32)clientWidth, (u32)clientHeight},
					{});
			});
		}
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto implDataPtr = (Context::Impl*) GetWindowLongPtr(hwnd, 0);
		if (implDataPtr == nullptr) {
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		auto& implData = *implDataPtr;
		auto& backendData = (BackendData&)implData.GetBackendData();
		auto windowIdOpt = implData.GetWindowId(hwnd);
		if (!windowIdOpt.Has()) {
			DENGINE_IMPL_APPLICATION_UNREACHABLE();
		}
		auto windowId = windowIdOpt.Get();

		auto& perWindowDataBackend = (PerWindowData&)implData.GetWindowBackend(hwnd);

		switch (msg) {
			case WM_DPICHANGED: {
				PushEventJob_ThreadSafe(backendData,
					[=](Context& ctx) {
						BackendInterface::WindowContentScale(
							ctx.GetImplData(),
							windowId,
							(float)HIWORD(wParam) / 96);
					});
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_CHAR: {
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

			case WM_MOUSEMOVE: {
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				PushEventJob_ThreadSafe(
					backendData,
					[=](Context& ctx) {
						BackendInterface::UpdateCursorPosition(ctx.GetImplData(), windowId, {x, y});
					});
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}
			case WM_LBUTTONDOWN: {
				// TODO: Check if left mouse button is considered primary
				BackendInterface::UpdateCursorButton(implData, windowId, CursorButton::Primary, true);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}
			case WM_LBUTTONUP: {
				BackendInterface::UpdateCursorButton(implData, windowId, CursorButton::Primary, false);
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

			case WM_KEYDOWN: {
				Handle_WM_KEY(wParam, lParam, true, backendData, hwnd, windowId);
				break;
			}
			case WM_KEYUP: {
				Handle_WM_KEY(wParam, lParam, false, backendData, hwnd, windowId);
				break;
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

	Std::Opt<XPlatformToBackend::NewWindow_ReturnT> NewWindow_OnPollingThread(
		Context::Impl& implData,
		BackendData& backendData,
		Std::Span<char const> const& title,
		Extent extent);
}

namespace Backend = DEngine::Platform::impl::Backend;

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

[[nodiscard]] static std::vector<std::string> GetCommandLineArgs()
{
	int argc;
	LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::string> args;

	if (argvW)
	{
		args.reserve(argc);
		for (int i = 0; i < argc; ++i)
		{
			// Convert wide string (UTF-16) to UTF-8 std::string
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
			std::string arg(size_needed - 1, 0); // -1 to exclude null terminator
			WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, arg.data(), size_needed, nullptr, nullptr);
			args.push_back(std::move(arg));
		}

		LocalFree(argvW);
	}

	return args;
}

// Main entry for our Win32 application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	using namespace DEngine;
	using namespace DEngine::Platform::impl::Backend;

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

	// Boot the main app-thread

	auto cmdLineArgs = GetCommandLineArgs();
	auto otherThread = std::thread{[cmdLineArgs] {
		std::vector<char const*> charPtrList;
		for (auto& item : cmdLineArgs) {
			charPtrList.push_back(item.c_str());
		}
		dengine_impl_main(charPtrList.size(), charPtrList.data());
	}};

	// We wait until the other thread has called init on our system.
	auto& implData = *initWaitFuture.get();
	
	// Here we go into the main polling loop, where
	// we are gonna be for the entirety of the program.
	Backend::MainPollingLoop(implData, backendData);

	return 0;
}

void Backend::MainPollingLoop(Context::Impl& implData, BackendData& backendData) {
	MSG msg = {};
	while (true) {
		// We always wait for a new result
		int getResult = GetMessage(&msg, nullptr, 0, 0);
		if (getResult >= 0) {
			if ((CustomMessageEnum)msg.message == CustomMessageEnum::CustomMessage) {
				// Handle our custom message?
				u64 jobId = (u64)msg.wParam;
				// Find the job and execute it.
				std::scoped_lock lock{ backendData.customJobsLock };
				auto& customJobs = backendData.customJobs;
				// Find the job with the ID
				auto iterator = Std::FindIf(
					customJobs.items.begin(),
					customJobs.items.end(),
					[&](auto const& item) { return item.id == jobId; });
				DENGINE_IMPL_APPLICATION_ASSERT(iterator != customJobs.items.end());
				{
					auto& item = *iterator;
					// consumeFn takes care of cleanup.
					item.consumeFn(item.data, implData, backendData);
				}
				customJobs.items.erase(iterator);

			}
			else {
				// We got a message
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (getResult == 0) {
				// We got the quit signal.
				DENGINE_IMPL_APPLICATION_UNREACHABLE();
			}
		}
		else { // getResult < 0
			// We got an error
			DENGINE_IMPL_APPLICATION_UNREACHABLE();
		}
	}
}

void Backend::RunOnBackendThread_Inner(
	BackendData& backendData,
	RunOnBackendThread_JobItem item)
{
	u64 jobId = 0;
	{
		std::scoped_lock lock{ backendData.customJobsLock };
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
			(UINT)CustomMessageEnum::CustomMessage,
			jobId,
			0);
		// The result is nonzero if it succeeds.
		if (postResult != 0)
			break;
	}
}

Context::Impl::PlatformBackendBase* XPlatformToBackend::Initialize(Context& ctx, Context::Impl& implData) {

	implData.cursorOpt = CursorData{};

	auto& backendData = *Backend::pGlobalBackendData;
	backendData.initWaitPromise.set_value(&implData);

	return Backend::pGlobalBackendData;
}

void XPlatformToBackend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	bool waitForEvents,
	u64 timeoutNs) 
{
	// We check if there's any jobs for us
	auto& backendData = (Backend::BackendData&)backendDataIn;


	// TODO, this should probably handle waiting and sleeping?

	{
		std::scoped_lock lock{ backendData.eventJobsLock };
		backendData.queuedEventCallbacks.Consume(ctx);
	}

	// Poll all XInput slots and fire events for any changes.
	// Connected slots are polled every frame; disconnected slots are probed
	// only every xinputDisconnectedPollInterval frames to save cycles.
	bool const probeDisconnected =
		(backendData.xinputDisconnectedPollCounter % Backend::BackendData::xinputDisconnectedPollInterval) == 0;
	backendData.xinputDisconnectedPollCounter += 1;

	for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; userIndex++) {
		bool const wasConnected = (backendData.xinputConnectedMask & (1u << userIndex)) != 0;
		if (!wasConnected && !probeDisconnected)
			continue;

		XINPUT_STATE xinputState = {};
		DWORD const result = XInputGetState(userIndex, &xinputState);
		if (result != ERROR_SUCCESS) {
			backendData.xinputConnectedMask &= ~(u8)(1u << userIndex);
			continue;
		}
		backendData.xinputConnectedMask |= (u8)(1u << userIndex);

		auto const deviceId = (GamepadDeviceId)userIndex;
		auto const& gp = xinputState.Gamepad;

		// Normalize a raw XInput thumb value to [-1, 1].
		auto normalizeThumb = [](SHORT raw) -> f32 {
			return raw >= 0
				? (f32)raw / 32767.f
				: (f32)raw / 32768.f;
		};

		BackendInterface::UpdateGamepadAxis(implData, deviceId, GamepadAxis::LeftX,  normalizeThumb(gp.sThumbLX));
		BackendInterface::UpdateGamepadAxis(implData, deviceId, GamepadAxis::LeftY,  normalizeThumb(gp.sThumbLY));
		BackendInterface::UpdateGamepadAxis(implData, deviceId, GamepadAxis::RightX, normalizeThumb(gp.sThumbRX));
		BackendInterface::UpdateGamepadAxis(implData, deviceId, GamepadAxis::RightY, normalizeThumb(gp.sThumbRY));

		auto pollKey = [&](WORD mask, GamepadKey key) {
			bool const pressed = (gp.wButtons & mask) != 0;
			if (pressed != implData.gamepadState.keyStates[(int)key])
				BackendInterface::UpdateGamepadKey(implData, deviceId, key, pressed);
		};
		pollKey(XINPUT_GAMEPAD_A,               GamepadKey::A);
		pollKey(XINPUT_GAMEPAD_B,               GamepadKey::B);
		pollKey(XINPUT_GAMEPAD_X,               GamepadKey::X);
		pollKey(XINPUT_GAMEPAD_Y,               GamepadKey::Y);
		pollKey(XINPUT_GAMEPAD_LEFT_SHOULDER,   GamepadKey::L1);
		pollKey(XINPUT_GAMEPAD_RIGHT_SHOULDER,  GamepadKey::R1);
		pollKey(XINPUT_GAMEPAD_LEFT_THUMB,      GamepadKey::L3);
		pollKey(XINPUT_GAMEPAD_RIGHT_THUMB,     GamepadKey::R3);
	}
}

void XPlatformToBackend::Destroy(Context::Impl::PlatformBackendBase* data) {
}

auto XPlatformToBackend::NewWindow_Blocking(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	Std::Span<char const> const& title,
	Extent extent)
	-> Std::Opt<NewWindow_ReturnT>
{
	auto& backendData = (Backend::BackendData&)backendDataIn;
	auto windowDataFuture = Backend::RunOnBackendThread(
		backendData,
		[&](Context::Impl& implData, Backend::BackendData& backendData) {
			return Backend::NewWindow_OnPollingThread(
				implData,
				backendData,
				title,
				extent);
		});
	return windowDataFuture.get();;
}

Std::Opt<XPlatformToBackend::NewWindow_ReturnT> Backend::NewWindow_OnPollingThread(
	Context::Impl& implData,
	BackendData& backendData,
	Std::Span<char const> const& title,
	Extent extent)
{
	// This function MUST run on the message polling thread.
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

	XPlatformToBackend::NewWindow_ReturnT returnVal = {};
	returnVal.windowData.extent.width = windowInfo.rcClient.right - windowInfo.rcClient.left;
	returnVal.windowData.extent.height = windowInfo.rcClient.bottom - windowInfo.rcClient.top;
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

void XPlatformToBackend::DestroyWindow(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	Context::Impl::WindowNode const& windowNode) 
{

}

void XPlatformToBackend::Log(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	LogSeverity severity,
	Std::Span<char const> const& msg) 
{
	std::cout.write(msg.Data(), (std::streamsize)msg.Size()) << std::endl;
}

bool XPlatformToBackend::StartTextInputSession(
	Context::Impl& implData,
	WindowID windowId,
	Context::Impl::PlatformBackendBase& backendDataIn,
	SoftInputFilter inputFilter,
	Std::Span<char const> const& text,
	u64 selectionStart,
	u64 selectionCount)
{
	/*
	auto& backendData = (Backend::BackendData&)backendDataIn;
	// We gotta run this on the WinMain thread
	Backend::RunOnBackendThread_Wait(
		backendData, 
		[windowId, inputFilter, text](Context::Impl& ctx, Backend::BackendData& backendData)
		{
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.WinMainThreadId_ThreadSafe() == GetCurrentThreadId());

			HWND hwnd = {};
			{
				auto windowsLock = std::scoped_lock{ctx.windowsLock};
				auto windowIter = Std::FindIf(
					ctx.windows.begin(), 
					ctx.windows.end(),
					[&](auto const& item) { return item.id == windowId; });
				DENGINE_IMPL_APPLICATION_ASSERT(windowIter != ctx.windows.end());
				auto& windowNode = *windowIter;
				auto& perWindowData = *(Backend::PerWindowData*)windowNode.backendData.Get();
				hwnd = perWindowData.hwnd;
			}

			auto hResult = Backend::HResult_Helper::Ok;

			ITfThreadMgr* threadMgr = nullptr;
			ITfThreadMgr2* threadMgr2 = nullptr;
			ITfSourceSingle* threadMgrSource = nullptr;
			{
				hResult = (Backend::HResult_Helper)CoCreateInstance(
					CLSID_TF_ThreadMgr,
					nullptr,
					CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(&threadMgr));
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}
				void* tempPtr = nullptr;

				hResult = (Backend::HResult_Helper)threadMgr->QueryInterface(IID_ITfThreadMgr2, &tempPtr);
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}
				threadMgr2 = (ITfThreadMgr2*) tempPtr;
			}

			TfClientId clientId = {};
			hResult = (Backend::HResult_Helper)threadMgr2->Activate(&clientId);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			ITfDocumentMgr* docMgr = nullptr;
			hResult = (Backend::HResult_Helper)threadMgr2->CreateDocumentMgr(&docMgr);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			auto textStore = new Backend::TextStoreTest;
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
			hResult = (Backend::HResult_Helper)docMgr->CreateContext(
				clientId,
				0,
				(ITextStoreACP2*) textStore,
				&tsfCtx,
				&editCookie);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			void* tsfCtxSourceTemp = nullptr;
			hResult = (Backend::HResult_Helper)tsfCtx->QueryInterface(IID_ITfSource, &tsfCtxSourceTemp);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}
			auto tsfCtxSource = (ITfSource*)tsfCtxSourceTemp;

			DWORD adviseSinkIdentifier = {};
			hResult = (Backend::HResult_Helper)tsfCtxSource->AdviseSink(
				IID_ITfTextEditSink,
				(ITfTextEditSink*)textStore,
				&adviseSinkIdentifier);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			hResult = (Backend::HResult_Helper) docMgr->Push(tsfCtx);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			if (textStore->currentSink != nullptr) {
				auto& sink = *textStore->currentSink;
				hResult = (Backend::HResult_Helper)sink.OnStartEditTransaction();
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}

				TS_TEXTCHANGE textChangeRange = {};
				textChangeRange.acpStart = 0;
				textChangeRange.acpOldEnd = 0;
				textChangeRange.acpNewEnd = (LONG) textInput.Size();
				hResult = (Backend::HResult_Helper)sink.OnTextChange(0, &textChangeRange);
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}

				hResult = (Backend::HResult_Helper)sink.OnSelectionChange();
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}

				hResult = (Backend::HResult_Helper)sink.OnLayoutChange(TS_LC_CREATE, 0);
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}

				hResult = (Backend::HResult_Helper)sink.OnEndEditTransaction();
				if (hResult != Backend::HResult_Helper::Ok) {
					std::abort();
				}
			}

			hResult = (Backend::HResult_Helper)threadMgr2->SetFocus(docMgr);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			ITfDocumentMgr* oldDocMgr = nullptr;
			hResult = (Backend::HResult_Helper)threadMgr->AssociateFocus(hwnd, docMgr, &oldDocMgr);
			if (hResult != Backend::HResult_Helper::Ok) {
				std::abort();
			}

			backendData.threadMgr = threadMgr;
			backendData.threadMgr2 = threadMgr2;
			backendData.clientId = clientId;
			backendData.docMgr = docMgr;
			backendData.textStore = textStore;
		});
	*/

	return true;
}

bool XPlatformToBackend::UpdateTextInputConnection(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendData,
	WindowID windowId,
	u64 selIndex,
	u64 selCount,
	Std::Span<u32 const> text)
{
	return true;
}

void XPlatformToBackend::UpdateTextInputConnectionSelection(
	Context::Impl::PlatformBackendBase& backendDataIn,
	u64 selIndex, 
	u64 selCount) 
{
}

void XPlatformToBackend::StopTextInputSession(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn)
{

}

void XPlatformToBackend::UpdateAccessibility(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	WindowID windowId,
	Std::RangeFnRef<AccessibilityUpdateElement> const& range,
	Std::Span<char const> textData)
{
	/*
	auto& backendData = (Backend::BackendData&)backendDataIn;
	auto& windowNode = *implData.GetWindowNode(windowId);
	auto& perWindowData = (Backend::PerWindowData&)*windowNode.backendData.Get();

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

		auto* out = new Backend::TestAccessFragment;
		out->root = perWindowData.accessProvider;
		out->index = i;
		perWindowData.accessProvider->children.push_back(out);
	}*/
}

Std::StackVec<char const*, 5> Platform::GetRequiredVkInstanceExtensions() noexcept {
	Std::StackVec<char const*, 5> returnVal = {};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_win32_surface");
	return returnVal;
}

Platform::FileInputStream::FileInputStream() {
	static_assert(sizeof(std::FILE*) <= sizeof(FileInputStream::m_buffer));
}

Platform::FileInputStream::FileInputStream(char const* path) {
	Open(path);
}

Platform::FileInputStream::FileInputStream(Std::Span<char const> path) {
	std::string temp;
	temp.append(path.Data(), path.Data() + path.Size());
	Open(temp.c_str());
}

Platform::FileInputStream::FileInputStream(FileInputStream&& other) noexcept {
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));
}

Platform::FileInputStream::~FileInputStream() {
	Close();
}

Platform::FileInputStream& Platform::FileInputStream::operator=(FileInputStream&& other) noexcept {
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&this->m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));

	return *this;
}

bool Platform::FileInputStream::Seek(i64 offset, SeekOrigin origin) {
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

bool Platform::FileInputStream::Read(char* output, u64 size) {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	size_t result = std::fread(output, 1, (size_t) size, file);
	return result == (size_t) size;
}

Std::Opt<u64> Platform::FileInputStream::Tell() const {
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

bool Platform::FileInputStream::IsOpen() const {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	return file != nullptr;
}

bool Platform::FileInputStream::Open(char const* path) {
	Close();
	std::FILE* file = std::fopen(path, "rb");
	std::memcpy(&m_buffer[0], &file, sizeof(std::FILE*));
	return file != nullptr;
}

void Platform::FileInputStream::Close() {
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file != nullptr)
		std::fclose(file);

	std::memset(&m_buffer[0], 0, sizeof(std::FILE*));
}
