#!/usr/bin/env python3

import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]

assert not (root / "src/ui_layout.h").exists()
assert not (root / "assets/material").exists()

files = [root / "Makefile", root / "lv_drv_conf.h"]
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
assert "disp_drv.sw_rotate = 1;" in main
assert "disp_drv.rotated = LV_DISP_ROT_90;" in main

driver = (root / "lv_drv_conf.h").read_text(encoding="utf-8")
sim = driver.split("#else // SIMULATOR", 1)[1]
assert re.search(r"#\s*define\s+SDL_HOR_RES\s+272\b", sim)
assert re.search(r"#\s*define\s+SDL_VER_RES\s+480\b", sim)
assert "EVDEV_CALIBRATE" not in (root / "Makefile").read_text(encoding="utf-8")
assert re.search(
    r"#\s*define\s+EVDEV_CALIBRATE\s+1\b",
    driver,
), "touch calibration must stay enabled in lv_drv_conf.h"

sim_config = json.loads(
    (root / "dev/simulator/fre3nderscreen.json").read_text(encoding="utf-8")
)
assert "default_printer" not in sim_config
assert "printers" not in sim_config
assert sim_config["moonraker_host"] == "127.0.0.1"
assert isinstance(sim_config["moonraker_port"], int)

print("Fre3nderScreen architecture checks passed")
