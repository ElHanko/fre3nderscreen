# Fre3nderScreen development

The production target is Fre3nder on the Ender-3 V3 KE / Ingenic X2000.
The logical UI is fixed at 272×480 portrait. Main navigation is Home, Control,
Files, Settings, More; subpages use fullscreen layouts.

Read [AGENTS.md](AGENTS.md) before changing code. Keep changes small and preserve
upstream licensing. Builds, commits, pushes, tags, releases and hardware actions
need their respective explicit authorization. A request to implement or validate
is not build authorization.

## Architecture

`main.cpp` initializes the runtime, display/input backend and application.
`Fre3nderScreen` owns the main UI and Moonraker connection; `InitPanel` discovers
printer objects and initializes the pages. `State` consumes Moonraker status.
`MainPanel` owns the Home, Control, Files, Settings and More navigation.

Settings separates `Fre3nderPanel` (WiFi placeholder, System, Apps, Updates, Backups)
from `Fre3nderScreenPanel` (UI-local preferences and self restart).
`ManagementClient` sends HTTP/JSON requests over the existing Core Unix socket;
System, Apps, OTA and Backup pages display results without implementing platform policy.

System → Timezone uses `GET/POST /fre3nder/api/v1/system/timezone` through the
same protected Unix peer. Two dropdowns derive regions and complete location
suffixes from Core's installed IANA catalog, including multi-level names such
as `America/Indiana/Indianapolis`. The page preselects the configured value,
shows the active value and restart requirement, and sends only the selected
identifier as JSON. Core validates and atomically stores the Root-managed
`/home/.fre3nder/system/timezone`, which survives OTA and SYS reset.
Core's S45 activates it at boot, with `Etc/UTC` as fallback. Screen never reads
or writes timezone files and carries no timezone list. Core keeps UTC/NTP
behavior, with no RTC or substitute clock; offline IANA rules do not make an
unsynchronized clock accurate. An older Core's `404` disables the controls with
an explicit message. A safe general reboot API is still missing, so the page
asks the user to restart later when idle and never calls the OTA reboot route.

The More → Tune page contains Fine Tune, Limits, Bed Mesh and Power. Unsupported
Guppy calibration/TMC dialogs have been removed; see [the audit](RELEASE-2026.2.md).
Configured Load/Unload commands are enabled from `printer.gcode.help` results.
No Guppy filament parameters are injected. Keep the existing `default_macros`
configuration keys; they are also used by the package recipe.

LVGL 9.6.0, libhv 1.3.4 (embedded nlohmann/json 3.12.0) and spdlog 1.17.0
(bundled fmt 12.1.0) are the three active submodules. `lv_drivers` is removed;
display and input drivers come from LVGL 9. The Makefile
builds LVGL through its upstream static CMake target and compiles
all `src/*.cpp`, `assets/*.c` and `assets/material_46/*.c`, plus touch calibration
for target builds. Deleting an unused `.cpp` removes its build inclusion. Assets
are referenced as LVGL symbols; themes are selected as `<theme>.json` at runtime.
The six themes are also part of the downstream artifact's fixed file set.
Do not delete resources on the basis of filenames or a single failed search.

## Build model

Fre3nder owns the Buildroot toolchain and target cross-build.
`fre3nder/configs/x2000/sources.json` is the production source pin;
`fre3nder/build/x2000/fre3nderscreen-component.sh` builds the neutral artifact.
`fre3nder-apps` imports that artifact and builds the signed `.fre3app`.
Production packaging, lifecycle scripts and deployment are outside this repo.

Building needs GNU Make, C and C++17 compilers, CMake and Python 3. The simulator
also needs SDL2 development files; its Debian 13 Docker image supplies them.
The Makefile's `build` target builds libhv's library target, spdlog and LVGL,
then the application at `build/bin/fre3nderscreen`. LVGL's static archive is
`lvgl/build/liblvgl.a`, configured with this repository's `lv_conf.h` and
`LV_DISABLE_API_MAPPING`; spdlog uses its bundled fmt. The production builder
supplies the Buildroot cross-compiler prefix and checks static MIPS32r2
hardfloat linkage/ABI. Do not reuse simulator output as a target artifact.

The application Makefile accepts:

- `CROSS_COMPILE`: target compiler prefix; unset selects the SDL simulator.
- `FRE3NDERSCREEN_VERSION`: version displayed in the UI; otherwise `dev-snapshot`.

The native Linux-fbdev display is 480×272 XRGB8888, rotated with
`LV_DISPLAY_ROTATION_90` to the logical 272×480 UI. Native evdev retains the
six affine touch coefficients and their inverse rotation. The only remaining
dependency patch, `patches/0001-lvgl-evdev-raw-mode.patch`, adds an opt-in raw
coordinate mode before evdev scaling/clamping; the production builder applies it.
VT ownership gates the native flush callback. Local framebuffer IOCTLs retain
blanking and pan reset; backlight and beeper paths still come from Fre3nder.
The SDL simulator uses LVGL's integrated software backend and native mouse,
keyboard and mousewheel at 272×480 with development-only API mocks.

The migration at `3a8ad6a` passed the MIPS/X2000 cross-build, Docker/SDL build
and interactive use, full Fre3nder Development build, signed packaging and
reference-device checks. See [the exact qualification scope](README.md#hardware-qualification)
for the installed app identity, RootFS-seed update, touch, 300-second
standby/wake and VT handoff. These results do not establish unreported page,
new-calibration or performance results, or qualify the later OS identity change.

The normal visual validation command, **only after explicit build approval**:

```sh
./dev/simulator/simulator.sh
```

It builds a Docker image and compiles the simulator. See the
[simulator guide](dev/simulator/README.md) for prerequisites, screenshots and
fixtures. Do not change production behavior just to simplify simulator tests.

## Runtime interface

Fre3nder's display manager and the `fre3nder-apps` service supply production
paths and permissions. The Screen accepts:

| Variable | Purpose |
| --- | --- |
| `FRE3NDERSCREEN_CONFIG` | Writable app JSON config; defaults beside the executable |
| `FRE3NDERSCREEN_THEME_DIR` | Read-only theme directory; defaults beside the config |
| `FRE3NDERSCREEN_FRAMEBUFFER` | Target framebuffer; defaults to `/dev/fb0` |
| `FRE3NDERSCREEN_INPUT` | Target evdev touch device |
| `FRE3NDERSCREEN_VT` | Target virtual terminal assigned by the display manager |
| `FRE3NDERSCREEN_READY_FILE` | Target readiness file required by the display manager |
| `FRE3NDERSCREEN_BEEPER_INPUT` | Optional target input sound device |
| `FRE3NDERSCREEN_BACKLIGHT_POWER` | Optional target backlight power attribute |
| `FRE3NDERSCREEN_DISPLAY_SLEEP_SEC` | Nonnegative target sleep timeout override |

`FRE3NDERSCREEN_MANAGEMENT_SOCKET` and `FRE3NDERSCREEN_USB_ROOT` are
**simulator-only** overrides. Production uses the fixed Core management socket
and `/run/fre3nder/usb`. Do not introduce alternative privileged host operations.

## Validation without a build

Run the existing Python checks individually, as CI does. None compiles C++:

```sh
python3 tests/test_fre3nderscreen_scope.py
python3 tests/test_fre3nderscreen_architecture.py
python3 tests/test_optional_hardware_feedback.py
python3 tests/test_touch_rotation.py
python3 -m unittest discover -s tests -p 'test_simulator_*.py' -v
bash -n dev/simulator/entrypoint.sh dev/simulator/simulator.sh dev/simulator/screenshot.sh
git diff --check
```

The integration tests start only the synthetic Management mock over temporary
Unix sockets. The C++ tests in `tests/` require compilation and therefore a
separate build authorization. Static checks and fixture tests cannot establish
that the application compiles or that a page works on hardware.

`tests/test_timezone_panel.cpp` exercises the actual LVGL dropdown selection,
preselection, complete multi-level identifier sent over Unix HTTP, deferred
activation and older-Core response. After an authorized simulator build it can
be compiled and run with a **separate explicit build authorization**:

```sh
g++ -std=c++17 -DSIMULATOR -DLV_DISABLE_API_MAPPING -I. -Ilibhv/include \
  tests/test_timezone_panel.cpp src/timezone_panel.cpp src/management_client.cpp \
  lvgl/build/liblvgl.a -pthread -lSDL2 -lm -o /tmp/fre3nderscreen-timezone-test
/tmp/fre3nderscreen-timezone-test
```

After an authorized simulator build, test the changed Tune navigation, command
availability in Extrude and explicit LED fixture, then check System, Apps and
Updates for integration regressions. A rebuild after a fix requires new approval.

See [RELEASE-2026.2.md](RELEASE-2026.2.md) for downstream changes and the existing
release commands. Hardware deployment is a separate Fre3nder operation.
