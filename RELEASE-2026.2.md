# Fre3nderScreen 2026.2 preparation

Status: source cleanup and documentation prepared; no cleanup build, final
release artifact, commit or publication performed by this task. The last
published release is [2026.1.1](https://github.com/ElHanko/fre3nderscreen/releases/tag/2026.1.1).

## Cleanup findings

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

- LVGL/lv_drivers, libhv and spdlog are used by rendering/input, Moonraker/JSON
  and logging. Their pinned submodules, licenses and three production patches
  remain. No third-party submodule contents were changed.
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

## Required downstream changes

These changes were identified but **not applied**:

1. **Fre3nder:** in `build/x2000/fre3nderscreen-component.sh`, remove the
   `wpa_supplicant/COPYING` check and the copy to `WPA-SUPPLICANT-LICENSE`.
   In `configs/x2000/sources.json`, remove only
   `userspace.fre3nderscreen.vendored_dependency`; preserve the independent
   `userspace.wpa_supplicant` platform package. Pin the reviewed final Screen
   commit, retaining release `2026.2` and the unchanged four submodule pins.
   Update the old pin assertion in `tests/test-x2000-fre3nderscreen-service`,
   `docs/licensing-and-provenance.md` and `docs/roadmap.md` to the final source,
   reduced license inventory and current qualification scope.
2. **fre3nder-apps:** remove `WPA-SUPPLICANT-LICENSE` from `LICENSES` in
   `scripts/prepare-fre3nderscreen` and both fixture file lists in
   `tests/test_fre3nderscreen_recipe.py`. Update
   `apps/fre3nderscreen/licenses/UPSTREAM` and `docs/fre3nderscreen-recipe.md`
   (seven imported license texts instead of eight, current source and hardware
   qualification). Keep importer and artifact file manifests consistent.
3. No Management or OTA API change is required. Do not add Guppy modules or
   shell-command support to Core. Load/Unload need operator-provided macros
   only if automatic filament handling is wanted; manual extrusion already
   works without them. WiFi UI integration is separate work.

Updating the Screen pin alone currently fails the builder's missing-license
check; updating only the builder then fails the importer's exact file-set check.
Coordinate both repositories before the release build. Existing artifacts from
the old pin keep their original notices.

## Validation completed without a build

The four existing static Python checks and all eight Management/OTA mock
integration tests passed. The mock tests ran outside the sandbox because it
blocks temporary Unix-socket binding. Shell syntax, JSON, own-header references,
all retained icon/font consumers, SVG counterparts, documentation links and
`git diff --check` passed. The deletion inventory and remaining Guppy references
were reviewed; retained license files and submodules are unchanged. Added text
was checked for secrets, personal paths and device identifiers. No C++ compilation
or UI runtime test was performed. The other repositories remain unchanged.

## Release steps

1. Review and approve the cleanup; obtain explicit approval for the simulator
   build `./dev/simulator/simulator.sh` from the Screen root. Expected result:
   compiled SDL UI with the mock services. Check More → Tune's four remaining
   entries and back navigation, Files/console, disabled Load/Unload with no
   macro, enabled custom commands when advertised, manual extrusion controls,
   explicit LED fixture, and System/Apps/Updates mock flows. Do not send these
   tests to hardware. Record results; fixes require separately approved rebuilds.
2. After review and separate commit/push authorization, make the final source
   commit fetchable. Obtain approval for the downstream edits above, run their
   relevant non-build recipe/wrapper tests: Fre3nder's
   `tests/test-x2000-fre3nderscreen-service` and
   `tests/test_x2000_fre3nderscreen_release.py`, and fre3nder-apps'
   `tests/test_fre3nderscreen_recipe.py` and
   `tests/test_build_fre3nderscreen_release.py`. Commit those changes separately
   when authorized. Check the package template: currently
   `2026.2-fre3nder.2`, `release_serial = 2`; the serial must remain greater than
   previously released packages. Development serial `0` is not a release.
3. Obtain approval for the existing combined release command, run from Fre3nder:

   ```sh
   scripts/build-x2000-fre3nderscreen-release \
     --apps-repo <fre3nder-apps-root> --key <private-ed25519-key>
   ```

   This runs `scripts/build-x2000-fre3nderscreen` for the release pin, then
   fre3nder-apps' `scripts/build-fre3nderscreen-release`. It checks MIPS ABI,
   static linkage, source identity and checksums; packages/signs the artifact;
   and copies the identical package to the local factory-seed directory.
   Expected outputs: neutral `local/production/artifacts/x2000/fre3nderscreen/app/`,
   signed `.fre3app` in fre3nder-apps `dist/`, and local
   `local/production/factory-apps/fre3nderscreen.fre3app`. No RootFS build or
   deployment is needed to produce this standalone release package.
4. Verify the signed package and recorded source/submodule/build identities
   through the existing package workflow. Any on-printer regression/deployment
   requires separate authorization under Fre3nder's rules. The operator-reported
   `65b37b9` tests below do not qualify the final cleanup binary.
5. Resolve the **existing `2026.2` tag before publication**. Local and remote
   tag object `80334527f1eb5d4a83905dc0e9f136de34ca325f` targets
   `63e7ecb9fff980b53f4987ef9994675aecf9e0a2`; it excludes native management.
   GitHub's release listing checked on 2026-10-19 contains `2026.1.1` and
   `2026.1`, with no `2026.2` release. Do not silently move the tag. The operator
   must explicitly authorize correcting the prepared tag, or choose a new
   version and update the pin/package version consistently.
6. Only with separate tag/push/publication approval: ensure the release tag
   resolves to the reviewed final source commit, publish GitHub release notes
   and attach the matching signed `.fre3app` and checksum. Preserve matching
   source, submodule and license provenance. No new release pipeline is needed.

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
- **Cleanup:** removed unused WPA sources, subprocess library, unsupported
  Guppy calibration/TMC dialogs, K1 defaults and stale resources. Optional
  filament macros are enabled only when advertised by Moonraker. Upstream
  history and the notices for retained components remain included.

Hardware validation reported by the operator: Fre3nder 2026.5 with Screen
`65b37b9` passed unprivileged startup, protected runtime, native System and Apps
(including writes), and the complete OTA flow including backup, readback,
activation and restart. This establishes those paths at that source revision;
it does not qualify every function or the later cleanup/release binary.

Before publication, add the actual final build/package and regression results.
The cleanup currently has no build or simulator runtime qualification.
