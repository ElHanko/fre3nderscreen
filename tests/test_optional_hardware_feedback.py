#!/usr/bin/env python3

from pathlib import Path

root = Path(__file__).resolve().parents[1]

main = (root / "src" / "main.cpp").read_text()
screen = (root / "src" / "guppyscreen.cpp").read_text()
beep = (root / "src" / "touch_beep.cpp").read_text()

assert "GUPPYSCREEN_BEEPER_INPUT" in main
assert "indev_drv_1.feedback_cb = TouchBeep::feedback_cb;" in main

assert "EV_SND" in beep
assert "SND_TONE" in beep
assert "CLICK_FREQUENCY_HZ = 260" in beep
assert "CLICK_DURATION_MS = 4" in beep
assert "CLICK_DEBOUNCE_MS = 120" in beep
assert "event_code != LV_EVENT_CLICKED" in beep
assert "lv_timer_create(stop_tone, CLICK_DURATION_MS, nullptr)" in beep

# /dev/mem may legitimately occur in the provenance comment. What must not
# exist is code opening it for direct hardware access.
assert 'open("/dev/mem"' not in beep
assert "open('/dev/mem'" not in beep

assert "GUPPYSCREEN_DISPLAY_SLEEP_SEC" in screen
assert "std::strtol(display_sleep_override, &end, 10)" in screen
assert "GUPPYSCREEN_BACKLIGHT_POWER" in screen
assert "set_backlight_power(backlight_power, '4')" in screen
assert "set_backlight_power(backlight_power, '0')" in screen
assert "LV_OBJ_FLAG_CLICKABLE" in screen

print("optional hardware feedback checks passed")
