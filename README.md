# Fre3nderScreen

Fre3nderScreen is the native local touchscreen UI for
[Fre3nder](https://github.com/ElHanko/fre3nder) on the **Creality Ender-3 V3 KE**.
It is an LVGL application for the Ingenic X2000, with a fixed **272×480 portrait**
layout, Linux framebuffer output and evdev touch input.

## Interface

The main navigation is **Home, Control, Files, Settings, More**:

- Printer status, temperatures, movement, extrusion, fans and configured LEDs.
- Moonraker print files, print control, macros and a G-code console.
- Fine Tune, motion limits, bed mesh and Moonraker power devices under More → Tune.
- Native **System**, **Apps** and **Updates** pages under Settings → Fre3nder.
- UI settings, display sleep, touch feedback and Screen restart.

Fre3nderScreen connects to one local Moonraker endpoint for Klipper control.
System information, app management and OTA operations use Fre3nder Core's
HTTP/JSON API over `/run/fre3nder-management/api.sock`. Core performs privileged
operations; the UI sends requests and displays their results.

WiFi is managed by Fre3nder. The Screen's WiFi page currently states that its
API integration is pending. Spoolman and power controls depend on components
actually configured in Moonraker. Load/Unload buttons require the configured
commands to be advertised by Moonraker; `LOAD_FILAMENT` and `UNLOAD_FILAMENT`
are optional macros, not built-in Klipper commands. Extrude/Retract use standard
G-code independently.

## Application and platform boundary

Fre3nder cross-builds the neutral X2000 app artifact from a pinned source commit.
[fre3nder-apps](https://github.com/ElHanko/fre3nder-apps) packages and signs the
separately installable `.fre3app`. Fre3nder's package core installs and runs it,
and can include the signed package as a factory seed.

The protected app runtime is owned by root. The Screen service runs as the
unprivileged user `fre3nder`, with primary GID `fre3nder-management` granted by
Core for the official protected service. Network policy, device permissions,
package transactions, firmware, OTA writes and host services belong to Fre3nder.

## Hardware qualification

The operator reports successful tests on the real X2000 with **Fre3nder 2026.5**
and **Fre3nderScreen commit `65b37b9`**:

- unprivileged startup as `fre3nder`, primary GID `fre3nder-management`;
- protected, root-owned app runtime;
- native system information;
- app management including write operations;
- OTA management and a complete update through backup, write, readback,
  activation and restart.

This qualification covers those tested paths at that commit. It does not
qualify every application function or the subsequent cleanup changes. The
`2026.2` release artifact still needs its own build and regression validation.

## Development and release

The Docker/SDL simulator is the standard tool for visual and functional UI
work. See [DEVELOPMENT.md](DEVELOPMENT.md) and the
[simulator guide](dev/simulator/README.md). Builds require explicit operator
authorization; simulator results do not imply hardware qualification.

The next planned standalone release is **2026.2**, following
[2026.1.1](https://github.com/ElHanko/fre3nderscreen/releases/tag/2026.1.1).
See [the cleanup findings, release plan and draft notes](RELEASE-2026.2.md),
including the existing old `2026.2` tag and required downstream license changes.

## Origin and licenses

Fre3nderScreen derives from
[Guppy Screen](https://github.com/ballaswag/guppyscreen), with its upstream Git
history and applicable copyright notices preserved. The inherited baseline is
`0.0.26-beta`, commit `cf5c6d7539a2dca090ca71c177f57a2d96df443a`.
Fre3nderScreen is distributed under [GNU GPL v3](LICENSE).

The active submodules retain their licenses: LVGL and lv_drivers (MIT), libhv
(BSD-3-Clause), and spdlog (MIT). Exact revisions are recorded by Git submodules
and the Fre3nder artifact manifest. Local touch calibration retains its source
notices in `lv_touch_calibration/`.

The inherited `assets/dejavusans_mono_14.c` remains in use by the console. Its
original font version and TTF cannot be reconstructed from this repository;
the [DejaVu Fonts License](licenses/DEJAVU-FONTS-LICENSE.txt) preserves the
Bitstream Vera derived terms.

The retained `assets/material_svg/` sources and their compiled
`assets/material_46/` counterparts were inherited from Guppy Screen, which
credits Pictogrammers' Material Design Icons. Their
[Apache License 2.0](licenses/MATERIAL-DESIGN-ICONS-LICENSE.txt) remains included.

Fre3nderScreen also acknowledges the upstream work represented in its source
and history, including Moonraker, KlipperScreen and Fluidd.
