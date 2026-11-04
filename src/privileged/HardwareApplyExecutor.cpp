#include "model/HardwareTypes.h"
#include "services/HardwareService.h"

#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <cstdio>
#include <unistd.h>

namespace {
struct Request {
    QSet<QString> expectedChwd;
    QStringList remove;
    QStringList install;
    QStringList support;
};

bool validName(const QString &value)
{
    static const QRegularExpression profile(QStringLiteral("^[a-z0-9][a-z0-9._-]{0,127}$"));
    return profile.match(value).hasMatch();
}

bool readRequest(const QStringList &arguments, Request &request)
{
    QSet<QString> used;
    for (int i = 1; i < arguments.size(); i += 2) {
        if (i + 1 >= arguments.size()) return false;
        const QString operation = arguments[i];
        const QString name = arguments[i + 1];
        if (operation == QLatin1String("--expect")) {
            if (!validName(name)) return false;
            request.expectedChwd.insert(name);
            continue;
        }
        if (operation == QLatin1String("--support-install")) {
            if (!name.startsWith(QLatin1String("support:"))
                || !validName(name.mid(8)) || used.contains(name)) return false;
            request.support.append(name);
        } else if (operation == QLatin1String("--install")
                   || operation == QLatin1String("--remove")) {
            if (!validName(name) || used.contains(name)) return false;
            if (operation == QLatin1String("--install")) request.install.append(name);
            else request.remove.append(name);
        } else return false;
        used.insert(name);
    }
    return !request.remove.isEmpty() || !request.install.isEmpty() || !request.support.isEmpty();
}

bool validate(const Request &request, const HardwareSnapshot &snapshot)
{
    QSet<QString> current;
    for (const auto &profile : snapshot.profiles) {
        if (profile.source == ProfileSource::Chwd && profile.installed)
            current.insert(profile.chwdName);
    }
    if (current != request.expectedChwd) return false;

    QSet<QString> finalChwd = current;
    for (const auto &name : request.remove) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (!profile || !profile->installed) return false;
        finalChwd.remove(name);
    }
    for (const auto &name : request.install) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (!profile || profile->installed || !profile->applicable) return false;
        finalChwd.insert(name);
    }
    QSet<QString> groups;
    for (const auto &name : finalChwd) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (profile && !profile->conflictGroup.isEmpty()
            && groups.contains(profile->conflictGroup)) return false;
        if (profile && !profile->conflictGroup.isEmpty()) groups.insert(profile->conflictGroup);
    }
    for (const auto &id : request.support) {
        const auto *profile = snapshot.profile(id);
        if (!profile || profile->source != ProfileSource::Support
            || !profile->applicable || profile->installed) return false;
    }
    return true;
}

bool execute(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(program, arguments);
    if (!process.waitForStarted(15000) || !process.waitForFinished(-1)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return false;
    return true;
}

void step(int index, int total, const QString &action, const QString &profile)
{
    const QByteArray actionBytes = action.toUtf8();
    const QByteArray profileBytes = profile.toUtf8();
    std::printf("STEP|%d|%d|%s|%s\n", index, total,
                actionBytes.constData(), profileBytes.constData());
    std::fflush(stdout);
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (geteuid() != 0) {
        std::fprintf(stderr, "Administrator authorization required.\n");
        return 2;
    }
    Request request;
    if (!readRequest(application.arguments(), request)) return 2;

    const HardwareSnapshot before = HardwareService::read();
    if (!before.isValid()) {
        std::fprintf(stderr, "Unable to query hardware profile state.\n");
        return 3;
    }
    if (!validate(request, before)) {
        std::fprintf(stderr, "Hardware state or supported profiles have changed. Refresh and review again.\n");
        return 3;
    }

    QSet<QString> packageSet;
    QSet<QString> serviceSet;
    for (const auto &id : request.support) {
        const auto *profile = before.profile(id);
        for (const auto &package : profile->packages) {
            if (!before.installedPackages.contains(package)) packageSet.insert(package);
        }
        for (const auto &service : profile->services) {
            if (!before.enabledServices.contains(service)) serviceSet.insert(service);
        }
    }
    QStringList packages = packageSet.values();
    QStringList services = serviceSet.values();
    packages.sort();
    services.sort();
    const int total = request.remove.size() + request.install.size()
                      + (packages.isEmpty() ? 0 : 1) + services.size();
    int progress = 0;

    for (const auto &name : request.remove) {
        step(++progress, total, QStringLiteral("remove"), name);
        if (!execute(QStringLiteral("/usr/bin/chwd"), {QStringLiteral("--remove"), name})) return 4;
    }
    for (const auto &name : request.install) {
        step(++progress, total, QStringLiteral("install"), name);
        if (!execute(QStringLiteral("/usr/bin/chwd"), {QStringLiteral("--install"), name})) return 4;
    }
    if (!packages.isEmpty()) {
        step(++progress, total, QStringLiteral("packages"), packages.join(QStringLiteral(", ")));
        QStringList arguments{QStringLiteral("-S"), QStringLiteral("--needed"),
                              QStringLiteral("--noconfirm"), QStringLiteral("--")};
        arguments.append(packages);
        if (!execute(QStringLiteral("/usr/bin/pacman"), arguments)) return 4;
    }
    for (const auto &name : services) {
        step(++progress, total, QStringLiteral("service"), name);
        if (!execute(QStringLiteral("/usr/bin/systemctl"),
                     {QStringLiteral("enable"), QStringLiteral("--now"), name})) return 4;
    }
    const HardwareSnapshot after = HardwareService::read();
    if (!after.isValid()) return 4;
    for (const auto &name : request.remove) {
        const auto *p = after.profile(QStringLiteral("chwd:") + name);
        if (p && p->installed) return 4;
    }
    for (const auto &name : request.install) {
        const auto *p = after.profile(QStringLiteral("chwd:") + name);
        if (!p || !p->installed) return 4;
    }
    for (const auto &id : request.support) {
        const auto *p = after.profile(id);
        if (!p || !p->installed) return 4;
    }
    std::puts("DONE");
    return 0;
}
