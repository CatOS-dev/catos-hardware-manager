#include "ApplyPreflight.h"

#include "HardwareTransaction.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace {
QByteArray output(const QString &program, const QStringList &arguments, int timeout, int *exitCode = nullptr)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(2000)) return {};
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished(1000);
        return {};
    }
    if (exitCode) *exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    return process.readAllStandardOutput();
}
}

QStringList Preflight::packagesToInstall(const HardwareSnapshot &snapshot, const QStringList &installChwd,
                                         const QStringList &support)
{
    QStringList packages;
    const auto add = [&packages](const QStringList &names) {
        for (const auto &name : names)
            if (!packages.contains(name)) packages.append(name);
    };
    for (const auto &name : installChwd)
        if (const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name)) add(profile->packages);
    for (const auto &id : support)
        if (const auto *profile = snapshot.profile(id)) add(profile->missingPackages);
    return packages;
}

bool Preflight::needsDkms(const HardwareSnapshot &snapshot, const QStringList &installChwd)
{
    for (const auto &name : installChwd) {
        const auto *profile = snapshot.profile(QStringLiteral("chwd:") + name);
        if (!profile) continue;
        for (const auto &package : profile->packages)
            if (package.contains(QStringLiteral("dkms"), Qt::CaseInsensitive)) return true;
    }
    return false;
}

QStringList Preflight::kernelsWithoutHeaders(const QString &root)
{
    // Arch kernel packages install /usr/lib/modules/<version>/vmlinuz; a
    // directory without it is a leftover of a removed kernel. The matching
    // headers package provides the "build" link DKMS compiles against.
    QStringList missing;
    const QDir modules(QDir(root).filePath(QStringLiteral("usr/lib/modules")));
    for (const auto &version : modules.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        const QDir kernel(modules.filePath(version));
        if (!kernel.exists(QStringLiteral("vmlinuz"))) continue;
        if (!QFileInfo(kernel.filePath(QStringLiteral("build"))).isDir()) missing.append(version);
    }
    return missing;
}

bool Preflight::secureBootEnabled(const QString &root)
{
    // EFI variable: 4 attribute bytes followed by one data byte.
    QFile variable(QDir(root).filePath(QStringLiteral(
        "sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-e0e8ef33ca8c")));
    if (!variable.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = variable.readAll();
    return data.size() >= 5 && data.at(4) == 1;
}

ApplyPreflight Preflight::run(const HardwareSnapshot &snapshot, const QStringList &removeChwd,
                              const QStringList &installChwd, const QStringList &support)
{
    ApplyPreflight result;
    const bool changesDrivers = !removeChwd.isEmpty() || !installChwd.isEmpty();
    if (needsDkms(snapshot, installChwd)) {
        result.kernelsWithoutHeaders = kernelsWithoutHeaders(QStringLiteral("/"));
        result.secureBoot = secureBootEnabled(QStringLiteral("/"));
    }
    if (changesDrivers) {
        result.snapshots = HardwareTransaction::snapshotsAvailable();
        // Switching drivers on a partially upgraded system is the riskiest
        // case on Arch; pacman -Qu only reads the local sync databases.
        int code = -1;
        const QByteArray updates = output(QStringLiteral("/usr/bin/pacman"), {QStringLiteral("-Qu")}, 10000, &code);
        result.updatesPending = code == 0 && !updates.trimmed().isEmpty();
    }
    const QStringList packages = packagesToInstall(snapshot, installChwd, support);
    if (!packages.isEmpty()) {
        int code = -1;
        const QByteArray sizes = output(QStringLiteral("/usr/bin/pacman"),
            QStringList{QStringLiteral("-Sp"), QStringLiteral("--needed"), QStringLiteral("--print-format"),
                        QStringLiteral("%s"), QStringLiteral("--")} + packages, 10000, &code);
        if (code == 0) {
            qint64 total = 0;
            for (const auto &line : sizes.split('\n')) {
                bool ok = false;
                const qint64 bytes = line.trimmed().toLongLong(&ok);
                if (ok) total += bytes;
            }
            result.downloadBytes = total;
        }
    }
    return result;
}
