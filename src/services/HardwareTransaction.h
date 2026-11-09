#pragma once

#include "model/HardwareTypes.h"
#include <functional>

namespace HardwareTransaction {
// snapper is installed and has at least one configuration.
bool snapshotsAvailable();
using Runner = std::function<bool(const QString &, const QStringList &)>;
using Reader = std::function<HardwareSnapshot()>;
using LineSink = std::function<void(const QString &)>;
using ErrorSink = std::function<void(const QString &)>;
bool apply(const HardwareSnapshot &before, const QStringList &remove,
           const QStringList &install, const QStringList &support,
           const Runner &run, const Reader &read, const LineSink &log = {},
           const ErrorSink &error = {});
}
