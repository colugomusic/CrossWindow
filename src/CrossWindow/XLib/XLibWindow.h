#pragma once

#include "../Common/EventQueue.h"
#include "../Common/Init.h"
#include "../Common/WindowDesc.h"
#include <any>
#include <memory>
#include <X11/Xlib.h>

typedef Window XLibWindow;

namespace xwin {

struct Window {
	Window() = default;
	~Window();
	[[nodiscard]] auto get_client_data() -> std::any { return client_data; }
	[[nodiscard]] auto get_native_handle() -> void* { return (void*)(window_); }
	[[nodiscard]] auto create(const WindowDesc& desc, EventQueue& eventQueue, void* parentWindow) -> bool;
	[[nodiscard]] auto is_valid() const -> bool { return bool(window_); }
	auto destroy() -> void;
	auto get_size(unsigned* width, unsigned* height) -> void;
	auto set_client_data(std::any data) -> void { client_data = data; }
	auto set_position(unsigned x, unsigned y) -> void;
	auto set_size(unsigned width, unsigned height) -> void;
protected:
	Window(Display* display, XLibWindow window) : display_(display), window_(window) {}
	Display* display_  = 0;
	XLibWindow window_ = 0;
	std::any client_data;
};

} // namespace xwin
