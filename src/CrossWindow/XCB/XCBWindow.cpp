#include "XCBWindow.h"

namespace xwin {

Window::~Window() {
	if (mConnection != nullptr) {
		destroy();
	}
}

auto Window::create(const WindowDesc& desc, EventQueue& eventQueue, void* parentWindow) -> bool {
	const XWinState& xwinState = getXWinState();
	mConnection = xwinState.connection;
	mScreen = xwinState.screen;

	mXcbWindowId = xcb_generate_id(mConnection);
	const auto parent_window_id = (xcb_window_t)(uintptr_t)(parentWindow);

	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t value_list[2] = {
		mScreen->black_pixel,
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
			XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
			XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE};

	xcb_create_window(mConnection, XCB_COPY_FROM_PARENT, mXcbWindowId, parent_window_id,
					  desc.x, desc.y, desc.width, desc.height, 0,
					  XCB_WINDOW_CLASS_INPUT_OUTPUT, mScreen->root_visual, mask,
					  value_list);

	xcb_map_window(mConnection, mXcbWindowId);

	const unsigned coords[] = {static_cast<unsigned>(desc.x), static_cast<unsigned>(desc.y)};
	xcb_configure_window(mConnection, mXcbWindowId, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);

	// Set the child window to always be on top of the parent window
	uint32_t mode[1] = {XCB_STACK_MODE_ABOVE};
	xcb_configure_window(mConnection, mXcbWindowId, XCB_CONFIG_WINDOW_STACK_MODE, mode);

	xcb_flush(mConnection);

	return true;
}

void Window::destroy() {
	xcb_destroy_window(mConnection, mXcbWindowId);
}

auto Window::get_size(unsigned* width, unsigned* height) -> void {
	// Get the window size
	xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(mConnection, mXcbWindowId);
	xcb_get_geometry_reply_t* geometry_reply = xcb_get_geometry_reply(mConnection, geometry_cookie, nullptr);
	if (geometry_reply) {
		*width = geometry_reply->width;
		*height = geometry_reply->height;
		free(geometry_reply);
	}
}

auto Window::set_position(unsigned x, unsigned y) -> void {
	// Set the window position
	uint32_t coords[] = {x, y};
	xcb_configure_window(mConnection, mXcbWindowId, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
}

auto Window::set_size(unsigned width, unsigned height) -> void {
	// Set the window size
	uint32_t dims[] = {width, height};
	xcb_configure_window(mConnection, mXcbWindowId, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, dims);
}

} // namespace xwin