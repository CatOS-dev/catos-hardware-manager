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
    auto consume = [](const QByteArray &chunk) {
        if (chunk.isEmpty()) return;
        std::fwrite(chunk.constData(), 1, size_t(chunk.size()), stdout);
        std::fflush(stdout);
        writeLog(chunk);
    };
    while (process.state() != QProcess::NotRunning) {
        if (process.waitForReadyRead(100)) consume(process.readAll());
        else if (!process.waitForFinished(100)) continue;
    }
    consume(process.readAll());
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
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
    // Every exit after the log was opened must end it with a verdict;
    // a log without DONE or FAILED means the executor was interrupted.
    const auto fail = [](int code, const char *record, const char *message) {
        std::printf("%s\n", record);
        std::fflush(stdout);
        std::fprintf(stderr, "%s\n", message);
        writeLog(QByteArray(record) + '\n');
        writeLog(QByteArray("FAILED\n"));
        return code;
    };
    HardwareApplyRequest request;
    if (!HardwareApplyRequestParser::read(application.arguments(), request))
        return fail(2, "ERROR|request|invalid", "Invalid request.");
    if (QFileInfo::exists(QStringLiteral("/var/lib/pacman/db.lck")))
        return fail(4, "ERROR|package-manager|locked", "Another package manager is running.");

    const HardwareSnapshot before = HardwareService::read(QStringLiteral("/usr/bin/chwd"), ReadOptions{false});
    if (!before.isValid())
        return fail(3, "ERROR|state|initial|invalid", "Unable to query hardware profile state.");
    if (!HardwareApplyRequestParser::validate(request, before))
        return fail(3, "ERROR|state|changed", "Hardware state or supported profiles have changed. Refresh and review again.");

    const bool ok = HardwareTransaction::apply(before, request,
        execute, [] { return HardwareService::read(QStringLiteral("/usr/bin/chwd"), ReadOptions{false}); },
        [](const QString &line) { writeLog((line + QLatin1Char('\n')).toUtf8()); },
        [](const QString &line) { writeLog((QStringLiteral("ERROR|") + line + QLatin1Char('\n')).toUtf8()); });
    writeLog((ok ? QByteArray("DONE\n") : QByteArray("FAILED\n")));
    return ok ? 0 : 4;
}
