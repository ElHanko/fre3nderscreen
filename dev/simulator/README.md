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

## Simulated Fre3nder Management API

In both interactive and screenshot modes, the simulator starts
`dev/simulator/mock_management.py` alongside the existing Moonraker mock.
The mock listens **only** on the container-local Unix-domain socket
`/work/runtime/management.sock` (permissions `0600`). The launcher exports
`FRE3NDERSCREEN_MANAGEMENT_SOCKET` to the simulated Fre3nderScreen process.
No TCP port is opened, no management web UI is required, and the real Core
Management daemon is neither accessed nor modified.

The mock implements the System, Apps and OTA API routes needed by the
native pages, with HTTP/1.0 JSON responses and synthetic state only. The
test-network addresses (`192.0.2.0/24`) and hostname are not real device data.
Undocumented routes are rejected. No production system or signing keys are used.

Run the mock's standalone, non-build integration tests from the repository root:

```sh
python3 -m unittest discover -s tests -p 'test_simulator_management.py' -v
```

The System page loads a snapshot on entry or Refresh. This fixture tests its
layout and formatting, **not** production permissions or on-device behavior.
The failure state remains testable by stopping the mock or using an invalid
socket path in a separate development run.

## Simulated Apps management

The native Apps page uses the same Unix socket as System. The simulator mock
now serves `apps/status`, `apps/detail` and a **simulated** version of the
existing POST Apps contract. Changes affect only in-memory fixture data.
There is **no package signature verification** in the mock, and the runtime
contains a dummy `sample-app.fre3app` under `/work/runtime/usb/` exclusively to
exercise USB selection/upload on the simulated display. Never deploy this file
or assume simulator acceptance proves that a real package is trusted.

On a real Fre3nder installation the Screen uses the fixed, existing
`/run/fre3nder/usb` mount and the Core Management API, which verifies signatures
and performs all package transactions. The Screen only selects files and sends
explicit operator requests. It never touches package state or system files.

## Simulated OTA updates

The native **Updates** page uses the local Management Unix socket. The simulator
mock follows the existing OTA API steps: upload/verify, preflight, backup-plan,
backup, confirmation, write, readback, activation preparation and reboot request.

All packages, backups, transactions, block targets, signatures and restart
outcomes are **synthetic**. The sample `.ota` under `/work/runtime/usb/` is an
invalid archive for production use and must never be installed on hardware.
The mock never writes a kernel, RootFS, backup archive or boot selector, never
reboots the host/container and never marks an update boot as validated.
The production UI sends only explicit commands to Fre3nder's existing Core API.

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
