#pragma once

#include "model/HardwareTypes.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

// Reading the executor's line protocol (PLAN/STEP/RESULT/RECOVERY/ERROR/
// DONE). The executor only speaks English identifiers; everything shown to
// the user is translated here.
namespace TransactionProtocol {
QString actionName(const QString &action);
// A profile id or CHWD name becomes the translated profile name.
QString targetName(const QString &action, const QString &target, const HardwareSnapshot &snapshot);
// Translated explanation for an ERROR record, or an empty string.
QString errorMessage(const QStringList &fields, const HardwareSnapshot &snapshot);
// Whether a root transaction log ends without DONE or FAILED.
bool interrupted(const QByteArray &log);
// pkexec exits 126 when the dialog is dismissed and 127 when
// authorisation fails; without a PLAN the executor never ran.
bool authorizationCancelled(int exitCode, bool sawPlan);
}
