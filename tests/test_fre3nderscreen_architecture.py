#!/usr/bin/env python3

import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]

assert not (root / "src/ui_layout.h").exists()
assert not (root / "assets/material").exists()

files = [root / "Makefile", root / "lv_conf.h"]
files += sorted((root / "src").glob("*.cpp"))
files += sorted((root / "src").glob("*.h"))

forbidden = (
    "UiLayout::",
    '"ui_layout.h"',
    "default_printer",
    '"/printers',
    "GUPPY_SMALL_SCREEN",
    "GUPPY_ROTATE",
    "display_rotate",
)

for path in files:
    text = path.read_text(encoding="utf-8")
    for token in forbidden:
        assert token not in text, f"{path.relative_to(root)} still contains {token}"
    assert not re.search(r"\bdf\(\)", text), f"{path.relative_to(root)} still contains df()"

main = (root / "src/main.cpp").read_text(encoding="utf-8")
assert "lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90)" in main
assert "lv_display_get_horizontal_resolution(disp) != 480" in main
assert "lv_display_get_vertical_resolution(disp) != 272" in main
assert "LV_COLOR_FORMAT_XRGB8888" in main
assert "lv_sdl_window_create(272, 480)" in main
assert "lv_evdev_set_raw_mode(touch, true)" in main
assert "lv_tc_indev_init(touch)" in main
assert not (root / "lv_drv_conf.h").exists()
assert not (root / "lv_drivers").exists()
assert "lv_drivers" not in (root / ".gitmodules").read_text()
assert "LV_DISABLE_API_MAPPING" in (root / "Makefile").read_text()
config = (root / "lv_conf.h").read_text()
for setting in ("LV_USE_LINUX_FBDEV 1", "LV_USE_EVDEV 1", "LV_USE_SDL 1", "LV_USE_LODEPNG 1"):
    assert setting in config

sim_config = json.loads(
    (root / "dev/simulator/fre3nderscreen.json").read_text(encoding="utf-8")
)
assert "default_printer" not in sim_config
assert "printers" not in sim_config
assert sim_config["moonraker_host"] == "127.0.0.1"
assert isinstance(sim_config["moonraker_port"], int)

print("Fre3nderScreen architecture checks passed")
