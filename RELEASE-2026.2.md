# Fre3nderScreen 2026.2 preparation

Status: upstream migration and downstream integration are complete at Screen
`3a8ad6a`. Signed Development app `2026.2.3a8ad6a-fre3nder.0.937984e` passed the
reported build and hardware checks below. This is not publication or
qualification of a production `2026.2-fre3nder.2` package. The previously
recorded published release is [2026.1.1](https://github.com/ElHanko/fre3nderscreen/releases/tag/2026.1.1).

## Cleanup findings

The following findings record the initial cleanup at `65b37b9`; downstream
requirements in this table have since been implemented as described below.
The audit starts at `65b37b9`. It follows the application entry point, member
construction, UI callbacks, Moonraker registration, Makefile wildcards, asset
symbols and runtime paths. Fre3nder and fre3nder-apps were inspected read-only.
Absence of a backend below means absence from their tracked production inputs;
it does not make claims about private operator configurations.

| Component | Evidence and disposition |
| --- | --- |
| `src/subprocess.hpp` | No include, consumer, registration or build reference. Removed unused 2,104-line library. |
| `wpa_supplicant/` | 847 inherited files; no application, Makefile, CI or simulator consumer. Removed. Downstream references only checked/copied its license; these must change before updating the source pin. |
| Belts Calibration | Reachable through More → Tune. Required `GUPPY_BELTS_SHAPER_CALIBRATION`, `GUPPY_EXCITATE_AXIS_AT_FREQ` and custom completion/graph output; no Fre3nder implementation. Removed dialog and entry. The KE uses Cartesian kinematics, not the inherited coupled-belt workflow. |
| Input Shaper | Reachable through More → Tune. `TEST_RESONANCES` was standard, but graphs/results required `guppy_input_shaper` and Save required missing `SAVE_INPUT_SHAPER`. Removed incomplete dialog and entry. |
| TMC Status | Reachable through More → Tune. Required `_GUPPY_LOAD_MODULE`, `_GUPPY_UNLOAD_MODULE` and custom `tmcstatus`; no backend. Removed dialog, metric container, exclusive spinbox helper and forced `tmcstatus` subscription. |
| TMC Tune | Constructed and shown as a disabled entry; could enable only with a host-local `motor_database.cfg`. Required missing `autotune_tmc` and `_GUPPY_*_CONFIG` helpers. Removed dialog, entry and host-path probe. |
| Filament macros | Package defaults already use `LOAD_FILAMENT` / `UNLOAD_FILAMENT`, but tracked Core printer config defines neither. Retained configurable commands; buttons stay disabled unless `printer.gcode.help` advertises them. Removed Guppy defaults and parameter injection. Standard Extrude/Retract remain. |
| K1 defaults | Removed implicit `output_pin LED` discovery and fan0/fan1/fan2/chamber defaults. Explicit configured pins and standard Moonraker discovery remain. Simulator LED is now explicitly configured in its fixture. |
| Calibration helpers | Removed `KUtils::is_homed`, calibration-file download helper and now-unused thumbnail-path accessor after their last consumers disappeared. Thumbnail retrieval used by Files/print status remains. |
| Assets and screenshots | Removed 31 SVGs without a compiled counterpart or runtime consumer, and 13 unreferenced historical landscape screenshots (including obsolete direct WiFi UI). All 25 compiled icons and the console's DejaVu font have active consumers and remain. |
| Simulator and funding | Removed obsolete `GUPPY_SMALL_SCREEN` / `GUPPY_ROTATE` arguments and commented cursor example. Removed upstream donation routing from `.github/FUNDING.yml`. |

Klipper's [SHAPER_CALIBRATE](https://www.klipper3d.org/G-Codes.html#shaper_calibrate)
provides calibration and can stage results for `SAVE_CONFIG` without the Guppy
shell helper. Its output is not the old diagram protocol. The existing Console
can send standard commands advertised by Klipper; no replacement UI or backend
was added. [DUMP_TMC](https://www.klipper3d.org/G-Codes.html#dump_tmc) provides a
register snapshot, not the removed live metrics or autotuning contract.

### Retained scope and provenance

- LVGL 9.6.0, libhv 1.3.4 (embedded nlohmann/json 3.12.0) and spdlog 1.17.0
  (bundled fmt 12.1.0) provide rendering/input, Moonraker/JSON and logging.
  Integrated LVGL drivers replace `lv_drivers`.
  All three old dependency patches are removed; the sole remaining patch adds
  opt-in evdev raw coordinates for the existing affine touch calibration.
  LodePNG's Zlib notice is retained with LVGL's MIT notice in the app artifact.
  The migration passed the reported Development build and hardware checks below.
- Theme selection constructs `<theme>.json` dynamically; all six themes are
  required by the current neutral-artifact/import file set. Retained.
- Spoolman, power, LED and generic fan/sensor handling are reachable standard
  Moonraker/Klipper clients, with runtime capability/configuration paths.
  They are not proof of support for another printer. Retained.
- WiFi remains an explicit integration-pending page. Core owns networking;
  there is no direct WPA implementation in Screen.
- The remaining executable-source `guppyscreen` reference is the touch-beep
  provenance URL. Other Guppy/K1 tokens in tests guard against obsolete scope.
  These are provenance and negative assertions, not active Guppy protocols.
- Root GPL text, upstream history, source copyright notices, touch-calibration
  notices, DejaVu and Material Design Icons license files remain. Removing the
  entire unused WPA component also removes its own source-tree license; no WPA
  code remains in Screen. Core's independent Buildroot WPA package is unaffected.

## Downstream integration

Fre3nder pins `3a8ad6a` and builds the neutral X2000 artifact. fre3nder-apps
imports and signs it with the matching exact file set: one binary, six themes,
six license texts, artifact provenance and checksums. The inherited WPA client
and its app license member are removed; Core's independent Buildroot
`wpa_supplicant` remains. The only dependency patch is the opt-in LVGL evdev
raw-mode patch. No Management or OTA API change was needed.

The current package tools are app-local:
`apps/fre3nderscreen/scripts/prepare-fre3nderscreen` and
`apps/fre3nderscreen/scripts/build-fre3nderscreen-release` in fre3nder-apps.
The production package template remains `2026.2-fre3nder.2`, release serial `2`;
the validated Development package uses serial `0`.

Fre3nder ships the signed Screen package as a RootFS Factory app. It is installed
and selected automatically on first installation. Firmware deployment preserves
an already installed version in persistent SYS; after SYS reset, signed-cache
recovery restores that version from HOME. An explicit update from the image is:

```sh
fre3nder app update /usr/share/fre3nder/factory-apps/fre3nderscreen.fre3app
```

Use Fre3nder's normal verification/update procedure. Required system apps have
a separate automatic boot reconciliation for higher release serials; Screen's
Factory bootstrap is not that update mechanism.

## Validation

The operator reports successful MIPS/X2000 cross-build, simulator build and
interactive use, and a complete Fre3nder Development build with Linux 6.6.161,
RootFS and OTA. The signed package was created and verified. On the reference
Ender-3 V3 KE, deployment and boot on slot B and the Screen update from the
embedded `.fre3app` succeeded. App `2026.2.3a8ad6a-fre3nder.0.937984e` passed
startup/rendering, touch, standby after 300 seconds, wake by touch, and tty1/tty7
switching with working touch after returning.

These results cover that Development package and the listed paths. The separate
FRE3NDER OS identity change `040b313` is implemented and tested, but has
not yet been validated on the device with a newly built RootFS. Earlier System,
Apps and full OTA-management results apply to `65b37b9`, as recorded in the
[README](README.md#hardware-qualification).

## Remaining release steps

Production packaging and publication remain separate from the completed
Development migration:

1. After explicit build/signing approval, run the existing combined release
   command from Fre3nder with reviewed inputs and a clean tracked apps worktree:

   ```sh
   scripts/build-x2000-fre3nderscreen-release \
     --apps-repo <fre3nder-apps-root> --key <private-ed25519-key>
   ```

   It cross-builds the pinned source, calls fre3nder-apps'
   `apps/fre3nderscreen/scripts/build-fre3nderscreen-release`, and prepares the
   neutral `local/production/artifacts/x2000/fre3nderscreen/app/`, signed package
   in fre3nder-apps `dist/`, and byte-identical
   `local/production/factory-apps/fre3nderscreen.fre3app`. No RootFS build or
   deployment is needed to produce that standalone package.
2. Verify the final signed package and its source/submodule/build identities.
   Separately authorize and qualify that exact production package on X2000;
   the Development results above do not qualify a later release binary.
3. Resolve the **existing `2026.2` tag before publication**. The local tag
   targets `823d090bc97f9e6b429aebda93f588450b33f40b`, before the dependency
   migration. Check remote tag state before publication.
   Do not silently move the tag. Correcting it requires explicit authorization;
   choosing another version requires consistent source/package version changes.
4. Only with separate tag/push/publication approval: publish the reviewed source
   and matching signed package, checksum and notes, retaining provenance and
   licenses. No new release pipeline is needed.

## Draft release notes — 2026.2

Fre3nderScreen 2026.2 adds native Fre3nder management to the local 272×480
Ender-3 V3 KE touchscreen:

- **System:** native system information from Fre3nder Core.
- **Apps:** native app management, including package upload and write operations.
- **Updates:** native OTA workflow through verification, preflight, backup,
  writing, readback, activation and restart requests.
- **Permissions:** the official protected service runs as unprivileged
  `fre3nder` with primary GID `fre3nder-management` in a root-owned app runtime.
  Requests use the local Unix socket; Core executes all privileged actions.
- **Upstreams:** LVGL 9.6.0 replaces 8.3.11; integrated fbdev/evdev/SDL
  drivers replace `lv_drivers`. libhv is 1.3.4 and spdlog is 1.17.0 with bundled
  fmt 12.1.0. Existing affine touch calibration and VT ownership are retained.
- **Cleanup:** removed unused WPA sources, subprocess library, unsupported
  Guppy calibration/TMC dialogs, K1 defaults and stale resources. Optional
  filament macros are enabled only when advertised by Moonraker. Upstream
  history and the notices for retained components remain included.

Hardware validation reported by the operator: Fre3nder 2026.5 with Screen
`65b37b9` passed unprivileged startup, protected runtime, native System and Apps
(including writes), and the complete OTA flow including backup, readback,
activation and restart. This establishes those paths at that source revision;
it does not qualify every function or the later cleanup/release binary.

The subsequent migration validation covers Development app
`2026.2.3a8ad6a-fre3nder.0.937984e` as described above. Before publication,
record the exact production package and its own regression results.
