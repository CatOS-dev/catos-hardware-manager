#pragma once

#include "model/HardwareTypes.h"

struct ReadOptions {
    bool scanners = true;
};

enum class MicrocodeBoot { Loaded, NotLoaded, Unknown };

class HardwareService {
public:
    // Whether the boot setup loads the early microcode image. Reports
    // NotLoaded only when the initramfs generator and every readable boot
    // entry are known and none of them loads it; anything else is Unknown.
    static MicrocodeBoot microcodeBoot(const QString &root, const QString &package);
    static HardwareSnapshot read(const QString &chwdProgram = QStringLiteral("/usr/bin/chwd"),
                                 ReadOptions options = {});
    static void attachSupportProfiles(HardwareSnapshot &snapshot, const QVector<HardwareProfile> &support);
    static void updateSupportState(HardwareProfile &profile, const HardwareSnapshot &snapshot);
};
