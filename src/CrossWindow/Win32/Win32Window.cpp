#include "Win32Window.h"

#include "Shobjidl.h"
#include "dwmapi.h"
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

enum Style : DWORD {
	windowed = WS_OVERLAPPEDWINDOW,
	aero_borderless = WS_POPUP | WS_THICKFRAME,
	basicBorderless = WS_CAPTION | WS_OVERLAPPED | WS_THICKFRAME |
					  WS_MINIMIZEBOX | WS_MAXIMIZEBOX
};

HBRUSH hBrush = CreateSolidBrush(RGB(30, 30, 30));

namespace xwin {

static thread_local Window* _windowBeingCreated = nullptr;
static thread_local std::unordered_map<HWND, Window*> _hwndMap = {};
static constexpr auto CLASS_NAME = "XWinWindow";

[[nodiscard]] static
auto has_class_been_registered_already(HINSTANCE hinstance) -> bool {
	WNDCLASSEX wc;
	if (GetClassInfoEx(hinstance, CLASS_NAME, &wc)) {
		return true;
	}
	return false;
}
	
Window::~Window() {
	destroy();
}

Window::Window(Window&& rhs) noexcept : m(std::move(rhs.m)) {
	rhs.m.hwnd = 0;
}

Window& Window::operator=(Window&& rhs) noexcept {
	if (this != &rhs) {
		destroy();
		m = std::move(rhs.m);
		rhs.m.hwnd = 0;
	}
	return *this;
}

auto Window::destroy() -> void {
	if (m.hwnd) {
		DestroyWindow(m.hwnd);
		_hwndMap.erase(m.hwnd);
		m.hwnd = 0;
	}
}

auto Window::create(const WindowDesc& desc, EventQueue& eventQueue, void* parent_window) -> bool {
	const auto parent_hwnd = reinterpret_cast<HWND>(parent_window);
	m.event_queue = &eventQueue;
	const auto& xwinState    = getXWinState();
	m.hinstance      = xwinState.hInstance;
	const auto hPrevInstance = xwinState.hPrevInstance;
	const auto lpCmdLine     = xwinState.lpCmdLine;
	int nCmdShow             = xwinState.nCmdShow;
	m.bg_color   = desc.backgroundColor;
	m.min_height = desc.minHeight;
	m.min_width  = desc.minWidth;
	m.frame      = desc.frame;
	m.wnd_class.cbSize        = sizeof(WNDCLASSEX);
	m.wnd_class.style         = CS_HREDRAW | CS_VREDRAW;
	m.wnd_class.lpfnWndProc   = Window::WindowProcStatic;
	m.wnd_class.cbClsExtra    = 0;
	m.wnd_class.cbWndExtra    = WS_EX_NOPARENTNOTIFY;
	m.wnd_class.hInstance     = m.hinstance;
	m.wnd_class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	m.wnd_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
	m.wnd_class.hbrBackground = hBrush;
	m.wnd_class.lpszMenuName  = NULL;
	m.wnd_class.lpszClassName = CLASS_NAME;
	m.wnd_class.hIconSm       = LoadIcon(NULL, IDI_WINLOGO);
	if (!has_class_been_registered_already(m.hinstance)) {
		if (!RegisterClassEx(&m.wnd_class)) {
			return false;
		}
	}
	int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	if (desc.fullscreen) {
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth  = screenWidth;
		dmScreenSettings.dmPelsHeight = screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		if ((desc.width != screenWidth) && (desc.height != screenHeight)) {
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
				// Stay in Windowed mode
			}
		}
	}
	m.ex_style = 0;
	m.style    = 0;
	if (desc.fullscreen) {
		m.ex_style = WS_EX_APPWINDOW;
		m.style = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else {
		m.ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		if (desc.frame) {
			m.style = Style::windowed;
		}
		else {
			m.style = Style::basicBorderless;
		}
	}
	// Store the current thread's DPI-awareness context
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	m.window_rect.left   = desc.x;
	m.window_rect.top    = desc.y;
	m.window_rect.right  = desc.fullscreen ? (long)screenWidth : (long)desc.width;
	m.window_rect.bottom = desc.fullscreen ? (long)screenHeight : (long)desc.height;
	AdjustWindowRectEx(&m.window_rect, m.style, FALSE, m.ex_style);
	_windowBeingCreated = this;
	m.hwnd =
		CreateWindowEx(
			0, CLASS_NAME, desc.title.c_str(), m.style,
			0, 0, m.window_rect.right - m.window_rect.left,
			m.window_rect.bottom - m.window_rect.top, parent_hwnd, NULL,
			m.hinstance, NULL);
	BOOL isNCRenderingEnabled{TRUE};
	DwmSetWindowAttribute(m.hwnd, DWMWA_NCRENDERING_ENABLED, &isNCRenderingEnabled, sizeof(isNCRenderingEnabled));
	if (!m.hwnd) {
		// Failed to create window...
		return false;
	} 
	if (!desc.fullscreen) {
		// Adjust size to match DPI
		int iDpi = GetDpiForWindow(m.hwnd);
		if (iDpi != USER_DEFAULT_SCREEN_DPI) {
			m.window_rect.bottom = MulDiv(m.window_rect.bottom, iDpi, USER_DEFAULT_SCREEN_DPI);
			m.window_rect.right  = MulDiv(m.window_rect.right, iDpi, USER_DEFAULT_SCREEN_DPI);
		}
		unsigned x = (GetSystemMetrics(SM_CXSCREEN) - m.window_rect.right) / 2;
		unsigned y = (GetSystemMetrics(SM_CYSCREEN) - m.window_rect.bottom) / 2;
		// Center on screen
		SetWindowPos(m.hwnd, 0, x, y, m.window_rect.right, m.window_rect.bottom, 0);
	}
	if (desc.visible) {
		ShowWindow(m.hwnd, SW_SHOW);
		SetForegroundWindow(m.hwnd);
		SetFocus(m.hwnd);
	}
	static const DWM_BLURBEHIND blurBehind{{0}, {TRUE}, {NULL}, {TRUE}};
	DwmEnableBlurBehindWindow(m.hwnd, &blurBehind);
	static const MARGINS shadow_state[2]{{0, 0, 0, 0}, {1, 1, 1, 1}};
	DwmExtendFrameIntoClientArea(m.hwnd, &shadow_state[0]);
	RegisterWindowMessage("TaskbarButtonCreated");
	HRESULT hrf = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (LPVOID*)&m.taskbar_list);
	setProgress(0.0f);
	set_size(desc.width, desc.height);
	return true;
}

void Window::minimize() { ShowWindow(m.hwnd, SW_MINIMIZE); }

void Window::maximize() {
	if (!IsZoomed(m.hwnd)) {
		ShowWindow(m.hwnd, SW_MAXIMIZE);
	}
	else {
		ShowWindow(m.hwnd, SW_RESTORE);
	}
}

void Window::trackEventsAsync(const std::function<void(const xwin::Event e)>& fun) {
	m.event_callback = fun;
}

void Window::setProgress(float progress) {
	unsigned max = 10000;
	unsigned cur = (unsigned)(progress * (float)max);
	m.taskbar_list->SetProgressValue(m.hwnd, cur, max);
}

void Window::showMouse(bool show) {
	ShowCursor(show ? TRUE : FALSE);
}

float Window::getDpiScale() const {
	int currentDpi = GetDpiForWindow(m.hwnd);
	int defaultDpi = USER_DEFAULT_SCREEN_DPI;
	return static_cast<float>(currentDpi) / static_cast<float>(defaultDpi);
}

std::string Window::getTitle() const {
	char str[1024];
	memset(str, 0, sizeof(char) * 1024);
	GetWindowTextA(m.hwnd, str, 1024);
	std::string outStr = std::string(str);
	return outStr;
}

void Window::setTitle(std::string title) {
	SetWindowText(m.hwnd, title.c_str());
}

auto Window::set_position(unsigned x, unsigned y) -> void {
	SetWindowPos(m.hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

auto Window::set_size(unsigned width, unsigned height) -> void {
	RECT rect, frame, border;
	GetWindowRect(m.hwnd, &rect);
	DwmGetWindowAttribute(m.hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
	border.left = frame.left - rect.left;
	border.top = frame.top - rect.top;
	border.right = rect.right - frame.right;
	border.bottom = rect.bottom - frame.bottom;
	int titlebarHeight = (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CXPADDEDBORDER));
	SetWindowPos(m.hwnd, nullptr, -1, -1, width + border.right + border.left, height + border.top + border.bottom + titlebarHeight, SWP_NOMOVE | SWP_NOREDRAW);
}

unsigned Window::getBackgroundColor() {
	return m.bg_color;
}

void Window::setBackgroundColor(unsigned color) {
	m.bg_color = color;
}

UVec2 Window::getPosition() const {
	RECT lpRect;
	GetWindowRect(m.hwnd, &lpRect);
	return UVec2(lpRect.left, lpRect.top);
}

UVec2 Window::getWindowSize() const {
	RECT lpRect;
	DwmGetWindowAttribute(m.hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &lpRect, sizeof(lpRect));
	int titlebarHeight = (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CXPADDEDBORDER));
	return UVec2(lpRect.right - lpRect.left, lpRect.bottom - lpRect.top - titlebarHeight);
}

UVec2 Window::getCurrentDisplaySize() const {
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	return UVec2(static_cast<unsigned>(screenWidth), static_cast<unsigned>(screenHeight));
}

UVec2 Window::getCurrentDisplayPosition() const {
	WINDOWPLACEMENT lpwndpl = {0};
	GetWindowPlacement(m.hwnd, &lpwndpl);
	UVec2 r = UVec2(lpwndpl.ptMinPosition.x, lpwndpl.ptMinPosition.y);
	return r;
}

void Window::setMousePosition(unsigned x, unsigned y) { SetCursorPos(x, y); }

HINSTANCE Window::getHinstance() { return m.hinstance; }

auto Window::get_native_handle() -> void* {
	return (void*)(m.hwnd);
}

void Window::executeEventCallback(const xwin::Event e) {
	if (m.event_callback) {
		m.event_callback(e);
	}
}

LRESULT CALLBACK Window::WindowProcStatic(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	Window* _this;
	if (_windowBeingCreated != nullptr) {
		_hwndMap.emplace(hwnd, _windowBeingCreated);
		_windowBeingCreated->m.hwnd = hwnd;
		_this = _windowBeingCreated;
		_windowBeingCreated = nullptr;
	}
	else {
		auto existing = _hwndMap.find(hwnd);
		_this = existing->second;
	}
	return _this->WindowProc(msg, wparam, lparam);
}

LRESULT Window::WindowProc(UINT msg, WPARAM wparam, LPARAM lparam) {
	MSG message;
	message.hwnd = m.hwnd;
	message.lParam = lparam;
	message.wParam = wparam;
	message.message = msg;
	message.time = 0;
	LRESULT result = m.event_queue->pushEvent(message, this);
	if (result > 0) return result;
	return DefWindowProc(m.hwnd, msg, wparam, lparam);
}

} // namespace xwin