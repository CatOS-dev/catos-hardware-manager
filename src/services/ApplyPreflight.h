#pragma once

#include "model/HardwareTypes.h"

// Facts shown in the confirmation dialog before anything runs as root.
// Everything here is advisory: the dialog explains, it does not block.
struct ApplyPreflight {
    // Installed kernels (by /usr/lib/modules/<version>) without the
    // headers a DKMS driver needs to build against.
    QStringList kernelsWithoutHeaders;
    bool secureBoot = false;
    bool snapshots = false;
    bool updatesPending = false;
    qint64 downloadBytes = -1;  // -1: unknown
};

namespace Preflight {
// Packages that are actually going to be installed by the request.
QStringList packagesToInstall(const HardwareSnapshot &snapshot, const QStringList &installChwd,
                              const QStringList &support);
bool needsDkms(const HardwareSnapshot &snapshot, const QStringList &installChwd);
QStringList kernelsWithoutHeaders(const QString &root);
bool secureBootEnabled(const QString &root);
// Runs pacman/snapper; call off the GUI thread.
ApplyPreflight run(const HardwareSnapshot &snapshot, const QStringList &removeChwd,
                   const QStringList &installChwd, const QStringList &support);
}
