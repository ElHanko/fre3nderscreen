# Project purpose

This repository is a maintained fork of Guppy Screen.

The immediate goal is to evaluate the original Guppy Screen codebase and later
community maintenance work, then build a maintainable open touchscreen stack for
Klipper/Moonraker systems.

The Ender-3 V3 KE is an explicit target platform. Support for other Guppy Screen
targets should not be removed without a documented reason.

# Upstreams

Keep these source lines distinct:

- `upstream`: original `ballaswag/guppyscreen`
- `probielodan`: later maintained `probielodan/guppyscreen`
- `origin`: this maintained fork

Do not merge another fork wholesale before its changes have been reviewed.

When comparing implementations, record exact commit SHAs.

# Change classification

Changes from other forks should be classified before adoption:

- `TAKE`: suitable to adopt essentially as-is
- `MODIFY`: useful but should be adapted before adoption
- `DROP`: not useful for this project
- `REVIEW`: insufficiently understood

Prefer small, reviewable commits over large unclassified merges.

# Ender-3 V3 KE target

Known reference characteristics relevant to the touchscreen work include:

- display resolution: 480x272
- Linux framebuffer interface
- touch input through Linux evdev
- MIPS host architecture
- Moonraker as the printer API

Do not hard-code device-specific network addresses, usernames, credentials, or
other local identifiers.

# Printer safety

Repository analysis and local builds do not authorize changes to a physical
printer.

Do not install, deploy, restart services, modify configuration, replace vendor
files, or otherwise change a printer unless the user explicitly authorizes that
specific action.

Installer behavior must be reviewed separately from application behavior.

In particular, do not assume that historical Guppy Screen installer actions are
appropriate for this fork.

# Installer policy

Long term, prefer installation that:

- changes only files required for Guppy Screen;
- avoids unrelated SSH configuration changes;
- avoids replacing unrelated system libraries where possible;
- does not disable vendor services unless explicitly requested;
- is reversible;
- records every modified file;
- supports dry-run or inspection where practical.

Separate generic Guppy Screen installation logic from device-specific integration.

# Licensing

This repository is GPL-3.0 licensed and retains the existing upstream license.

Do not relicense GPL-covered source as MIT or another incompatible license.

Preserve upstream copyright and license notices.

When incorporating code from another GPL-compatible fork, retain provenance and
record the source commit where practical.

# Git rules

Before committing:

1. run `git status --short`;
2. run `git diff --check`;
3. review the complete diff;
4. verify that generated binaries, build output, secrets, and local device data
   are not staged;
5. preserve third-party attribution and provenance.

Do not create commits unless explicitly requested.
