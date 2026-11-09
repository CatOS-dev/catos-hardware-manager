#include "TransactionProtocol.h"

#include "widgets/HardwareUi.h"

#include <QCoreApplication>

QString TransactionProtocol::actionName(const QString &action)
{
    if (action == QLatin1String("preflight")) return QCoreApplication::translate("TransactionProtocol", "Download packages");
    if (action == QLatin1String("snapshot")) return QCoreApplication::translate("TransactionProtocol", "Create snapshot");
    if (action == QLatin1String("remove")) return QCoreApplication::translate("TransactionProtocol", "Remove driver");
    if (action == QLatin1String("install")) return QCoreApplication::translate("TransactionProtocol", "Install driver");
    if (action == QLatin1String("packages")) return QCoreApplication::translate("TransactionProtocol", "Install support components");
    if (action == QLatin1String("service")) return QCoreApplication::translate("TransactionProtocol", "Start service");
    if (action == QLatin1String("disable-service")) return QCoreApplication::translate("TransactionProtocol", "Stop service");
    return action;
}

QString TransactionProtocol::targetName(const QString &action, const QString &target, const HardwareSnapshot &snapshot)
{
    if (action == QLatin1String("remove") || action == QLatin1String("install")) {
        if (const auto *profile = snapshot.profile(QStringLiteral("chwd:") + target))
            return HardwareUi::translatedName(*profile);
    }
    if (const auto *profile = snapshot.profile(target)) return HardwareUi::translatedName(*profile);
    if (action == QLatin1String("snapshot")) return {};
    return target;
}

QString TransactionProtocol::errorMessage(const QStringList &fields, const HardwareSnapshot &snapshot)
{
    if (fields.size() < 3 || fields[0] != QLatin1String("ERROR")) return {};
    const QString &kind = fields[1];
    const auto name = [&snapshot](const QString &target) {
        if (const auto *profile = snapshot.profile(target)) return HardwareUi::translatedName(*profile);
        if (const auto *profile = snapshot.profile(QStringLiteral("chwd:") + target))
            return HardwareUi::translatedName(*profile);
        return target;
    };
    if (kind == QLatin1String("package-manager"))
        return QCoreApplication::translate("TransactionProtocol", "Another package manager is running. Close it and try again.");
    if (kind == QLatin1String("request"))
        return QCoreApplication::translate("TransactionProtocol", "The request was rejected. Refresh and review the changes again.");
    if (kind == QLatin1String("state") && fields[2] == QLatin1String("changed"))
        return QCoreApplication::translate("TransactionProtocol", "The hardware state changed since it was shown. Refresh and review the changes again.");
    if (kind == QLatin1String("state") && fields[2] == QLatin1String("final"))
        return QCoreApplication::translate("TransactionProtocol", "The changes ran, but the final state could not be read. Refresh to check the result.");
    if (kind == QLatin1String("state"))
        return QCoreApplication::translate("TransactionProtocol", "The hardware state could not be read. Refresh and try again.");
    if (kind == QLatin1String("verify") && fields.size() >= 4)
        return fields[3] == QLatin1String("still-installed")
            ? QCoreApplication::translate("TransactionProtocol", "%1 is still installed after removal.").arg(name(fields[2]))
            : QCoreApplication::translate("TransactionProtocol", "%1 was not installed.").arg(name(fields[2]));
    if (kind == QLatin1String("support") && fields.size() >= 4)
        return fields[3] == QLatin1String("still-active")
            ? QCoreApplication::translate("TransactionProtocol", "The services of %1 are still running.").arg(name(fields[2]))
            : QCoreApplication::translate("TransactionProtocol", "%1 is not ready after installation.").arg(name(fields[2]));
    if (kind == QLatin1String("step") && fields.size() >= 4)
        return QCoreApplication::translate("TransactionProtocol", "\"%1\" failed for %2.")
            .arg(actionName(fields[2]), name(fields[3]));
    return {};
}

bool TransactionProtocol::interrupted(const QByteArray &log)
{
    const QList<QByteArray> lines = log.trimmed().split('\n');
    if (log.trimmed().isEmpty()) return false;
    const QByteArray last = lines.constLast().trimmed();
    return last != "DONE" && last != "FAILED";
}

bool TransactionProtocol::authorizationCancelled(int exitCode, bool sawPlan)
{
    return !sawPlan && (exitCode == 126 || exitCode == 127);
}
