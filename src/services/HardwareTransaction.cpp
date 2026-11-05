#include "HardwareTransaction.h"

#include <cstdio>

namespace HardwareTransaction {
bool apply(const HardwareSnapshot &before, const QStringList &remove,
           const QStringList &install, const QStringList &support,
           const Runner &run, const Reader &read)
{
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
    for (const auto &name : install + remove) {
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
        std::printf("PLAN|%d|%d|%s|%s\n", i + 1, int(steps.size()),
                    steps[i].action.toUtf8().constData(), steps[i].target.toUtf8().constData());
    std::fflush(stdout);
    QStringList removed;
    for (int i = 0; i < steps.size(); ++i) {
        const auto &step = steps[i];
        std::printf("STEP|%d|%d|%s|%s\n", i + 1, int(steps.size()),
                    step.action.toUtf8().constData(), step.target.toUtf8().constData());
        std::fflush(stdout);
        const bool ok = run(step.program, step.arguments);
        std::printf("RESULT|%d|%s\n", i + 1, ok ? "completed" : "failed");
        std::fflush(stdout);
        if (!ok) {
            // Restore only when driver switching failed. A later support failure
            // must not undo an otherwise successful driver switch.
            if ((step.action == QLatin1String("remove") || step.action == QLatin1String("install")) && !removed.isEmpty()) {
                const auto current = read();
                bool canRestore = current.isValid();
                for (const auto &name : install) {
                    const auto *profile = current.profile(QStringLiteral("chwd:") + name);
                    if (profile && profile->installed) canRestore = false;
                }
                for (const auto &name : removed) {
                    const bool ran = canRestore && run(QStringLiteral("/usr/bin/chwd"), {QStringLiteral("--install"), name});
                    const auto recovered = read();
                    const auto *profile = recovered.profile(QStringLiteral("chwd:") + name);
                    const bool restored = ran && recovered.isValid() && profile && profile->installed;
                    std::printf("RECOVERY|%s|%s\n", name.toUtf8().constData(), restored ? "completed" : "needs-attention");
                }
                std::fflush(stdout);
            }
            std::fprintf(stderr, "Operation failed: %s %s. Review the step results before retrying.\n",
                         step.action.toUtf8().constData(), step.target.toUtf8().constData());
            return false;
        }
        if (step.action == QLatin1String("remove")) removed.append(step.target);
    }
    const auto after = read();
    if (!after.isValid()) {
        std::fprintf(stderr, "Cannot verify the final hardware state. Refresh before retrying.\n");
        return false;
    }
    for (const auto &name : remove) {
        const auto *profile = after.profile(QStringLiteral("chwd:") + name);
        if (profile && profile->installed) return false;
    }
    for (const auto &name : install) {
        const auto *profile = after.profile(QStringLiteral("chwd:") + name);
        if (!profile || !profile->installed) return false;
    }
    for (const auto &id : support) {
        const auto *profile = after.profile(id);
        if (!profile || !profile->ready) {
            std::fprintf(stderr, "Support verification failed: %s. Check packages and service status.\n", id.toUtf8().constData());
            return false;
        }
    }
    std::puts("DONE");
    return true;
}
}
