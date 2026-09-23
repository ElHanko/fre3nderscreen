# Docker UI development

This directory provides two Docker-based x86_64 development modes for the
existing Fre3nderScreen SDL simulator. The application code and LVGL UI are the
same code used by Fre3nderScreen; only the display/input backend is the existing
SDL simulator.

The repository is mounted read-only at `/src`. A separate working copy and all
x86_64 build products live below the already ignored `build/docker-sim/`
directory, so simulator objects do not contaminate MIPS build output.

## Prerequisites

- Docker with the Compose plugin
- initialized repository submodules
- `xhost` on the host for the interactive X11 mode

Initialize submodules once if needed:

```bash
git submodule update --init --recursive
```

## Interactive simulator

```bash
./dev/simulator/simulator.sh
```

The container builds Fre3nderScreen without `CROSS_COMPILE`, which activates the
existing SDL simulator, and opens the 272x480 portrait window used for Fre3nder UI development on the host X11 display.
The wrapper grants the current local user X access only for the duration of the
container and removes the grant again on exit.

## Screenshot generator

```bash
./dev/simulator/screenshot.sh home
```

The screenshot mode runs the same simulator against a private Xvfb display at
272x480 and writes:

```text
build/ui-screenshots/home.png
```

The default capture delay is three seconds. Override it when a screen needs
more time to settle:

```bash
SCREENSHOT_DELAY=5 ./dev/simulator/screenshot.sh home
```

## Moonraker safety

The tracked simulator config points to `127.0.0.1:7125` *inside the container*.
By default there is therefore no connection to a real printer.

For intentional integration testing, provide a different config from inside
the repository mount:

```bash
FRE3NDERSCREEN_CONFIG_SOURCE=/src/dev/simulator/my-fre3nderscreen.json \
  ./dev/simulator/simulator.sh
```

`host.docker.internal` is available from both services and can be used in such
a custom config to reach a Moonraker service published on the Docker host.
The selected config is copied to writable runtime storage before startup,
because Fre3nderScreen writes defaults back to its config during initialization.

## Rebuild

Normal runs use incremental compilation in `build/docker-sim/`. To force a full
simulator rebuild:

```bash
SIMULATOR_FORCE_REBUILD=1 ./dev/simulator/simulator.sh
```

or:

```bash
SIMULATOR_FORCE_REBUILD=1 ./dev/simulator/screenshot.sh home
```
