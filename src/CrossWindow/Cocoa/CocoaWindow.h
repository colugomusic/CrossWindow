#pragma once

#include "../Common/EventQueue.h"
#include "../Common/Init.h"
#include "../Common/WindowDesc.h"

#include <any>
#include <memory>
#include <vector>

namespace xwin
{
struct Window {
	~Window();
	[[nodiscard]] auto create(const WindowDesc& desc, EventQueue& eventQueue, void* parentWindow) -> bool;
	[[nodiscard]] auto get_client_data() -> std::any { return client_data; }
	[[nodiscard]] auto get_native_handle() -> void*;
	[[nodiscard]] auto is_valid() const -> bool { return bool(view); }
	auto destroy() -> void;
	auto set_client_data(std::any data) -> void { client_data = data; }
	auto set_position(unsigned x, unsigned y) -> void;
	auto set_size(unsigned width, unsigned height) -> void;
	auto get_native_handle() -> void*;
	void setMousePosition(unsigned x, unsigned y);
	UVec2 getCurrentDisplaySize();
	// MacOS Only Functions:
	enum class LayerType {
		Metal,
		OpenGL,
		LayerTypeMax
	};
	// Set the type of this window's view layer
	void setLayer(LayerType type);
  protected:
	std::any client_data;
	// NSString*
	void* mTitle = nullptr;
	// XWinWindow*
	void* window = nullptr;
	// XWinView*
	void* view = nullptr;
	// Any Layer Type
	void* layer = nullptr;
	// https://stackoverflow.com/questions/3202629/where-can-i-find-a-list-of-mac-virtual-key-codes
	typedef Key MacKeycodeToDigitalInputMap[1 << (8 * sizeof(unsigned char))];
};

} // namespace xwin
