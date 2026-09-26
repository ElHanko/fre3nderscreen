#include "touch_beep.h"

#include "spdlog/spdlog.h"

#include <cstdint>

/*
 * Ender-3 V3 KE touch-click semantics and the 260 Hz / 4 ms / 120 ms
 * parameters were compared with the OpenKE GuppyScreen implementation:
 *
 * https://github.com/coreflake1/guppyscreen/blob/062bf5c97c8c35a24c339284e3c6ccce224ab4e6/src/touch_beep.cpp
 *
 * This implementation intentionally does not use OpenKE's userspace PWM
 * helper or /dev/mem access. It writes EV_SND/SND_TONE events to Linux's
 * pwm-beeper input device instead.
 */

#ifndef SIMULATOR
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#endif

namespace {

constexpr uint32_t CLICK_FREQUENCY_HZ = 260;
constexpr uint32_t CLICK_DURATION_MS = 4;
constexpr uint32_t CLICK_DEBOUNCE_MS = 120;

#ifndef SIMULATOR

int beeper_fd = -1;
uint32_t last_click_tick = 0;
lv_timer_t *stop_timer = nullptr;

bool write_tone(int frequency) {
    if (beeper_fd < 0) {
        return false;
    }

    struct input_event event = {};
    event.type = EV_SND;
    event.code = SND_TONE;
    event.value = frequency;

    return write(beeper_fd, &event, sizeof(event)) ==
           static_cast<ssize_t>(sizeof(event));
}

void stop_tone(lv_timer_t *timer) {
    (void)write_tone(0);

    if (stop_timer == timer) {
        stop_timer = nullptr;
    }

    lv_timer_del(timer);
}

#endif

}

namespace TouchBeep {

void init(const char *input_path) {
#ifndef SIMULATOR
    if (beeper_fd >= 0) {
        close(beeper_fd);
        beeper_fd = -1;
    }

    if (input_path == nullptr || input_path[0] == '\0') {
        return;
    }

    beeper_fd = open(input_path, O_WRONLY | O_CLOEXEC);
    if (beeper_fd < 0) {
        spdlog::warn("touch beep unavailable: cannot open {}", input_path);
        return;
    }

    spdlog::info("touch beep enabled on {}", input_path);
#else
    (void)input_path;
#endif
}

void shutdown() {
#ifndef SIMULATOR
    if (stop_timer != nullptr) {
        lv_timer_del(stop_timer);
        stop_timer = nullptr;
    }

    if (beeper_fd >= 0) {
        (void)write_tone(0);
        close(beeper_fd);
        beeper_fd = -1;
    }
#endif
}

void feedback_cb(lv_indev_drv_t * /*drv*/, uint8_t event_code) {
#ifndef SIMULATOR
    if (beeper_fd < 0 || event_code != LV_EVENT_CLICKED) {
        return;
    }

    uint32_t now = lv_tick_get();
    if (last_click_tick != 0 &&
        static_cast<uint32_t>(now - last_click_tick) < CLICK_DEBOUNCE_MS) {
        return;
    }
    last_click_tick = now;

    if (stop_timer != nullptr) {
        lv_timer_del(stop_timer);
        stop_timer = nullptr;
        (void)write_tone(0);
    }

    if (!write_tone(CLICK_FREQUENCY_HZ)) {
        spdlog::warn("touch beep disabled after input write failure");
        close(beeper_fd);
        beeper_fd = -1;
        return;
    }

    stop_timer = lv_timer_create(stop_tone, CLICK_DURATION_MS, nullptr);
    if (stop_timer == nullptr) {
        (void)write_tone(0);
    }
#else
    (void)event_code;
#endif
}

}
