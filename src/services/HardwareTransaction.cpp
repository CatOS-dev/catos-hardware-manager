#include "HardwareTransaction.h"

#include <cstdio>
#include <QFileInfo>
#include <QProcess>
#include <QtGlobal>

namespace {
bool snapperConfigured()
{
    if (!QFileInfo::exists(QStringLiteral("/usr/bin/snapper"))) return false;
    QProcess process;
    process.start(QStringLiteral("/usr/bin/snapper"), {QStringLiteral("list-configs")});
    return process.waitForFinished(3000) && process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0 && !process.readAllStandardOutput().trimmed().isEmpty();
}
}

namespace HardwareTransaction {
bool apply(const HardwareSnapshot &before, const QStringList &remove,
           const QStringList &install, const QStringList &support,
           const Runner &run, const Reader &read, const LineSink &log, const ErrorSink &error)
{
    const auto emitError = [&error](const QString &message) {
        std::fprintf(stderr, "%s\n", message.toUtf8().constData());
        if (error) error(message);
    };
    const auto emitLine = [&log](const QString &line) {
        std::printf("%s\n", line.toUtf8().constData());
        std::fflush(stdout);
        if (log) log(line);
    };
    const bool changesDrivers = !remove.isEmpty() || !install.isEmpty();
    if (!before.isValid() || (changesDrivers && !before.chwdAvailable)) {
        emitError(QStringLiteral("Cannot query the initial hardware state. Refresh before applying changes."));
        return false;
    }
    struct Step { QString action; QString target; QString program; QStringList arguments; };
    QVector<Step> steps;
    QSet<QString> packageSet;
    QSet<QString> serviceSet;
    QSet<QString> downloadSet;
    for (const auto &id : support) {
        const auto *profile = before.profile(id);
        if (!profile) return false;
        for (const auto &package : profile->missingPackages) packageSet.insert(package);
        for (const auto &service : profile->services) {
            if (!before.enabledServices.contains(service) || !before.activeServices.contains(service))
                serviceSet.insert(service);
        }
    }
    downloadSet = packageSet;
    QStringList downloadProfiles = install;
    if (!install.isEmpty()) downloadProfiles.append(remove);
    for (const auto &name : downloadProfiles) {
        const auto *profile = before.profile(QStringLiteral("chwd:") + name);
        if (!profile) return false;
        for (const auto &package : profile->packages) downloadSet.insert(package);
    }
    QStringList downloads = downloadSet.values();
    downloads.sort();
    if (!downloads.isEmpty()) {
        QStringList arguments{QStringLiteral("-Sw"), QStringLiteral("--noconfirm"), QStringLiteral("--")};
        arguments.append(downloads);
        steps.append({QStringLiteral("preflight"), downloads.join(QStringLiteral(", ")),
                      QStringLiteral("/usr/bin/pacman"), arguments});
    }
    if (changesDrivers && !qEnvironmentVariableIsSet("CATOS_DISABLE_SNAPSHOTS")
        && snapperConfigured()) {
        const QString description = QStringLiteral("catos-hardware: ")
            + (remove + install).join(QStringLiteral(", "));
        steps.append({QStringLiteral("snapshot"), description, QStringLiteral("/usr/bin/snapper"),
                      {QStringLiteral("create"), QStringLiteral("--description"), description}});
    }
    for (const auto &name : remove)
        steps.append({QStringLiteral("remove"), name, QStringLiteral("/usr/bin/chwd"), {QStringLiteral("--remove"), name}});
    for (const auto &name : install)
        steps.append({QStringLiteral("install"), name, QStringLiteral("/usr/bin/chwd"), {QStringLiteral("--install"), name}});
    QStringList packages = packageSet.values();
    packages.sort();
    if (!packages.isEmpty()) {
        QStringList arguments{QStringLiteral("-S"), QStringLiteral("--needed"), QStringLiteral("--noconfirm"), QStringLiteral("--")};
        arguments.append(packages);
        steps.append({QStringLiteral("packages"), packages.join(QStringLiteral(", ")), QStringLiteral("/usr/bin/pacman"), arguments});
    }
    QStringList services = serviceSet.values();
    services.sort();
    for (const auto &name : services) {
        const QString action = before.enabledServices.contains(name) ? QStringLiteral("start") : QStringLiteral("enable");
        QStringList arguments{action};
        if (action == QLatin1String("enable")) arguments.append(QStringLiteral("--now"));
        arguments.append(name);
        steps.append({QStringLiteral("service"), name, QStringLiteral("/usr/bin/systemctl"), arguments});
    }
    // Emit the complete plan before making changes, so skipped operations stay visible.
    for (int i = 0; i < steps.size(); ++i)
        emitLine(QStringLiteral("PLAN|%1|%2|%3|%4").arg(i + 1).arg(steps.size(), 0)
                     .arg(steps[i].action, steps[i].target));
    QStringList removed;
    for (int i = 0; i < steps.size(); ++i) {
        const auto &step = steps[i];
        emitLine(QStringLiteral("STEP|%1|%2|%3|%4").arg(i + 1).arg(steps.size(), 0)
                     .arg(step.action, step.target));
        const bool ok = run(step.program, step.arguments);
        emitLine(QStringLiteral("RESULT|%1|%2").arg(i + 1).arg(ok ? QStringLiteral("completed") : QStringLiteral("failed")));
        if (!ok) {
            // Restore only when driver switching failed. A later support failure
            // must not undo an otherwise successful driver switch.
            if ((step.action == QLatin1String("remove") || step.action == QLatin1String("install")) && !removed.isEmpty()) {
                const auto current = read();
                bool canRestore = current.isValid() && current.chwdAvailable;
                for (const auto &name : install) {
                    const auto *profile = current.profile(QStringLiteral("chwd:") + name);
                    if (profile && profile->installed) canRestore = false;
                }
                for (const auto &name : removed) {
                    const bool ran = canRestore && run(QStringLiteral("/usr/bin/chwd"), {QStringLiteral("--install"), name});
                    const auto recovered = read();
                    const auto *profile = recovered.profile(QStringLiteral("chwd:") + name);
                    const bool restored = ran && recovered.isValid() && recovered.chwdAvailable && profile && profile->installed;
                    emitLine(QStringLiteral("RECOVERY|%1|%2").arg(name, restored ? QStringLiteral("completed") : QStringLiteral("needs-attention")));
                }
            }
            emitError(QStringLiteral("Operation failed: %1 %2. Review the step results before retrying.")
                          .arg(step.action, step.target));
            return false;
        }
        if (step.action == QLatin1String("remove")) removed.append(step.target);
    }
    const auto after = read();
    if (!after.isValid() || (changesDrivers && !after.chwdAvailable)) {
        emitError(QStringLiteral("Cannot verify the final hardware state. Refresh before retrying."));
        return false;
    }
    for (const auto &name : remove) {
        const auto *profile = after.profile(QStringLiteral("chwd:") + name);
        if (profile && profile->installed) {
            emitLine(QStringLiteral("ERROR|verify|%1|still-installed").arg(name));
            return false;
        }
    }
    for (const auto &name : install) {
        const auto *profile = after.profile(QStringLiteral("chwd:") + name);
        if (!profile || !profile->installed) {
            emitLine(QStringLiteral("ERROR|verify|%1|not-installed").arg(name));
            return false;
        }
    }
    for (const auto &id : support) {
        const auto *profile = after.profile(id);
        if (!profile || !profile->ready) {
            emitLine(QStringLiteral("ERROR|support|%1|not-ready").arg(id));
            return false;
        }
    }
    emitLine(QStringLiteral("DONE"));
    return true;
}
}
