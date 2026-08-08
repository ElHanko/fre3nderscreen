# Fork plan

## Purpose

This fork exists to continue evaluating and maintaining Guppy Screen as an open
touch UI for Klipper/Moonraker systems.

The Ender-3 V3 KE is an explicit target, including its 480x272 display and MIPS
host platform.

The initial task is not to select one existing fork blindly. Instead, the
original project and later maintenance work will be compared and changes will be
adopted selectively.

## Source lines

### A - Original Guppy Screen

Repository:

`ballaswag/guppyscreen`

Reference commit:

`07409cb031bbbfc57cd7817ba295e5385e3d5565`

This is the current `upstream/main` reference and the common ancestor used for
the initial comparison.

### B - probielodan continuation

Repository:

`probielodan/guppyscreen`

Reference commit at initial inventory:

`fac48354a2a1d115bfda14cd59972f383fbba390`

At the time of the initial comparison, this branch is eight commits ahead of the
original reference and has no commits on the original side that are absent from
it.

### C - This fork

Repository:

`ElHanko/guppyscreen`

Development branch:

`integration`

The integration branch initially starts from the original Guppy Screen reference
commit, not from the later continuation.

## Initial relationship

The initial Git comparison is:

```text
upstream/main      07409cb
       |
       +-- 8 commits --> probielodan/main  fac4835

origin/main        07409cb
origin/integration 07409cb
```

The common ancestor of `upstream/main` and `probielodan/main` is exactly the
current original upstream HEAD.

This means the later continuation can be evaluated as a clean eight-commit
delta from the original project.

## Evaluation strategy

Each later change should be classified as:

- `TAKE`
- `MODIFY`
- `DROP`
- `REVIEW`

Evaluation should consider:

- correctness and bug fixes;
- compatibility with current Klipper and Moonraker;
- Ender-3 V3 KE compatibility;
- 480x272 UI behavior;
- MIPS build support;
- framebuffer and evdev support;
- installer invasiveness;
- maintainability;
- dependencies and bundled third-party components;
- portability to other supported Guppy Screen devices.

## Architecture direction

The preferred long-term structure separates the touchscreen application from
device-specific installation and vendor integration.

Conceptually:

```text
Guppy Screen application
        |
        +-- Moonraker API
        |
        +-- display backend
        |      `-- Linux framebuffer / platform backend
        |
        +-- input backend
               `-- Linux evdev / platform backend

Device integration
        |
        +-- Ender-3 V3 KE
        +-- Creality K1 family
        +-- generic Linux / Raspberry Pi
```

The application should not require unrelated modifications to SSH, printer
firmware, or system configuration merely to render the UI.

## First analysis milestone

Before selecting the development base:

1. inspect all eight commits in `probielodan/main`;
2. produce a file-level and functional delta;
3. classify each change;
4. identify installer-only changes separately from application changes;
5. identify build-system changes and current MIPS support;
6. decide whether to adopt individual commits, reconstruct selected changes, or
   move the integration branch to the maintained continuation.

No printer deployment is part of this milestone.

## License

This repository remains licensed under GNU GPL version 3 in accordance with the
existing upstream project license.

Source copied or adapted from other Guppy Screen forks must retain appropriate
GPL licensing and provenance.
