#include "Cli.h"

#include "dialogs/TransactionProtocol.h"
#include "model/ProfileSelection.h"
#include "services/HardwareService.h"
#include "widgets/HardwareUi.h"
#include "widgets/Presentation.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTextStream>

#include <cstdio>
#include <unistd.h>

namespace {
QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream &err()
{
    static QTextStream stream(stderr);
    return stream;
}

QString levelName(StatusLevel level)
{
    switch (level) {
    case StatusLevel::Ok: return QStringLiteral("ok");
    case StatusLevel::Pending: return QStringLiteral("pending");
    case StatusLevel::Attention: return QStringLiteral("attention");
    case StatusLevel::Error: return QStringLiteral("error");
    case StatusLevel::Neutral: return QStringLiteral("neutral");
    }
    return {};
}

struct Plan {
    QStringList remove;
    QStringList install;
    QStringList support;
    bool isEmpty() const { return remove.isEmpty() && install.isEmpty() && support.isEmpty(); }
};

Plan recommendedPlan(const HardwareSnapshot &snapshot, ProfileSelection &selection)
{
    selection.refresh(snapshot, false);
    selection.selectRecommended(snapshot);
    return {selection.removingChwd(snapshot), selection.installingChwd(snapshot), selection.enablingSupport(snapshot)};
}

QString profileName(const HardwareSnapshot &snapshot, const QString &id)
{
    const auto *profile = snapshot.profile(id);
    return profile ? HardwareUi::translatedName(*profile) : id;
}

int list(const HardwareSnapshot &snapshot, bool json)
{
    ProfileSelection current;
    current.refresh(snapshot, false);
    ProfileSelection recommended;
    const Plan plan = recommendedPlan(snapshot, recommended);
    if (json) {
        QJsonArray devices;
        for (const auto &device : snapshot.devices) {
            const auto status = Presentation::deviceStatus(device, snapshot, current);
            devices.append(QJsonObject{
                {QStringLiteral("id"), device.id},
                {QStringLiteral("category"), categoryKey(device.category)},
                {QStringLiteral("name"), device.name},
                {QStringLiteral("driver"), device.driver},
                {QStringLiteral("status"), levelName(status.level)},
                {QStringLiteral("statusText"), status.text},
                {QStringLiteral("profiles"), QJsonArray::fromStringList(device.profileIds)}});
        }
        const QJsonObject root{
            {QStringLiteral("devices"), devices},
            {QStringLiteral("warnings"), QJsonArray::fromStringList(snapshot.warnings)},
            {QStringLiteral("recommended"), QJsonObject{
                {QStringLiteral("installDrivers"), QJsonArray::fromStringList(plan.install)},
                {QStringLiteral("removeDrivers"), QJsonArray::fromStringList(plan.remove)},
                {QStringLiteral("installSupport"), QJsonArray::fromStringList(plan.support)}}}};
        out() << QJsonDocument(root).toJson(QJsonDocument::Indented);
        return 0;
    }
    for (const auto category : HardwareUi::categoryOrder()) {
        bool header = false;
        for (const auto &device : snapshot.devices) {
            if (device.category != category) continue;
            if (!header) {
                out() << categoryTitle(category) << '\n';
                header = true;
            }
            const auto status = Presentation::deviceStatus(device, snapshot, current);
            const QString summary = Presentation::deviceSummary(device, snapshot);
            out() << "  " << device.name << "\n    " << status.text;
            if (!summary.isEmpty()) out() << " · " << summary;
            out() << '\n';
        }
    }
    for (const auto &warning : snapshot.warnings) err() << QCoreApplication::translate("Cli", "Warning: %1").arg(warning) << '\n';
    if (plan.isEmpty()) {
        out() << '\n' << QCoreApplication::translate("Cli", "No recommended changes.") << '\n';
        return 0;
    }
    out() << '\n' << QCoreApplication::translate("Cli", "Recommended changes (--apply-recommended):") << '\n';
    for (const auto &name : plan.install) out() << "  + " << profileName(snapshot, QStringLiteral("chwd:") + name) << '\n';
    for (const auto &id : plan.support) out() << "  + " << profileName(snapshot, id) << '\n';
    return 0;
}

int apply(const HardwareSnapshot &snapshot, const ProfileSelection &selection, const Plan &plan, bool yes)
{
    if (plan.isEmpty()) {
        out() << QCoreApplication::translate("Cli", "Nothing to do.") << '\n';
        return 0;
    }
    out() << QCoreApplication::translate("Cli", "The following changes will be applied:") << '\n';
    for (const auto &name : plan.remove) out() << "  - " << profileName(snapshot, QStringLiteral("chwd:") + name) << '\n';
    for (const auto &name : plan.install) out() << "  + " << profileName(snapshot, QStringLiteral("chwd:") + name) << '\n';
    for (const auto &id : plan.support) out() << "  + " << profileName(snapshot, id) << '\n';
    out().flush();
    if (!yes) {
        if (!isatty(STDIN_FILENO)) {
            err() << QCoreApplication::translate("Cli", "Not a terminal: pass --yes to apply without asking.") << '\n';
            return 2;
        }
        out() << QCoreApplication::translate("Cli", "Apply these changes? [y/N] ");
        out().flush();
        QTextStream in(stdin);
        const QString answer = in.readLine().trimmed().toLower();
        if (answer != QLatin1String("y") && answer != QLatin1String("yes")) return 1;
    }

    QStringList arguments;
    for (const auto &name : selection.baseline().values()) arguments << QStringLiteral("--expect") << name;
    for (const auto &name : plan.remove) arguments << QStringLiteral("--remove") << name;
    for (const auto &name : plan.install) arguments << QStringLiteral("--install") << name;
    for (const auto &id : plan.support) arguments << QStringLiteral("--support-install") << id;
    const QString executor = QStringLiteral("/usr/lib/catos/catos-hardware-apply");
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    // Installers run as root already; everyone else authorises through polkit.
    if (geteuid() == 0) process.start(executor, arguments);
    else process.start(QStringLiteral("/usr/bin/pkexec"), QStringList{executor} + arguments);
    if (!process.waitForStarted(10000)) {
        err() << process.errorString() << '\n';
        return 1;
    }
    bool sawPlan = false;
    QString failure;
    QByteArray pending;
    const auto consume = [&] {
        int pos;
        while ((pos = pending.indexOf('\n')) != -1) {
            const QString line = QString::fromUtf8(pending.left(pos)).trimmed();
            pending.remove(0, pos + 1);
            const auto fields = line.split(QLatin1Char('|'));
            if (fields.size() == 5 && fields[0] == QLatin1String("PLAN")) {
                sawPlan = true;
            } else if (fields.size() == 5 && fields[0] == QLatin1String("STEP")) {
                const QString target = TransactionProtocol::targetName(fields[3], fields[4], snapshot);
                out() << QStringLiteral("[%1/%2] ").arg(fields[1], fields[2]) << TransactionProtocol::actionName(fields[3])
                      << (target.isEmpty() ? QString() : QStringLiteral(": ") + target) << '\n';
                out().flush();
            } else if (fields.size() == 3 && fields[0] == QLatin1String("RESULT") && fields[2] != QLatin1String("completed")) {
                out() << "  " << QCoreApplication::translate("Cli", "failed") << '\n';
            } else if (fields[0] == QLatin1String("ERROR")) {
                const QString text = TransactionProtocol::errorMessage(fields, snapshot);
                if (!text.isEmpty() && failure.isEmpty()) failure = text;
            }
        }
    };
    while (process.state() != QProcess::NotRunning) {
        if (process.waitForReadyRead(200)) {
            pending += process.readAll();
            consume();
        }
    }
    pending += process.readAll();
    if (!pending.isEmpty() && !pending.endsWith('\n')) pending += '\n';
    consume();
    const int code = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    if (TransactionProtocol::authorizationCancelled(code, sawPlan)) {
        err() << QCoreApplication::translate("Cli", "Authorization was cancelled. Nothing was changed.") << '\n';
        return 3;
    }
    if (code != 0) {
        err() << (failure.isEmpty() ? QCoreApplication::translate("Cli", "Some changes could not be applied.") : failure) << '\n'
              << QCoreApplication::translate("Cli", "Details: /var/log/catos-hardware-manager/last-transaction.log") << '\n';
        return 1;
    }
    out() << QCoreApplication::translate("Cli", "Done.");
    if (!plan.install.isEmpty() || !plan.remove.isEmpty())
        out() << ' ' << QCoreApplication::translate("Cli", "Restart the computer to use the new drivers.");
    out() << '\n';
    return 0;
}
}

bool Cli::wants(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        const QByteArray argument(argv[i]);
        if (argument == "--list" || argument == "--apply-recommended" || argument == "--help" || argument == "-h")
            return true;
    }
    return false;
}

int Cli::run(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("CatOS"));
    app.setApplicationName(QStringLiteral("catos-hardware-manager"));
    HardwareUi::installTranslations();
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("Cli", "CatOS Hardware Manager"));
    parser.addHelpOption();
    const QCommandLineOption listOption(QStringLiteral("list"), QCoreApplication::translate("Cli", "List devices, their status and recommended changes."));
    const QCommandLineOption jsonOption(QStringLiteral("json"), QCoreApplication::translate("Cli", "With --list: print JSON."));
    const QCommandLineOption applyOption(QStringLiteral("apply-recommended"),
        QCoreApplication::translate("Cli", "Install recommended drivers and support components. Never removes an installed driver."));
    const QCommandLineOption yesOption(QStringLiteral("yes"), QCoreApplication::translate("Cli", "Do not ask for confirmation."));
    parser.addOptions({listOption, jsonOption, applyOption, yesOption});
    parser.process(app);
    if (parser.isSet(listOption) == parser.isSet(applyOption)) {
        err() << QCoreApplication::translate("Cli", "Use either --list or --apply-recommended.") << '\n';
        return 2;
    }
    const HardwareSnapshot snapshot = HardwareService::read(QStringLiteral("/usr/bin/chwd"), ReadOptions{false});
    if (!snapshot.isValid()) {
        err() << QCoreApplication::translate("Cli", "Hardware query failed: %1").arg(snapshot.error) << '\n';
        return 1;
    }
    if (parser.isSet(listOption)) return list(snapshot, parser.isSet(jsonOption));
    ProfileSelection selection;
    const Plan plan = recommendedPlan(snapshot, selection);
    return apply(snapshot, selection, plan, parser.isSet(yesOption));
}
