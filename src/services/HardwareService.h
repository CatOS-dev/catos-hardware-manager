#pragma once

#include "model/HardwareTypes.h"

class HardwareService {
public:
    static HardwareSnapshot read(const QString &chwdProgram = QStringLiteral("/usr/bin/chwd"));
    static void attachSupportProfiles(HardwareSnapshot &snapshot, const QVector<HardwareProfile> &support);
    static void updateSupportState(HardwareProfile &profile, const HardwareSnapshot &snapshot);
};
