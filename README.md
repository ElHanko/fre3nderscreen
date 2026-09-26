# Fre3nderScreen

Fre3nderScreen is the local touchscreen user interface for
[Fre3nder](https://github.com/ElHanko/fre3nder).

It is derived from Guppy Screen and uses Moonraker to control Klipper, but it is
no longer developed as a generic Guppy Screen distribution. The product target
is Fre3nder on the Creality Ender-3 V3 KE.

## Scope

Fre3nderScreen is intentionally specialized for the Fre3nder platform:

- Fre3nder on the Ender-3 V3 KE
- Ingenic X2000 Linux target
- one local Moonraker endpoint
- Linux framebuffer display
- Linux evdev touchscreen input
- fixed 272x480 Fre3nder portrait touchscreen layout
- Fre3nder-specific display standby and touch feedback
- SDL host simulation as a development tool

Generic multi-printer, Android, K1/K1 Max, Debian appliance, ARM release, and
other compatibility paths are not product goals. Code that only exists for
those targets is expected to be removed as the fork is simplified.

## Integration

Fre3nderScreen is built and integrated by the Fre3nder repository. The
Fre3nder system owns the target toolchain, service integration, configuration
placement, runtime devices, and release packaging.

The standalone host simulator remains useful for UI development.

## Runtime overrides

Fre3nderScreen currently supports these runtime environment variables:

- `FRE3NDERSCREEN_CONFIG`
- `FRE3NDERSCREEN_THEME_DIR`
- `FRE3NDERSCREEN_INPUT`
- `FRE3NDERSCREEN_BEEPER_INPUT`
- `FRE3NDERSCREEN_BACKLIGHT_POWER`
- `FRE3NDERSCREEN_DISPLAY_SLEEP_SEC`

These are an application/runtime interface. The Fre3nder repository provides
the actual production values.

## Development direction

The cleanup follows a simple rule:

> Keep what Fre3nder needs. Remove compatibility code that Fre3nder does not
> need.

The host simulator is retained because it materially improves UI development.
Generic target support is not retained merely for compatibility.

## Origin and license

Fre3nderScreen is derived from
[Guppy Screen](https://github.com/ballaswag/guppyscreen).

The existing Git history and copyright/license provenance are intentionally
retained. The project remains distributed under the GNU GPL v3 license found
in `LICENSE`.

Third-party components retain their own licenses.

The unchanged `assets/dejavusans_mono_14.c` was inherited from Guppy Screen.
Its name identifies DejaVu Sans Mono; the original font version and TTF file
cannot be reconstructed from this repository. See the
[DejaVu Fonts License](licenses/DEJAVU-FONTS-LICENSE.txt) (Bitstream Vera derived
terms).

The unchanged `assets/material_svg/` tree was inherited from Guppy Screen,
which credits Pictogrammers' Material Design Icons. The still-used compiled
counterparts in `assets/material_46/` derive from these SVG sources. See the
[Apache License 2.0](licenses/MATERIAL-DESIGN-ICONS-LICENSE.txt).

## Credits

Fre3nderScreen builds on work from Guppy Screen and its upstream dependencies,
including LVGL, Moonraker, KlipperScreen, Fluidd, Material Design Icons, and
other projects already represented in the repository history and source tree.
