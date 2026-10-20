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

Settings separates `Fre3nderPanel` (WiFi placeholder, System, Apps, Updates)
from `Fre3nderScreenPanel` (UI-local preferences and self restart).
`ManagementClient` sends HTTP/JSON requests over the existing Core Unix socket;
System, Apps and OTA pages display results without implementing platform policy.

The More → Tune page contains Fine Tune, Limits, Bed Mesh and Power. Unsupported
Guppy calibration/TMC dialogs have been removed; see [the audit](RELEASE-2026.2.md).
Configured Load/Unload commands are enabled from `printer.gcode.help` results.
No Guppy filament parameters are injected. Keep the existing `default_macros`
configuration keys; they are also used by the package recipe.

LVGL, lv_drivers, libhv and spdlog are active submodules. The Makefile compiles
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

The application Makefile accepts:

- `CROSS_COMPILE`: target compiler prefix; unset selects the SDL simulator.
- `FRE3NDERSCREEN_VERSION`: version displayed in the UI; otherwise `dev-snapshot`.

The target uses `LV_DISP_ROT_90` and touch calibration. The SDL simulator runs
at 272×480 using development-only Moonraker and Management mocks. The dependency
patches in `patches/` remain required by the production cross-builder.

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

After an authorized simulator build, test the changed Tune navigation, command
availability in Extrude and explicit LED fixture, then check System, Apps and
Updates for integration regressions. A rebuild after a fix requires new approval.

See [RELEASE-2026.2.md](RELEASE-2026.2.md) for downstream changes and the existing
release commands. Hardware deployment is a separate Fre3nder operation.
