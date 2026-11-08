#include "DiagnosticReport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QFile>
#include <QSet>
#include <QRegularExpression>
#include "services/HardwareService.h"

QByteArray DiagnosticReport::create(const HardwareSnapshot &snapshot)
{
    QJsonArray devices;
    for (const auto &device : snapshot.devices) {
        // Allowlisted fields only: no serials, addresses, hostnames, names or raw logs.
        devices.append(QJsonObject{{QStringLiteral("category"), categoryTitle(device.category)},
            {QStringLiteral("bus"), device.bus}, {QStringLiteral("vendorId"), device.vendorId},
            {QStringLiteral("productId"), device.productId}, {QStringLiteral("driver"), device.driver},
            {QStringLiteral("profiles"), QJsonArray::fromStringList(device.profileIds)}});
    }
    QJsonArray profiles;
    QJsonArray warningSummary;
    for (const auto &warning : snapshot.warnings) {
        QString summary;
        if (warning.contains(QStringLiteral("Driver query unavailable"), Qt::CaseInsensitive))
            summary = QStringLiteral("driver-query-unavailable");
        else if (warning.contains(QStringLiteral("updates are available"), Qt::CaseInsensitive))
            summary = QStringLiteral("system-updates-available");
        else if (warning.contains(QStringLiteral("package database"), Qt::CaseInsensitive))
            summary = QStringLiteral("package-database-stale");
        else if (warning.contains(QStringLiteral("package dependency"), Qt::CaseInsensitive))
            summary = QStringLiteral("package-satisfiability-unavailable");
        else if (warning.contains(QStringLiteral("service"), Qt::CaseInsensitive))
            summary = QStringLiteral("service-query-warning");
        else
            summary = QStringLiteral("hardware-query-warning");
        if (!warningSummary.contains(summary)) warningSummary.append(summary);
    }
    QJsonArray kernelParameters;
    QFile cmdline(QStringLiteral("/proc/cmdline"));
    if (cmdline.open(QIODevice::ReadOnly)) {
        const QSet<QString> allowedPrefixes{QStringLiteral("nomodeset"), QStringLiteral("i915."), QStringLiteral("amdgpu."), QStringLiteral("radeon."), QStringLiteral("nouveau."), QStringLiteral("nvidia."), QStringLiteral("vfio-pci.ids")};
        for (const auto &part : QString::fromUtf8(cmdline.readAll()).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
            for (const auto &prefix : allowedPrefixes) {
                if (part == prefix || part.startsWith(prefix + QLatin1Char('='))) {
                    kernelParameters.append(part);
                    break;
                }
            }
        }
    }
    for (const auto &profile : snapshot.profiles) {
        profiles.append(QJsonObject{{QStringLiteral("id"), profile.id},
            {QStringLiteral("applicable"), profile.applicable}, {QStringLiteral("installed"), profile.installed},
            {QStringLiteral("ready"), profile.source == ProfileSource::Chwd ? profile.installed : profile.ready},
            {QStringLiteral("missingPackages"), QJsonArray::fromStringList(profile.missingPackages)},
            {QStringLiteral("disabledServices"), QJsonArray::fromStringList(profile.disabledServices)},
            {QStringLiteral("inactiveServices"), QJsonArray::fromStringList(profile.inactiveServices)}});
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("schema"), 1},
        {QStringLiteral("kernel"), QSysInfo::kernelVersion()},
        {QStringLiteral("chwdAvailable"), snapshot.chwdAvailable},
        {QStringLiteral("queryFailed"), !snapshot.isValid()},
        {QStringLiteral("warningCount"), snapshot.warnings.size()},
        {QStringLiteral("warningSummary"), warningSummary},
        {QStringLiteral("kernelParameters"), kernelParameters},
        {QStringLiteral("devices"), devices}, {QStringLiteral("profiles"), profiles}}).toJson();
}
