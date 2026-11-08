#pragma once

#include "model/HardwareTypes.h"

#include <QSet>

class DeviceDiscovery {
public:
    // Read-only enumeration. Device capabilities are derived from kernel/udev
    // classifications, never inferred from an arbitrary device-name substring.
    static QVector<HardwareDevice> enumerate(bool scanners = true);
    static QVector<HardwareDevice> enumerate(bool scanners, const QSet<QString> &acpiHids);
};
