#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

// What a row shows, computed in one place. Rows call these both when they
// are created and when they are updated in place, so the two paths cannot
// drift apart.

enum class StatusLevel { Neutral, Ok, Pending, Attention, Error };

struct StatusText {
    QString text;
    StatusLevel level = StatusLevel::Neutral;
    bool operator==(const StatusText &other) const { return text == other.text && level == other.level; }
};

namespace Presentation {
// Whether the device only works with a kernel driver bound to it. Readers
// handled in userspace (fingerprint, smart card, scanner) and synthetic
// system entries never have one.
bool driverExpected(const HardwareDevice &device);
bool hasPendingChange(const HardwareProfile &profile, const ProfileSelection &selection);

StatusText deviceStatus(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                        const ProfileSelection &selection);
QString deviceSummary(const HardwareDevice &device, const HardwareSnapshot &snapshot);
QString deviceIdentifiers(const HardwareDevice &device);

QString profileName(const HardwareProfile &profile, bool recommended);
StatusText profileState(const HardwareProfile &profile, const ProfileSelection &selection);
bool profileToggleable(const HardwareProfile &profile);
}
