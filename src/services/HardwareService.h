#pragma once

#include "model/HardwareTypes.h"

class HardwareService {
public:
    static HardwareSnapshot read(const QString &chwdProgram = QStringLiteral("/usr/bin/chwd"));
};
