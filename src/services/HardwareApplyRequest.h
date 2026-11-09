#pragma once

#include "model/HardwareTypes.h"

struct HardwareApplyRequest {
    QSet<QString> expectedChwd;
    QStringList remove;
    QStringList install;
    QStringList support;
    // Support profiles whose services are stopped and disabled. Packages
    // stay installed: other software may depend on them.
    QStringList disableServices;
};

namespace HardwareApplyRequestParser {
bool read(const QStringList &arguments, HardwareApplyRequest &request);
bool validate(const HardwareApplyRequest &request, const HardwareSnapshot &snapshot);
}
