#pragma once

#include "lvgl/lvgl.h"

namespace TouchBeep {

void init(const char *input_path);
void feedback_cb(lv_indev_drv_t *drv, uint8_t event_code);

}
