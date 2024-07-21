#pragma once

#include "../Common/EventQueue.h"
#include "../Common/Init.h"
#include "../Common/WindowDesc.h"

#include <any>
#include <functional>
#include <memory>
#include <unordered_map>
#include <Windows.h>

struct ITaskbarList3;

namespace xwin {

struct Window {
	~Window();
	[[nodiscard]] auto create(const WindowDesc& desc, EventQueue& eventQueue, void* parentWindow) -> bool;
	[[nodiscard]] auto get_client_data() -> std::any { return m.client_data; }
	[[nodiscard]] auto is_valid() const -> bool { return m.hwnd != 0; }
	auto destroy() -> void;
	auto set_client_data(std::any data) -> void { m.client_data = data; }
	std::string getTitle() const;
	void setTitle(std::string title);
	UVec2 getPosition() const;
	void showMouse(bool show);
	void setMousePosition(unsigned x, unsigned y);
	UVec2 getWindowSize() const;
	UVec2 getCurrentDisplaySize() const;
	UVec2 getCurrentDisplayPosition() const;
	float getDpiScale() const;
	unsigned getBackgroundColor();
	void setBackgroundColor(unsigned color);
	void minimize();
	void maximize();
	void trackEventsAsync(const std::function<void(const xwin::Event e)>& fun);

	// Windows Only Functions:
	void setProgress(float progress);
	HINSTANCE getHinstance();
	auto get_native_handle() -> void*;
	auto set_position(unsigned x, unsigned y) -> void;
	auto set_size(unsigned width, unsigned height) -> void;
	auto executeEventCallback(const xwin::Event e) -> void;

	/**
	 * It's possible to define regions in the window as part of the titlebar,
	 * a help region, maximize, minimize buttons, and much more.
	 */

	// The hit rectangle's type (eg. this rectangle is a titlebar)
	enum class HitRectType : size_t {
		None,
		TitleBar,
		Maximize,
		Minimize,
		Close,
		Help,
		HitRectTypeMax
	};
	// A Win32 special hit region, can be a titlebar, maximize button, etc.
	struct HitRect {
		UVec2 position;
		UVec2 size;
		HitRectType type;
	};
	std::vector<HitRect> hitRects;
	friend class EventQueue;
private:
	struct members {
		bool frame                  = true;
		DEVMODE screen_settings     = {0};
		DWORD ex_style              = 0;
		DWORD style                 = 0;
		EventQueue* event_queue     = nullptr;
		HINSTANCE hinstance         = 0;
		HWND hwnd                   = 0;
		ITaskbarList3* taskbar_list = nullptr;
		RECT window_rect            = {0};
		unsigned bg_color           = 0xFFFFFFFF;
		unsigned min_height         = 0;
		unsigned min_width          = 0;
		WNDCLASSEX wnd_class        = {0};
		std::function<void(const xwin::Event e)> event_callback;
		std::any client_data;
	};
	static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam); 
	LRESULT WindowProc(UINT msg, WPARAM wparam, LPARAM lparam); 
	members m;
};

} // namespace xwin
