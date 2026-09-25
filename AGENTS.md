# Project scope

This repository develops Fre3nderScreen, the dedicated local touchscreen UI
for Fre3nder on the Creality Ender-3 V3 KE.

Fre3nderScreen is derived from Guppy Screen, but it is not intended to remain
a generic Guppy Screen downstream. Code, assets, compatibility paths, and
dependencies should remain only when they serve a concrete Fre3nderScreen
requirement.

The production platform, operating system, printer integration, deployment,
and system-level functionality belong to the Fre3nder project.

# Development principles

Use KISS.

Before proposing, implementing, or expanding any task, ask:

> Is this step necessary, or does it materially advance the current goal?

In particular:

- understand the existing architecture and cause before changing code;
- prefer the smallest meaningful change;
- do not introduce unnecessary frameworks, abstractions, helpers, or parallel
  implementations;
- do not generalize a one-off requirement without evidence that the abstraction
  is useful;
- prefer evidence-driven fixes for observed problems over speculative
  hardening;
- do not turn temporary diagnostics, migration helpers, or test scaffolding
  into projects of their own;
- distinguish between `REQUIRED`, `USEFUL`, and `INTERESTING` work and defer
  the latter two when they do not materially reduce risk or unblock progress;
- preserve existing functionality unless changing it is explicitly part of the
  task.

Risk reduction and validation effort must remain proportional to the actual
change.

# Architecture boundary

Fre3nderScreen is a UI application, not a second platform-management layer.

Fre3nderScreen may own UI-local behavior such as:

- display sleep;
- UI log level;
- emergency-stop confirmation;
- icon/display preferences such as inverted Z controls;
- its own application version;
- restarting Fre3nderScreen itself.

Platform functionality should be provided by Fre3nder through an appropriate
API or existing platform interface. Examples include:

- network and WiFi management;
- Klipper or firmware restart;
- system and device information;
- platform updates;
- host-level configuration;
- operating-system or service management.

Do not add new direct host/system hacks to Fre3nderScreen when the behavior
belongs to the Fre3nder platform.

Do not create a second implementation in Fre3nderScreen merely because the
platform API does not yet exist. Document the missing platform capability
instead.

# Production and hardware boundary

This repository does not own production deployment to the physical printer.

Do not perform any of the following without explicit operator authorization:

- deployment to the printer;
- hardware writes;
- firmware flashing;
- rebooting the printer;
- filesystem creation, repair, or formatting;
- partition or raw block-device writes;
- bootloader, kernel, or RootFS changes;
- destructive or persistent recovery operations.

When production integration or deployment is required, follow the applicable
rules and established workflows in the Fre3nder repository.

A successful simulator test or local build does not authorize deployment to
the physical printer.

# Build execution requires explicit operator authorization

Automated coding agents, including Codex, must not start builds on their own.

Builds are operator-controlled actions because they consume local compute time
and coding-agent usage.

A general request to implement, validate, test, continue, finish, or proceed is
not build authorization.

Before executing any build, stop and report:

- why the build is needed;
- the smallest suitable build scope;
- the exact command;
- the expected result or artifact.

Only execute the build after explicit authorization for that specific build.

A materially different or broader build requires new authorization.

This applies to all build-producing commands, including:

- `make build`;
- target builds;
- component builds;
- Docker-based build pipelines;
- simulator commands that rebuild the simulator.

In particular:

```sh
./dev/simulator/simulator.sh
```

uses a Docker build and therefore requires explicit build authorization.

# Simulator

The Docker/SDL simulator under:

```text
dev/simulator/
```

is the standard development path for visual and UI-functional testing.

Prefer simulator validation over repeatedly deploying Fre3nderScreen to the
physical printer.

When a simulator build has been explicitly authorized, the normal test command
is:

```sh
./dev/simulator/simulator.sh
```

Do not modify production code solely to make a simulator test easier when the
same test can be implemented in simulator-specific mocks or fixtures.

Keep simulator-only behavior inside the development/simulator scope wherever
practical.

The simulator is a development tool, not a second supported production
platform.

# Validation cadence

Use the smallest validation appropriate to the current change.

Agents may perform read-only inspection and non-build checks without build
authorization, including where appropriate:

- `git diff`;
- `git diff --check`;
- `git status`;
- source/reference searches;
- static inspection;
- targeted non-build scripts or checks.

Do not repeatedly rebuild the simulator after every small edit.

Group related changes and use a simulator build as a validation gate when the
implementation is ready for meaningful visual or runtime testing.

If an authorized build exposes a problem:

1. investigate and change the concrete issue;
2. use targeted non-build checks where possible;
3. stop and request authorization before rebuilding.

# UI architecture and style

Fre3nderScreen targets a logical 272x480 portrait UI.

The established Fre3nder visual language is:

- dark background;
- dark cards;
- thin borders;
- white primary text;
- cyan for normal interaction and accents;
- red only for dangerous/destructive actions;
- no gradients;
- no unnecessary shadows;
- real subpages use fullscreen layouts;
- small back button on the left;
- page title centered where applicable.

Main pages use the navigation structure:

- Home
- Control
- Files
- Settings
- More

Do not preserve obsolete Guppy layouts merely by recoloring them when a simpler
native Fre3nder structure is appropriate.

Visual consistency is useful, but do not create a large UI framework or generic
component system solely to eliminate small amounts of repeated LVGL code.

# Legacy and cleanup discipline

Fre3nderScreen contains inherited Guppy Screen code and naming.

Do not assume that code is obsolete merely because it contains `guppy`,
`GUPPY`, an old class name, or an old asset name.

Before removing code:

1. find all source consumers;
2. inspect construction and registration paths;
3. inspect build-system inclusion;
4. consider indirect callbacks and runtime registration;
5. consider dynamically referenced assets or names;
6. determine whether the code is actually reachable.

Classify legacy findings as appropriate:

- active Fre3nderScreen functionality;
- internal compatibility/protocol name;
- visible legacy branding;
- dead code;
- unclear.

If the evidence is insufficient, mark the result `unclear` rather than guessing.

Do not perform global search-and-replace operations on Guppy names.

Names such as macros, JSON paths, RPC values, configuration keys, and shell
commands may form compatibility interfaces with Fre3nder, Klipper, Moonraker,
or existing configuration.

Before renaming such an interface, identify both sides of the interface and
change them together only when that migration is explicitly in scope.

Prefer small cleanup commits with one coherent purpose, for example:

- remove proven dead helper classes;
- remove proven unused assets;
- remove an obsolete compatibility path;
- rename one established internal interface.

Do not mix broad cleanup, behavioral changes, and unrelated UI redesign in one
change unless they cannot reasonably be separated.

# Guppy Screen provenance

Fre3nderScreen is derived from Guppy Screen.

Preserve upstream history, licensing information, copyright notices, and
third-party provenance where required.

Removing unused Guppy-specific functionality does not imply that historical
provenance or applicable license information may be removed.

Do not remove or rewrite licensing/provenance material merely as part of a
branding cleanup.

When importing or modifying third-party material, identify:

- its source;
- version or commit where applicable;
- applicable license;
- whether redistribution is permitted.

# Assets

Before deleting an asset, establish that it is not referenced by:

- C/C++ source;
- headers;
- build scripts;
- generated asset declarations;
- simulator code;
- runtime/dynamic path construction.

Do not treat a failed simple text search as sufficient evidence when the asset
may be selected indirectly or generated into another symbol name.

# Device-specific information

Do not commit information identifying one printer, computer, user, or network.

This includes:

- IP or MAC addresses;
- serial numbers or unique device identifiers;
- SSH fingerprints or local SSH aliases;
- personal usernames;
- personal home paths;
- filesystem or partition UUIDs;
- local storage usage;
- machine-specific installed software;
- local experiments or private helper scripts;
- private network details.

General source and documentation must use placeholders where necessary, such as:

- `<project-root>`
- `<printer-host>`
- `<printer-ip>`
- `<local-user>`

# Secrets

Never store or commit:

- passwords;
- tokens;
- API keys;
- private SSH keys;
- credentials;
- authentication cookies;
- session secrets.

# Local paths

Do not commit personal workstation paths such as:

```text
/home/<user>/...
```

Use repository-relative paths or placeholders such as:

```text
<project-root>
```

Technical paths that are genuinely part of the Fre3nder runtime interface may
be documented when necessary.

# Git rules

Do not discard, reset, overwrite, or revert existing user changes unless the
operator explicitly requests it.

Do not create a commit unless the operator explicitly authorizes the commit.

Commit authorization applies only to the requested commit or clearly stated
commit set.

Do not push unless the operator separately and explicitly authorizes the push.

A commit request is not push authorization.

Before every commit, at minimum:

1. run `git status --short`;
2. run `git diff --check`;
3. review the complete intended diff;
4. review the staged diff;
5. run `git diff --cached --check`;
6. verify that only intended files are staged;
7. scan for secrets and device-specific information;
8. verify provenance/licensing for new or modified third-party material.

Prefer focused commits whose message describes one logical change.

# Command discipline

Commands shown to the operator must be safe to paste into an interactive shell.

Do not place a top-level:

```sh
exit
```

or:

```sh
return
```

in a command block intended for interactive execution.

Do not use top-level:

```sh
set -e
```

or:

```sh
set -euo pipefail
```

in interactive command blocks where a failed diagnostic command could terminate
the operator's shell.

Such constructs are acceptable inside an intentionally created script where
their scope is limited to that script.

Markdown code fences must use normal language identifiers, for example:

```sh
echo "example"
```

Do not add tool-specific metadata to Markdown fences.

# Documentation rules

Document demonstrated behavior.

Clearly distinguish:

- behavior observed in Fre3nderScreen;
- behavior inherited from Guppy Screen;
- behavior provided by Fre3nder;
- assumptions or hypotheses.

Do not describe simulator-only behavior as hardware-qualified behavior.

Do not describe a change as validated on the physical printer when it was only
compiled or tested in the simulator.

# Project goal

Fre3nderScreen should provide the maintainable open local touchscreen UI for
Fre3nder.

The project should favor:

- simple native Fre3nder UI structures;
- maintainability;
- clear ownership boundaries;
- open implementations;
- removal of genuinely unused inherited scope;
- reproducible simulator testing.

The project does not need to preserve Guppy Screen's generic multi-printer,
multi-platform, packaging, or compatibility scope unless Fre3nder has a concrete
need for it.

Production packaging and platform integration belong to Fre3nder.
