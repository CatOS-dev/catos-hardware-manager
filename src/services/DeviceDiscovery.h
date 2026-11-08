#pragma once

#include "model/HardwareTypes.h"

class DeviceDiscovery {
public:
    // Read-only enumeration. Device capabilities are derived from kernel/udev
    // classifications, never inferred from an arbitrary device-name substring.
    static QVector<HardwareDevice> enumerate(bool scanners = true);
};
