# CatOS Hardware Manager

Qt Widgets hardware manager following CatOS Hello's sidebar and palette. The
application has three pages: hardware devices (expandable rows), installed
profiles and the profile catalog.

The sidebar, window icon and desktop launcher use the original SVG shipped by
catos-branding (/usr/share/pixmaps/catos-logo.svg and the catos-logo icon).

## Source structure

- src/app: main window, sidebar, state coordination and launch
- src/pages: three independent pages
- src/widgets: reusable device/Profile rows and theme-aware graphics
- src/model: hardware state and staged Profile selection
- src/services: CHWD query, independent udev/SANE discovery and YAML matching
- src/dialogs: change preview and progress after one pkexec authorization
- src/privileged: validated, non-shell privileged transaction executor
- data/profiles: system support Profiles independent of CHWD

The hardware service merges PCI/USB CHWD profiles with physical sysfs/udev
devices. CHWD is optional for discovering other hardware: Bluetooth, audio,
cameras, printers, scanners detected by SANE, tablets, gamepads, Thunderbolt,
smart-card readers, mobile broadband.

## Profile rules

Support Profiles are YAML documents installed under
/usr/share/catos-hardware-manager/profiles. They declare packages,
optional system services and a match.any array. Each rule in any is a
conjunction of supplied conditions; rules themselves are alternatives:

- capability: detected from device subsystem, class, interface or backend
- bus: real device bus (PCI, USB, ACPI or a supported scanner backend)
- vendor, device: numerical hardware IDs
- driver: current bound kernel driver
- module: kernel module the device's modalias resolves to (works even when
  the driver failed to bind, e.g. because firmware is missing)
- modalias_prefix: Linux kernel alias prefix
- dmi_vendor: actual system DMI vendor
- acpi_hid: ACPI hardware ID

Rules are loaded by the app and privileged executor. The executor validates
selected IDs against the installed catalog and actual hardware; the GUI never
supplies raw package commands.

## Device support notes

Fingerprint support installs fprintd and libfprint when a supported reader is detected.
It does not modify PAM; enabling fingerprint login or sudo remains a desktop or user choice.
Wireless and graphics firmware profiles install Arch `linux-firmware-*` files only; they do not flash device firmware.

## Transactions

A checkbox only stages a change. The user confirms all changes before a
single pkexec authorization. CHWD profiles can be installed and removed;
system support Profiles install packages and enable declared system services.
Those packages may be shared, so unchecking an already-installed support
Profile does not silently remove them. A ready support Profile with services
offers an explicit "Stop services" action instead, which runs
`systemctl disable --now` for its services and keeps the packages.

The confirmation lists driver removals first and notes, without blocking,
installed kernels that lack headers for a DKMS build, Secure Boot, pending
system updates, whether snapper takes a snapshot and the download size.

Support packages are installed in one pacman invocation. CHWD operations,
pacman and service changes still run sequentially, not as an atomic rollback-
capable filesystem transaction. Partial completion is reported and actual
system state is refreshed.

## Build

Requires Qt 6 Widgets/SVG/Concurrent/DBus, libudev, yaml-cpp, CMake and Ninja.
Device discovery tests additionally use umockdev (optional).
An installed CHWD with JSON support is used for CHWD-only profiles. Other
device discovery continues to work if CHWD is unavailable.

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    DESTDIR=/tmp/catos-hardware-stage cmake --install build --prefix /usr

Installed GUI: /usr/bin/catos-hardware-manager
Privileged executor: /usr/lib/catos/catos-hardware-apply

## Command line

For installers and first-boot setup:

    catos-hardware-manager --list [--json]
    catos-hardware-manager --apply-recommended [--yes]

`--apply-recommended` applies what "Select recommended" stages and never
removes an installed driver. Without `--yes` it asks on a terminal and refuses
otherwise. As root it runs the executor directly, otherwise through pkexec.
Exit codes: 0 done or nothing to do, 1 failed or declined, 2 usage error,
3 authorization cancelled.

## Translations

The UI language follows the desktop language list in order (`LANGUAGE`
first). Simplified Chinese is maintained by hand in
`translations/*_zh_CN.ts` and `name_zh`/`description_zh` profile fields.
Traditional Chinese is generated from it with OpenCC:

    lupdate -no-obsolete -locations none src -ts translations/catos-hardware-manager_zh_CN.ts
    tools/sync-traditional-chinese.py

Put corrections for the Traditional text into the script's replacement table.

## Diagnostics and device support

Installed support is independent of whether an external device is connected.
Service enablement and active state are checked separately; stopped services
can be selected for repair without reinstalling their packages. Socket-backed
profiles check the socket, not an intentionally idle daemon. Discovery-only
scanner setup is available before SANE or its discovery backend is installed.
Recommended selection includes only support profiles explicitly marked
`recommended: true`; optional utilities remain manual choices.

udev events trigger a debounced refresh that preserves pending choices.
Diagnostic reports have a local preview and export only allowlisted hardware
IDs, drivers, profile status, missing packages and service status. Device names,
serials, addresses and raw logs are excluded. Device test/settings buttons
open KDE System Settings modules or KDE applications and only appear when the
module or application is installed.

Driver switches download declared packages, dependencies and recovery packages
before removal. This catches download/dependency failures, but cannot simulate
CHWD hooks, conditional packages or guarantee conflict-free installation. When
a switch fails after removal and no replacement is registered, the executor
attempts to reinstall removed configurations and verifies their recorded state.
It does not claim atomic rollback or restore partially executed hook effects.
The application shows completed, failed and unexecuted steps, saves the last
operation locally, and refreshes actual state so retry targets remaining work.

## Verification

    cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
    cmake --build build
    ctest --test-dir build --output-on-failure

- hardware-regression: selection, request validation, transactions with fake
  runners, microcode boot detection, preflight checks, diagnostic privacy.
- device-discovery: libudev discovery against umockdev sysfs trees (GPU behind
  a bridge, USB Bluetooth, unbound smart card, vendor-specific reader, camera,
  hybrid graphics). Runs through umockdev-wrapper.
- ui: offscreen widget tests for row presentation, in-place updates, focus
  restoration, palette changes, the confirmation dialog and protocol parsing.
