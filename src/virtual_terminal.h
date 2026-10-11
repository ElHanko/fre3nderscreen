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
void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels);
}
#endif
