#include "XLibWindow.h"

namespace xwin {

Window::~Window() {
	destroy();
}

auto Window::destroy() -> void {
	if (window_) {
		XDestroyWindow(display_, window_);
		window_ = 0;
	}
}

auto Window::create(const WindowDesc& desc, EventQueue& eventQueue, void* parentWindow) -> bool {
	XInitThreads();
	display_  = XOpenDisplay(NULL);
	const auto screen   = DefaultScreen(display_);
	const auto visual   = DefaultVisual(display_, screen);
	const auto depth    = DefaultDepth(display_, screen);
	const auto colormap = XCreateColormap(display_, RootWindow(display_, screen), visual, AllocNone);
	XSetWindowAttributes windowAttributes = {};
	windowAttributes.colormap         = colormap;
	windowAttributes.background_pixel = 0xFFFFFFFF;
	windowAttributes.border_pixel     = 0;
	windowAttributes.event_mask       = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;
	XLibWindow parent = (parentWindow) ? reinterpret_cast<XLibWindow>(parentWindow) : RootWindow(display_, screen);
	window_ =
		XCreateWindow(display_, parent, 0, 0, desc.width,
					  desc.height, 0, depth, InputOutput, visual,
					  CWBackPixel | CWBorderPixel | CWEventMask | CWColormap,
					  &windowAttributes);
	if (parentWindow) {
		XSetTransientForHint(display_, window, parent);
	}
	XSelectInput(display_, window, ExposureMask | KeyPressMask);
	XMapWindow(display_, window);
	XFlush(display_);
	return true;
}

auto Window::get_size(unsigned* width, unsigned* height) -> void {
	XLibWindow root;
	int x, y;
	unsigned border_width, depth;
	XGetGeometry(display_, window_, &root, &x, &y, width, height, &border_width, &depth);
}

auto Window::set_position(unsigned x, unsigned y) -> void {
	XMoveWindow(display_, window_, x, y);
}

auto Window::set_size(unsigned width, unsigned height) -> void {
	XResizeWindow(display_, window_, width, height);
}

} // namespace xwin