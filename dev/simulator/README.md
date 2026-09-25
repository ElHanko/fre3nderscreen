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

Initialize submodules once if needed:

```bash
git submodule update --init --recursive
```

## Interactive simulator

```bash
./dev/simulator/simulator.sh
```

The container builds Fre3nderScreen without `CROSS_COMPILE`, which activates the
existing SDL simulator. The simulator runs on a private 272x480 Xvfb display
inside the container and is exposed locally through both VNC and noVNC:

```text
VNC:     127.0.0.1:5901
Browser: http://127.0.0.1:6080/vnc.html?autoconnect=1&resize=scale
```

The container's VNC server listens on port 5900 internally. Docker maps it to
host port 5901 so it does not collide with a VNC server using the host's normal
5900 port. Both host ports are bound to `127.0.0.1`, so the simulator is not
published to the LAN.

Mouse input through VNC or noVNC is delivered to the same SDL input path used by
the simulator. No X11 display, `xhost` permission, VNC server, or noVNC install
is required on the host.

## Simulated Moonraker

The simulator starts a small Moonraker-compatible WebSocket service inside the
same container on `127.0.0.1:7125`. Nothing is published to the host or LAN.

The mock exists only to initialize the real Fre3nderScreen UI with deterministic
development data. By default it reports:

- nozzle: 21 C, target 0 C
- bed: 52 C, target 0 C
- printer state: ready / standby
- no files, macros, power devices, or external components

Temperature edits from the UI are accepted and the simulated temperature moves
toward the selected target, so normal and active-heating states can both be
checked without touching a real printer.

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

The tracked simulator config points to `127.0.0.1:7125` *inside the container*,
which is the simulator-only Moonraker mock. By default there is therefore no
connection to a real printer.

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
