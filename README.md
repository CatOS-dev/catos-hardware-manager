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
