#include "Presentation.h"

#include "HardwareUi.h"

#include <QCoreApplication>

namespace {
QString tr(const char *text)
{
    return QCoreApplication::translate("Presentation", text);
}

QString attention(const QString &reason)
{
    return tr("Needs attention: %1").arg(reason);
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

    if (pending) return {tr("Changes pending"), StatusLevel::Pending};
    if (device.capabilities.contains(QStringLiteral("fingerprint-unsupported")))
        return {attention(tr("no open-source driver supports this reader yet")), StatusLevel::Attention};
    if (driverExpected(device) && device.driver.isEmpty())
        return {tr("Driver not found"), StatusLevel::Error};
    if (recommendedDriverMissing)
        return {attention(tr("recommended driver is not installed")), StatusLevel::Attention};
    if (missingRecommended)
        return {attention(tr("recommended support components are missing")), StatusLevel::Attention};
    if (serviceIssue)
        return {attention(tr("a support service is not running")), StatusLevel::Attention};
    return {tr("Normal"), StatusLevel::Ok};
}

QString Presentation::deviceSummary(const HardwareDevice &device, const HardwareSnapshot &snapshot)
{
    QStringList parts;
    if (!device.driver.isEmpty()) parts.append(tr("Driver: %1").arg(device.driver));
    QStringList installed;
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (profile && profile->source == ProfileSource::Chwd && profile->installed)
            installed.append(HardwareUi::translatedName(*profile));
    }
    if (!installed.isEmpty()) parts.append(tr("CHWD: %1").arg(installed.join(QStringLiteral(", "))));
    return parts.join(QStringLiteral(" · "));
}

QString Presentation::deviceIdentifiers(const HardwareDevice &device)
{
    QStringList identifiers;
    if (!device.address.isEmpty()) identifiers.append(device.bus.toUpper() + QLatin1Char(' ') + device.address);
    if (!device.vendor.isEmpty()) identifiers.append(device.vendor);
    return identifiers.join(QStringLiteral(" · "));
}

QString Presentation::profileName(const HardwareProfile &profile, bool recommended)
{
    QString name = HardwareUi::translatedName(profile);
    if (recommended) name += tr("  · Recommended");
    return name;
}

StatusText Presentation::profileState(const HardwareProfile &profile, const ProfileSelection &selection)
{
    const bool selected = selection.selected(profile.id);
    if (profile.source == ProfileSource::Chwd) {
        if (profile.installed && !selected) return {tr("Will be removed"), StatusLevel::Pending};
        if (!profile.installed && selected) return {tr("Will be installed"), StatusLevel::Pending};
        if (profile.installed) return {tr("Installed"), StatusLevel::Ok};
        if (!profile.applicable) return {tr("Not applicable"), StatusLevel::Neutral};
        return {tr("Not installed"), StatusLevel::Neutral};
    }
    if (!profile.applicable && profile.installed) return {tr("Installed; device disconnected"), StatusLevel::Neutral};
    if (!profile.applicable) return {tr("Not applicable"), StatusLevel::Neutral};
    if (!profile.conflictingPackages.isEmpty())
        return {tr("Managed by %1").arg(profile.conflictingPackages.join(QStringLiteral(", "))), StatusLevel::Neutral};
    if (profile.ready) return {tr("Ready"), StatusLevel::Ok};
    if (selected) return {tr("Will be installed"), StatusLevel::Pending};
    if (profile.missingPackages.isEmpty()) return {tr("Service not running"), StatusLevel::Attention};
    if (profile.recommended) return {tr("Missing components"), StatusLevel::Attention};
    return {tr("Not installed"), StatusLevel::Neutral};
}

bool Presentation::profileToggleable(const HardwareProfile &profile)
{
    // A disabled checkbox suggests a broken button: offer one only when the
    // profile can actually be staged for a change.
    return (profile.applicable || (profile.source == ProfileSource::Chwd && profile.installed))
        && profile.conflictingPackages.isEmpty()
        && !(profile.systemManaged && profile.ready);
}
