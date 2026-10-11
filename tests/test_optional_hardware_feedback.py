#!/usr/bin/env python3

from pathlib import Path

root = Path(__file__).resolve().parents[1]

main = (root / "src" / "main.cpp").read_text()
screen = (root / "src" / "fre3nderscreen.cpp").read_text()
beep = (root / "src" / "touch_beep.cpp").read_text()

assert "FRE3NDERSCREEN_BEEPER_INPUT" in main
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

assert "FRE3NDERSCREEN_DISPLAY_SLEEP_SEC" in screen
assert "std::strtol(display_sleep_override, &end, 10)" in screen
assert "FRE3NDERSCREEN_BACKLIGHT_POWER" in screen
assert "set_backlight_power(backlight_power, '4')" in screen
assert "set_backlight_power(backlight_power, '0')" in screen
assert "LV_OBJ_FLAG_CLICKABLE" in screen

print("optional hardware feedback checks passed")

vt = (root / "src/virtual_terminal.cpp").read_text()
assert "VirtualTerminal::flush" in main
for interface in ("VT_SETMODE", "VT_RELDISP", "VT_ACTIVATE", "VT_WAITACTIVE", "KD_GRAPHICS", "KD_TEXT", "TIOCSCTTY"):
    assert interface in vt
assert "if (active) fbdev_flush" in vt
assert "else lv_disp_flush_ready" in vt
assert "active = false; // No framebuffer writes" in vt
assert "lv_indev_enable(touch, false)" in screen
assert "evdev_read(touch->driver, &ignored)" in screen
assert "lv_obj_invalidate(lv_scr_act())" in screen
assert "vt_active && display_sleep != -1" in screen
assert "VirtualTerminal::activate();" in screen
assert screen.index("VirtualTerminal::activate();") < screen.index("std::ofstream(ready_file)")
assert "TouchBeep::shutdown();" in screen
print("VT handoff static checks passed")
