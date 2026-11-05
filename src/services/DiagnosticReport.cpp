#include "DiagnosticReport.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

QByteArray DiagnosticReport::create(const HardwareSnapshot &snapshot)
{
    QJsonArray devices;
    for (const auto &device : snapshot.devices) {
        // Allowlisted fields only: no serials, addresses, hostnames, names or raw logs.
        devices.append(QJsonObject{{QStringLiteral("category"), static_cast<int>(device.category)},
            {QStringLiteral("bus"), device.bus}, {QStringLiteral("vendorId"), device.vendorId},
            {QStringLiteral("productId"), device.productId}, {QStringLiteral("driver"), device.driver},
            {QStringLiteral("profiles"), QJsonArray::fromStringList(device.profileIds)}});
    }
    QJsonArray profiles;
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
        {QStringLiteral("devices"), devices}, {QStringLiteral("profiles"), profiles}}).toJson();
}
