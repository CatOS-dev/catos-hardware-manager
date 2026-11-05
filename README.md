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
smart-card readers, mobile broadband and system firmware.

## Profile rules

Support Profiles are YAML documents installed under
/usr/share/catos-hardware-manager/profiles. They declare packages,
optional system services and a match.any array. Each rule in any is a
conjunction of supplied conditions; rules themselves are alternatives:

- capability: detected from device subsystem, class, interface or backend
- bus: real device bus (PCI, USB, ACPI or a supported scanner backend)
- vendor, device: numerical hardware IDs
- driver: current bound kernel driver
- modalias_prefix: Linux kernel alias prefix
- dmi_vendor: actual system DMI vendor
- acpi_hid: ACPI hardware ID

Rules are loaded by the app and privileged executor. The executor validates
selected IDs against the installed catalog and actual hardware; the GUI never
supplies raw package commands.

## Transactions

A checkbox only stages a change. The user confirms all changes before a
single pkexec authorization. CHWD profiles can be installed and removed;
system support Profiles install packages and enable declared system services.
Those packages may be shared, so unchecking an already-installed support
Profile does not silently remove them.

Support packages are installed in one pacman invocation. CHWD operations,
pacman and service changes still run sequentially, not as an atomic rollback-
capable filesystem transaction. Partial completion is reported and actual
system state is refreshed.

## Build

Requires Qt 6 Widgets/SVG/Concurrent, libudev, yaml-cpp, CMake and Ninja.
An installed CHWD with JSON support is used for CHWD-only profiles. Other
device discovery continues to work if CHWD is unavailable.

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    DESTDIR=/tmp/catos-hardware-stage cmake --install build --prefix /usr

Installed GUI: /usr/bin/catos-hardware-manager
Privileged executor: /usr/lib/catos/catos-hardware-apply

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
serials, addresses and raw logs are excluded. Device test/settings shortcuts
appear when their corresponding desktop tools are installed.

Driver switches download declared packages, dependencies and recovery packages
before removal. This catches download/dependency failures, but cannot simulate
CHWD hooks, conditional packages or guarantee conflict-free installation. When
a switch fails after removal and no replacement is registered, the executor
attempts to reinstall removed configurations and verifies their recorded state.
It does not claim atomic rollback or restore partially executed hook effects.
The application shows completed, failed and unexecuted steps, saves the last
operation locally, and refreshes actual state so retry targets remaining work.

## Firmware

The firmware page uses the installed `fwupdmgr` client and its authorization
mechanism. Install firmware support, check for updates to refresh metadata and
list real devices, then review available versions and confirm a device update.
The client retains its device safety checks. Automatic history upload and
remote-enablement prompts are disabled. Remotes must already be configured.
Some devices stage updates for a restart; the application does not reboot the
machine automatically. Recheck the version after the required restart.
Device-specific interactive requests that require a terminal can fail and are
shown in the operation details. No firmware is flashed by the automated tests.

## Verification

    cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
    cmake --build build
    ctest --test-dir build --output-on-failure

Regression tests cover stopped services, disconnected devices, scanner discovery
bootstrap, download failure before driver removal, recovery after failed driver
installation, pending selection refresh and diagnostic report privacy. Firmware
page tests use a separate fake client process to exercise asynchronous JSON
responses, absent updates and invalid replies without changing the host.
