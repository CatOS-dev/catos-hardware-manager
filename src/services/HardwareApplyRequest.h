#pragma once

#include "model/HardwareTypes.h"

struct HardwareApplyRequest {
    QSet<QString> expectedChwd;
    QStringList remove;
    QStringList install;
    QStringList support;
};

namespace HardwareApplyRequestParser {
bool read(const QStringList &arguments, HardwareApplyRequest &request);
bool validate(const HardwareApplyRequest &request, const HardwareSnapshot &snapshot);
}
