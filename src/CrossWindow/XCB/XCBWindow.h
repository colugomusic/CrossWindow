#pragma once

#include "../Common/EventQueue.h"
#include "../Common/Init.h"
#include "../Common/WindowDesc.h"

#include <any>
#include <xcb/xcb.h>

namespace xwin {

struct Window {
	Window() = default;
	~Window();
	[[nodiscard]] auto get_client_data() -> std::any { return client_data; }
	[[nodiscard]] auto get_native_handle() -> void* { return (void*)(mXcbWindowId); }
	[[nodiscard]] auto create(const WindowDesc& desc, EventQueue& eventQueue, void* parentWindow) -> bool;
	[[nodiscard]] auto is_valid() const -> bool { return bool(mXcbWindowId); }
	auto destroy() -> void;
	auto get_size(unsigned* width, unsigned* height) -> void;
	auto set_client_data(std::any data) -> void { client_data = data; }
	auto set_position(unsigned x, unsigned y) -> void;
	auto set_size(unsigned width, unsigned height) -> void;
protected:
	xcb_connection_t* mConnection = nullptr;
	xcb_screen_t* mScreen = nullptr;
	unsigned mXcbWindowId = 0;
	unsigned mDisplay = 0;
	std::any client_data;
};

} // namespace xwin
