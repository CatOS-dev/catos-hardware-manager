#pragma once

#include "model/HardwareTypes.h"
#include <functional>

namespace HardwareTransaction {
using Runner = std::function<bool(const QString &, const QStringList &)>;
using Reader = std::function<HardwareSnapshot()>;
using LineSink = std::function<void(const QString &)>;
bool apply(const HardwareSnapshot &before, const QStringList &remove,
           const QStringList &install, const QStringList &support,
           const Runner &run, const Reader &read, const LineSink &log = {});
}
