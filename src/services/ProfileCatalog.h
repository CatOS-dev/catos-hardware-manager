#pragma once

#include "model/HardwareTypes.h"

class ProfileCatalog {
public:
    static QString directoryForApplication();
    static QString installedDirectory();
    static QVector<HardwareProfile> load(const QString &directory, QString *error = nullptr);
    static bool matches(const HardwareProfile &profile, const HardwareDevice &device);
};
