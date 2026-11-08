#pragma once

#include "lvgl/lvgl.h"

namespace TouchBeep {

void init(const char *input_path);
void shutdown();
void feedback_cb(lv_event_t *event);

}
