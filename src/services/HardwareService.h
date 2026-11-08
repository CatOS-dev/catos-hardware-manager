#pragma once

#include "model/HardwareTypes.h"

struct ReadOptions {
    bool scanners = true;
};

class HardwareService {
public:
    static HardwareSnapshot read(const QString &chwdProgram = QStringLiteral("/usr/bin/chwd"),
                                 ReadOptions options = {});
    static void attachSupportProfiles(HardwareSnapshot &snapshot, const QVector<HardwareProfile> &support);
    static void updateSupportState(HardwareProfile &profile, const HardwareSnapshot &snapshot);
};
