#pragma once

#include "../Common/Event.h"

#include <queue>

#include <X11/Xlib.h>

namespace xwin
{
class Window;

class EventQueue
{
  public:
    void pushEvent(const XEvent* event, Window* window);

  protected:
    std::queue<Event> mQueue;
};
}