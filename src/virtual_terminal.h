#pragma once

#ifndef SIMULATOR
#include "lvgl/lvgl.h"

namespace VirtualTerminal {
enum Event { None, Release, Acquire, Stop };
void init(const char *path);
void activate();
Event poll();
void acknowledge(bool acquired);
void shutdown();
void attach_display(lv_display_t *display, const char *framebuffer_path);
void blank(bool enabled);
void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels);
}
#endif
