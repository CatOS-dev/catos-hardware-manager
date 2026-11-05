#pragma once
#include "model/HardwareTypes.h"

namespace DiagnosticReport {
QByteArray create(const HardwareSnapshot &snapshot);
}
