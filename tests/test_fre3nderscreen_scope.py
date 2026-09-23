#!/usr/bin/env python3

from pathlib import Path

root = Path(__file__).resolve().parents[1]

forbidden_paths = [
    ".github/workflows/build.yml",
    ".github/workflows/pull_request.yml",
    ".github/workflows/guppydroid.yml",
    "assets/zbolt",
    "debian",
    "installer-deb.sh",
    "installer.sh",
    "k1",
    "reinstall-creality.sh",
    "release.sh",
    "VERSION",
    "version.sh",
    "src/kd_graphic_mode.cpp",
    "src/platform.h",
    "src/printer_select_panel.cpp",
    "src/printer_select_panel.h",
]

for rel in forbidden_paths:
    assert not (root / rel).exists(), f"dead product scope returned: {rel}"

owned_sources = [root / "Makefile"]
owned_sources += sorted((root / "src").glob("*.cpp"))
owned_sources += sorted((root / "src").glob("*.h"))

forbidden_tokens = [
    "OS_ANDROID",
    "ZBOLT",
    "GUPPY_THEME",
    "printer_select_panel",
    "PrinterSelectPanel",
    "kd_graphic_mode",
    '"k1"',
]

for path in owned_sources:
    text = path.read_text(encoding="utf-8")
    for token in forbidden_tokens:
        assert token not in text, f"{path.relative_to(root)} still contains {token}"

print("Fre3nderScreen scope checks passed")
