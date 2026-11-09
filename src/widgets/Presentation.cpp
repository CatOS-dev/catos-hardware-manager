#include "Presentation.h"

#include "HardwareUi.h"

#include <QCoreApplication>

namespace {
QString attention(const QString &reason)
{
    return QCoreApplication::translate("Presentation", "Needs attention: %1").arg(reason);
}
}

bool Presentation::driverExpected(const HardwareDevice &device)
{
    // Only devices found through udev carry a modalias; CHWD-only and
    // synthetic entries cannot report a driver either way.
    if (device.modalias.isEmpty() || device.id.startsWith(QLatin1String("system:"))) return false;
    switch (device.category) {
    case DeviceCategory::Graphics:
    case DeviceCategory::Network:
    case DeviceCategory::Audio:
    case DeviceCategory::Bluetooth:
    case DeviceCategory::Camera:
    case DeviceCategory::Tablet:
    case DeviceCategory::Controller:
    case DeviceCategory::MobileBroadband:
    case DeviceCategory::Thunderbolt:
        return true;
    default:
        return false;
    }
}

bool Presentation::hasPendingChange(const HardwareProfile &profile, const ProfileSelection &selection)
{
    const bool selected = selection.selected(profile.id);
    if (profile.source == ProfileSource::Chwd) return selected != profile.installed;
    return profile.applicable && !profile.ready && selected;
}

StatusText Presentation::deviceStatus(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                                      const ProfileSelection &selection)
{
    bool pending = false;
    bool missingRecommended = false;
    bool serviceIssue = false;
    bool recommendedDriverMissing = false;
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (!profile) continue;
        pending |= hasPendingChange(*profile, selection);
        if (profile->source != ProfileSource::Support || !profile->applicable
            || !profile->conflictingPackages.isEmpty())
            continue;
        if (profile->recommended && !profile->missingPackages.isEmpty()) missingRecommended = true;
        else if (profile->missingPackages.isEmpty() && !profile->ready) serviceIssue = true;
    }
    if (const auto *recommended = snapshot.profile(device.recommendedProfile);
        recommended && recommended->source == ProfileSource::Chwd && !recommended->installed) {
        bool groupInstalled = false;
        for (const auto &profile : snapshot.profiles) {
            groupInstalled |= profile.source == ProfileSource::Chwd && profile.installed
                && !recommended->conflictGroup.isEmpty() && profile.conflictGroup == recommended->conflictGroup;
        }
        recommendedDriverMissing = !groupInstalled;
    }

    if (pending) return {QCoreApplication::translate("Presentation", "Changes pending"), StatusLevel::Pending};
    if (device.capabilities.contains(QStringLiteral("fingerprint-unsupported")))
        return {attention(QCoreApplication::translate("Presentation", "no open-source driver supports this reader yet")), StatusLevel::Attention};
    if (driverExpected(device) && device.driver.isEmpty())
        return {QCoreApplication::translate("Presentation", "Driver not found"), StatusLevel::Error};
    if (recommendedDriverMissing)
        return {attention(QCoreApplication::translate("Presentation", "recommended driver is not installed")), StatusLevel::Attention};
    if (missingRecommended)
        return {attention(QCoreApplication::translate("Presentation", "recommended support components are missing")), StatusLevel::Attention};
    if (serviceIssue)
        return {attention(QCoreApplication::translate("Presentation", "a support service is not running")), StatusLevel::Attention};
    return {QCoreApplication::translate("Presentation", "Normal"), StatusLevel::Ok};
}

QString Presentation::deviceSummary(const HardwareDevice &device, const HardwareSnapshot &snapshot)
{
    QStringList parts;
    if (!device.driver.isEmpty()) parts.append(QCoreApplication::translate("Presentation", "Driver: %1").arg(device.driver));
    QStringList installed;
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (profile && profile->source == ProfileSource::Chwd && profile->installed)
            installed.append(HardwareUi::translatedName(*profile));
    }
    if (!installed.isEmpty()) parts.append(QCoreApplication::translate("Presentation", "CHWD: %1").arg(installed.join(QStringLiteral(", "))));
    return parts.join(QStringLiteral(" · "));
}

QString Presentation::deviceIdentifiers(const HardwareDevice &device)
{
    // Synthetic entries (system:cpu, system:graphics, ...) have no bus address.
    if (device.id.startsWith(QLatin1String("system:"))) return device.vendor;
    QStringList identifiers;
    if (!device.address.isEmpty()) identifiers.append(device.bus.toUpper() + QLatin1Char(' ') + device.address);
    if (!device.vendor.isEmpty()) identifiers.append(device.vendor);
    return identifiers.join(QStringLiteral(" · "));
}

QString Presentation::profileName(const HardwareProfile &profile, bool recommended)
{
    QString name = HardwareUi::translatedName(profile);
    if (recommended) name += QCoreApplication::translate("Presentation", "  · Recommended");
    return name;
}

StatusText Presentation::profileState(const HardwareProfile &profile, const ProfileSelection &selection)
{
    const bool selected = selection.selected(profile.id);
    if (profile.source == ProfileSource::Chwd) {
        if (profile.installed && !selected) return {QCoreApplication::translate("Presentation", "Will be removed"), StatusLevel::Pending};
        if (!profile.installed && selected) return {QCoreApplication::translate("Presentation", "Will be installed"), StatusLevel::Pending};
        if (profile.installed) return {QCoreApplication::translate("Presentation", "Installed"), StatusLevel::Ok};
        if (!profile.applicable) return {QCoreApplication::translate("Presentation", "Not applicable"), StatusLevel::Neutral};
        return {QCoreApplication::translate("Presentation", "Not installed"), StatusLevel::Neutral};
    }
    if (!profile.applicable && profile.installed) return {QCoreApplication::translate("Presentation", "Installed; device disconnected"), StatusLevel::Neutral};
    if (!profile.applicable) return {QCoreApplication::translate("Presentation", "Not applicable"), StatusLevel::Neutral};
    if (!profile.conflictingPackages.isEmpty())
        return {QCoreApplication::translate("Presentation", "Managed by %1").arg(profile.conflictingPackages.join(QStringLiteral(", "))), StatusLevel::Neutral};
    if (profile.ready) return {QCoreApplication::translate("Presentation", "Ready"), StatusLevel::Ok};
    if (selected) return {QCoreApplication::translate("Presentation", "Will be installed"), StatusLevel::Pending};
    if (profile.missingPackages.isEmpty()) return {QCoreApplication::translate("Presentation", "Service not running"), StatusLevel::Attention};
    if (profile.recommended) return {QCoreApplication::translate("Presentation", "Missing components"), StatusLevel::Attention};
    return {QCoreApplication::translate("Presentation", "Not installed"), StatusLevel::Neutral};
}

bool Presentation::profileToggleable(const HardwareProfile &profile)
{
    // A disabled checkbox suggests a broken button: offer one only when the
    // profile can actually be staged for a change.
    return (profile.applicable || (profile.source == ProfileSource::Chwd && profile.installed))
        && profile.conflictingPackages.isEmpty()
        && !(profile.systemManaged && profile.ready);
}
