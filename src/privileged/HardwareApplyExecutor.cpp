#include "model/HardwareTypes.h"
#include "services/HardwareService.h"
#include "services/HardwareTransaction.h"
#include "services/HardwareApplyRequest.h"

#include <QCoreApplication>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QFileInfo>

#include <cstdio>
#include <csignal>
#include <unistd.h>

namespace {
QFile *transactionLog = nullptr;

void writeLog(const QByteArray &data)
{
    if (!transactionLog || !transactionLog->isOpen()) return;
    transactionLog->write(data);
    transactionLog->flush();
}

bool execute(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, arguments);
    if (!process.waitForStarted(15000)) return false;
    const bool finished = process.waitForFinished(-1);
    const QByteArray output = process.readAll();
    if (!output.isEmpty()) {
        std::fwrite(output.constData(), 1, size_t(output.size()), stdout);
        std::fflush(stdout);
        writeLog(output);
    }
    return finished && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

}

int main(int argc, char **argv)
{
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGHUP, SIG_IGN);
    QCoreApplication application(argc, argv);
    if (geteuid() != 0) {
        std::fprintf(stderr, "Administrator authorization required.\n");
        return 2;
    }
    QFile logFile(QStringLiteral("/var/log/catos-hardware-manager/last-transaction.log"));
    QDir().mkpath(QFileInfo(logFile.fileName()).absolutePath());
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        transactionLog = &logFile;
        writeLog((QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + QStringLiteral(" START\n")).toUtf8());
    }
    HardwareApplyRequest request;
    if (!HardwareApplyRequestParser::read(application.arguments(), request)) return 2;
    if (QFileInfo::exists(QStringLiteral("/var/lib/pacman/db.lck"))) {
        std::fprintf(stderr, "ERROR|package-manager|locked\n");
        writeLog(QByteArray("ERROR|package-manager|locked\n"));
        return 4;
    }

    const HardwareSnapshot before = HardwareService::read(QStringLiteral("/usr/bin/chwd"), ReadOptions{false});
    if (!before.isValid()) {
        std::fprintf(stderr, "Unable to query hardware profile state.\n");
        return 3;
    }
    if (!HardwareApplyRequestParser::validate(request, before)) {
        std::fprintf(stderr, "Hardware state or supported profiles have changed. Refresh and review again.\n");
        return 3;
    }

    const bool ok = HardwareTransaction::apply(before, request.remove, request.install, request.support,
        execute, [] { return HardwareService::read(QStringLiteral("/usr/bin/chwd"), ReadOptions{false}); },
        [](const QString &line) { writeLog((line + QLatin1Char('\n')).toUtf8()); },
        [](const QString &line) { writeLog((QStringLiteral("ERROR|") + line + QLatin1Char('\n')).toUtf8()); });
    writeLog((ok ? QByteArray("DONE\n") : QByteArray("FAILED\n")));
    return ok ? 0 : 4;
}
