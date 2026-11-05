#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

enum class DeviceCategory {
    Graphics, Network, Audio, Bluetooth, Camera, Printer, Scanner,
    Tablet, Firmware, Thunderbolt, Controller, MobileBroadband, SmartCard, Other
};

enum class ProfileSource { Chwd, Support };

struct HardwareMatchRule {
    QString capability;
    QString bus;
    QString vendor;
    QString device;
    QString driver;
    QString modaliasPrefix;
    QString dmiVendor;
    QString acpiHid;
};

struct HardwareDevice {
    QString id;
    DeviceCategory category = DeviceCategory::Other;
    QString name;
    QString vendor;
    QString bus;
    QString address;
    QString modalias;
    QString driver;
    QString vendorId;
    QString productId;
    QString dmiVendor;
    QString acpiHid;
    QStringList capabilities;
    QStringList profileIds;
    QString recommendedProfile;
};

struct HardwareProfile {
    QString id;
    QString name;
    QString chineseName;
    QString description;
    QString chineseDescription;
    DeviceCategory category = DeviceCategory::Other;
    ProfileSource source = ProfileSource::Support;
    QStringList packages;
    QStringList services;
    QStringList missingPackages;
    QStringList inactiveServices;
    QStringList disabledServices;
    QString conflictGroup;
    QVector<HardwareMatchRule> matchRules;
    QString chwdName;
    bool applicable = false;
    bool installed = false;
    bool ready = false;
    bool discovery = false;
    bool recommended = false;
    bool systemManaged = false;  // Support packages shared across software; do not remove automatically.
};

struct HardwareSnapshot {
    QVector<HardwareDevice> devices;
    QVector<HardwareProfile> profiles;
    QSet<QString> installedPackages;
    QSet<QString> enabledServices;
    QSet<QString> activeServices;
    QStringList warnings;
    bool chwdAvailable = false;
    QString error;

    bool isValid() const { return error.isEmpty(); }
    const HardwareProfile *profile(const QString &id) const
    {
        for (const auto &profile : profiles) {
            if (profile.id == id) return &profile;
        }
        return nullptr;
    }
};

QString categoryTitle(DeviceCategory category);
QString categoryIcon(DeviceCategory category);
