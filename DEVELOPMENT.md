# Fre3nderScreen development

Fre3nderScreen is the dedicated local touchscreen UI for Fre3nder.

The production target is Fre3nder on the Ender-3 V3 KE. The x86/SDL build is
kept as a development simulator so UI work can be tested without repeatedly
deploying to the printer.

## Dependencies

The current source tree uses:

- LVGL
- lv_drivers
- libhv
- spdlog
- wpa_supplicant client code

These dependencies are not automatically permanent. During the Fre3nderScreen
cleanup, each dependency should remain only while Fre3nderScreen still uses it.

## Build model

### Fre3nder target

The Fre3nder repository owns the target toolchain and production integration.
Use its Fre3nderScreen component build path for target artifacts.

This repository intentionally does not ship K1, Debian/Raspberry Pi, Android,
standalone installer, or release-packaging paths. Production packaging and
deployment belong to Fre3nder.

The application build accepts:

- `CROSS_COMPILE` - target toolchain prefix
- `FRE3NDERSCREEN_VERSION` - version string exposed in the UI
- `GUPPY_SMALL_SCREEN` - legacy build switch, pending cleanup
- `GUPPY_ROTATE` - legacy build switch, pending cleanup
- `EVDEV_CALIBRATE` - legacy calibration build switch, pending cleanup

The legacy `GUPPY_*` build switches are deliberately not renamed in this
identity migration because their behavior will be simplified in a later
Fre3nder-specific cleanup.

### Host simulator

With `CROSS_COMPILE` unset, the Makefile builds the SDL simulator.

Typical development prerequisites on Debian/Ubuntu:

```sh
sudo apt-get install -y build-essential cmake libsdl2-dev
```

Then:

```sh
make build
```

The resulting executable is:

```text
build/bin/fre3nderscreen
```

No target build is performed by the migration script.

## Runtime interface

The current runtime path overrides are:

- `FRE3NDERSCREEN_CONFIG`
- `FRE3NDERSCREEN_THEME_DIR`
- `FRE3NDERSCREEN_INPUT`
- `FRE3NDERSCREEN_BEEPER_INPUT`
- `FRE3NDERSCREEN_BACKLIGHT_POWER`
- `FRE3NDERSCREEN_DISPLAY_SLEEP_SEC`

The default application config filename is `fre3nderscreen.json`.

## Cleanup principle

Fre3nderScreen is not intended to remain a generic Guppy Screen downstream.

When a code path only exists for another printer, architecture, operating
system, release format, or multi-printer use case, it should be removed unless
there is a concrete Fre3nder requirement for it.

The SDL simulator is the main exception: it is retained as a development tool,
not as a supported printer platform.

## Provenance

Fre3nderScreen is derived from Guppy Screen. Keep upstream history, license
information, and third-party provenance intact while removing unused product
scope.
