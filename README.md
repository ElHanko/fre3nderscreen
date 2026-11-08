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
- Native **System**, **Apps**, **Updates** and **Backups** pages under Settings → Fre3nder.
- UI settings, display sleep, touch feedback and Screen restart.

Fre3nderScreen connects to one local Moonraker endpoint for Klipper control.
System information, app management, OTA and backup operations use Fre3nder Core's
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
and ships the signed display app as a Factory app in the RootFS at
`/usr/share/fre3nder/factory-apps/fre3nderscreen.fre3app`. First installation
automatically installs and selects it, using persistent SYS by default.
Firmware deployment does not replace an already installed Screen version;
after SYS reset, recovery restores its signed package cached in HOME. To apply
the embedded Factory app explicitly, use Fre3nder's normal update operation:

```sh
fre3nder app update /usr/share/fre3nder/factory-apps/fre3nderscreen.fre3app
```

See Fre3nder's [verification and update procedure](https://github.com/ElHanko/fre3nder/blob/main/docs/installation.md#update-the-installed-display-app).
Required system apps, such as Moonraker, use a separate boot reconciliation
that automatically applies image seeds with a higher release serial.

The protected app runtime is owned by root. The Screen service runs as the
unprivileged user `fre3nder`, with primary GID `fre3nder-management` granted by
Core for the official protected service. Network policy, device permissions,
package transactions, firmware, OTA writes and host services belong to Fre3nder.

## Hardware qualification

The upstream migration is complete at **Screen `3a8ad6a`**. The operator reports
successful validation of signed Development app
**`2026.2.3a8ad6a-fre3nder.0.937984e`** on the reference Ender-3 V3 KE:

- MIPS/X2000 cross-build and Docker/SDL build with interactive use;
- full Fre3nder Development build with Linux 6.6.161, RootFS and OTA;
- signed package creation/verification, deployment and boot on slot B, then
  updating Screen from the `.fre3app` included in the RootFS;
- startup/rendering, touch, display standby after 300 seconds, wake by touch,
  and tty1/tty7 switching with working touch after returning.

These results cover that Development package and the listed paths, not every
UI function or a later production release binary. Fre3nder's separate FRE3NDER
OS identity change `040b313` is implemented and tested, but has not yet
been validated on the device with a newly built RootFS.

Earlier, the operator reported successful tests on the real X2000 with
**Fre3nder 2026.5** and **Fre3nderScreen commit `65b37b9`**:

- unprivileged startup as `fre3nder`, primary GID `fre3nder-management`;
- protected, root-owned app runtime;
- native system information;
- app management including write operations;
- OTA management and a complete update through backup, write, readback,
  activation and restart.

Those earlier management results apply to `65b37b9`; the migration results
above cover the current Development package. A production `2026.2` release
artifact still needs its own build and regression validation.

The operator additionally reports successful testing of the backup development
changes on the reference Ender-3 V3 KE running Fre3nder **2026.5.a**: package
signature verification and installation, the backup panel on the physical
touchscreen, and a SYS backup to HOME. HOME backups and USB destinations remain
unqualified on hardware; simulator receipts do not qualify those paths.

## Development and release

The Docker/SDL simulator is the standard tool for visual and functional UI
work. See [DEVELOPMENT.md](DEVELOPMENT.md) and the
[simulator guide](dev/simulator/README.md). Builds require explicit operator
authorization; simulator results do not imply hardware qualification.

The next planned standalone release is **2026.2**, following
[2026.1.1](https://github.com/ElHanko/fre3nderscreen/releases/tag/2026.1.1).
See [the cleanup findings, release plan and draft notes](RELEASE-2026.2.md),
including the existing old `2026.2` tag and completed downstream integration.

## Origin and licenses

Fre3nderScreen derives from
[Guppy Screen](https://github.com/ballaswag/guppyscreen), with its upstream Git
history and applicable copyright notices preserved. The inherited baseline is
`0.0.26-beta`, commit `cf5c6d7539a2dca090ca71c177f57a2d96df443a`.
Fre3nderScreen is distributed under [GNU GPL v3](LICENSE).

The active submodules retain their licenses: LVGL 9.6.0 (MIT, with LodePNG
under Zlib), libhv 1.3.4 (BSD-3-Clause, with embedded nlohmann/json 3.12.0 under
MIT), and spdlog 1.17.0 with bundled fmt 12.1.0 (MIT). The separate `lv_drivers`
dependency has been removed. Exact revisions are recorded by Git submodules
and the Fre3nder artifact manifest. Local touch calibration retains its source
notices in `lv_touch_calibration/`. The sole LVGL patch adds an opt-in evdev
raw mode for the existing six-coefficient affine touch calibration.

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
