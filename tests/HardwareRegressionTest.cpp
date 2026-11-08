#include "model/ProfileSelection.h"
#include "services/DiagnosticReport.h"
#include "services/HardwareService.h"
#include "services/HardwareTransaction.h"
#include "services/HardwareApplyRequest.h"
#include "services/ProfileCatalog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

class HardwareRegressionTest : public QObject {
    Q_OBJECT
private:
    HardwareProfile bluetooth() const
    {
        HardwareProfile profile;
        profile.id = QStringLiteral("support:bluetooth");
        profile.source = ProfileSource::Support;
        profile.packages = {QStringLiteral("bluez")};
        profile.services = {QStringLiteral("bluetooth.service")};
        profile.matchRules = {{QStringLiteral("bluetooth"), {}, {}, {}, {}, {}, {}, {}}};
        profile.systemManaged = true;
        return profile;
    }
    HardwareSnapshot stoppedBluetooth() const
    {
        HardwareSnapshot state;
        state.installedPackages.insert(QStringLiteral("bluez"));
        state.enabledServices.insert(QStringLiteral("bluetooth.service"));
        HardwareDevice device;
        device.id = QStringLiteral("usb:1-2");
        device.capabilities = {QStringLiteral("bluetooth")};
        state.devices.append(device);
        HardwareService::attachSupportProfiles(state, {bluetooth()});
        return state;
    }
    HardwareSnapshot drivers() const
    {
        HardwareSnapshot state;
        state.chwdAvailable = true;
        for (const auto &name : {QStringLiteral("old"), QStringLiteral("new")}) {
            HardwareProfile profile;
            profile.source = ProfileSource::Chwd;
            profile.id = QStringLiteral("chwd:") + name;
            profile.chwdName = name;
            profile.applicable = true;
            profile.installed = name == QLatin1String("old");
            profile.packages = {name + QStringLiteral("-driver")};
            state.profiles.append(profile);
        }
        return state;
    }
private slots:
    void initTestCase()
    {
        qputenv("CATOS_DISABLE_SNAPSHOTS", "1");
    }

    void recommendedDriverDoesNotStageInstalledConflictRemoval()
    {
        auto state = drivers();
        state.profiles[0].conflictGroup = QStringLiteral("gpu");
        state.profiles[1].conflictGroup = QStringLiteral("gpu");
        HardwareDevice device;
        device.id = QStringLiteral("pci:0000:01:00.0");
        device.recommendedProfile = QStringLiteral("chwd:new");
        state.devices.append(device);
        ProfileSelection selection;
        selection.refresh(state, false);
        selection.selectRecommended(state);
        QVERIFY(selection.selected(QStringLiteral("chwd:old")));
        QVERIFY(!selection.selected(QStringLiteral("chwd:new")));
        QVERIFY(selection.removingChwd(state).isEmpty());
    }
    void stoppedEnabledServiceCanBeRepaired()
    {
        auto state = stoppedBluetooth();
        QVERIFY(state.profiles.first().installed);
        QVERIFY(!state.profiles.first().ready);
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(!selection.selected(QStringLiteral("support:bluetooth")));
        QVERIFY(selection.select(QStringLiteral("support:bluetooth"), true, state));
        QCOMPARE(selection.enablingSupport(state), QStringList{QStringLiteral("support:bluetooth")});
        QStringList calls;
        const bool result = HardwareTransaction::apply(state, {}, {}, selection.enablingSupport(state),
            [&calls, &state](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                state.activeServices.insert(QStringLiteral("bluetooth.service"));
                HardwareService::updateSupportState(state.profiles[0], state);
                return true;
            }, [&state] { return state; });
        QVERIFY(result);
        QCOMPARE(calls, QStringList{QStringLiteral("/usr/bin/systemctl start bluetooth.service")});
        QVERIFY(state.profiles.first().ready);
    }
    void unpluggingDoesNotUninstallSupport()
    {
        auto state = stoppedBluetooth();
        state.activeServices = state.enabledServices;
        state.devices.clear();
        state.profiles.clear();
        HardwareService::attachSupportProfiles(state, {bluetooth()});
        QVERIFY(!state.profiles.first().applicable);
        QVERIFY(state.profiles.first().installed);
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(!selection.select(QStringLiteral("support:bluetooth"), true, state));
    }
    void executorRequestRejectsDuplicateAndConflictingEntries()
    {
        HardwareApplyRequest request;
        QVERIFY(!HardwareApplyRequestParser::read({QStringLiteral("apply"), QStringLiteral("--expect"), QStringLiteral("old"),
            QStringLiteral("--expect"), QStringLiteral("old")}, request));
        QVERIFY(!HardwareApplyRequestParser::read({QStringLiteral("apply"), QStringLiteral("--install"), QStringLiteral("new"),
            QStringLiteral("--install"), QStringLiteral("new")}, request));

        auto state = drivers();
        state.profiles[0].conflictGroup = QStringLiteral("gpu");
        state.profiles[1].conflictGroup = QStringLiteral("gpu");
        request = {};
        QVERIFY(HardwareApplyRequestParser::read({QStringLiteral("apply"), QStringLiteral("--expect"), QStringLiteral("old"),
            QStringLiteral("--remove"), QStringLiteral("old"), QStringLiteral("--install"), QStringLiteral("new")}, request));
        QVERIFY(HardwareApplyRequestParser::validate(request, state));
        request.install = {QStringLiteral("new"), QStringLiteral("old")};
        QVERIFY(!HardwareApplyRequestParser::validate(request, state));
    }

    void moduleRulesMatchWithoutBoundDriver()
    {
        QString error;
        const auto catalog = ProfileCatalog::load(QStringLiteral(PROFILE_DIRECTORY), &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        const auto it = std::find_if(catalog.cbegin(), catalog.cend(), [](const HardwareProfile &profile) {
            return profile.id == QLatin1String("support:audio-sof");
        });
        QVERIFY(it != catalog.cend());
        HardwareDevice device;
        device.bus = QStringLiteral("pci");
        device.modalias = QStringLiteral("acpi:INT34C8:");
        device.modules = {QStringLiteral("snd_sof_pci_intel_tgl")};
        QVERIFY(ProfileCatalog::matches(*it, device));
    }

    void scannerBootstrapDoesNotNeedScanimageOrDevice()
    {
        QString error;
        const auto catalog = ProfileCatalog::load(QStringLiteral(PROFILE_DIRECTORY), &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        HardwareSnapshot state;
        HardwareService::attachSupportProfiles(state, catalog);
        const auto *scanner = state.profile(QStringLiteral("support:scanner"));
        QVERIFY(scanner);
        QVERIFY(scanner->discovery);
        QVERIFY(scanner->applicable);
        QVERIFY(scanner->missingPackages.contains(QStringLiteral("sane")));
        QVERIFY(state.devices.isEmpty());
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(selection.select(scanner->id, true, state));
        QCOMPARE(selection.enablingSupport(state), QStringList{scanner->id});
    }
    void removalDoesNotDownloadRecoveryPackages()
    {
        auto state = drivers();
        QStringList calls;
        const bool result = HardwareTransaction::apply(state, {QStringLiteral("old")}, {}, {},
            [&calls, &state](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                state.profiles[0].installed = false;
                return true;
            }, [&state] { return state; });
        QVERIFY(result);
        QCOMPARE(calls, QStringList{QStringLiteral("/usr/bin/chwd --remove old")});
    }
    void missingChwdCannotVerifyRemoval()
    {
        const auto before = drivers();
        const bool result = HardwareTransaction::apply(before, {QStringLiteral("old")}, {}, {},
            [](const QString &, const QStringList &) { return true; }, [] { return HardwareSnapshot{}; });
        QVERIFY(!result);
    }
    void missingChwdCannotAuthorizeRecovery()
    {
        const auto before = drivers();
        QStringList calls;
        const bool result = HardwareTransaction::apply(before, {QStringLiteral("old")}, {QStringLiteral("new")}, {},
            [&calls](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                return arguments != QStringList{QStringLiteral("--install"), QStringLiteral("new")};
            }, [] { return HardwareSnapshot{}; });
        QVERIFY(!result);
        QCOMPARE(calls.size(), 3);
        QCOMPARE(calls.last(), QStringLiteral("/usr/bin/chwd --install new"));
    }
    void failedDownloadNeverRemovesDriver()
    {
        const auto before = drivers();
        QStringList calls;
        const bool result = HardwareTransaction::apply(before, {QStringLiteral("old")}, {QStringLiteral("new")}, {},
            [&calls](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                return false;
            }, [&before] { return before; });
        QVERIFY(!result);
        QCOMPARE(calls, QStringList{QStringLiteral("/usr/bin/pacman -Sw --noconfirm -- new-driver old-driver")});
    }
    void failedSwitchRestoresRemovedDriver()
    {
        auto state = drivers();
        const auto before = state;
        QStringList calls;
        const bool result = HardwareTransaction::apply(before, {QStringLiteral("old")}, {QStringLiteral("new")}, {},
            [&calls, &state](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                if (arguments == QStringList{QStringLiteral("--remove"), QStringLiteral("old")}) state.profiles[0].installed = false;
                if (arguments == QStringList{QStringLiteral("--install"), QStringLiteral("old")}) state.profiles[0].installed = true;
                return arguments != QStringList{QStringLiteral("--install"), QStringLiteral("new")};
            }, [&state] { return state; });
        QVERIFY(!result);
        QCOMPARE(calls, QStringList({QStringLiteral("/usr/bin/pacman -Sw --noconfirm -- new-driver old-driver"),
            QStringLiteral("/usr/bin/chwd --remove old"), QStringLiteral("/usr/bin/chwd --install new"),
            QStringLiteral("/usr/bin/chwd --install old")}));
        QVERIFY(state.profiles[0].installed);
    }
    void laterServiceFailureKeepsSuccessfulDriverSwitch()
    {
        auto state = drivers();
        auto support = bluetooth();
        support.applicable = true;
        state.profiles.append(support);
        state.installedPackages.insert(QStringLiteral("bluez"));
        HardwareService::updateSupportState(state.profiles[2], state);
        QStringList calls;
        const bool result = HardwareTransaction::apply(state, {QStringLiteral("old")}, {QStringLiteral("new")}, {support.id},
            [&calls, &state](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                if (arguments == QStringList{QStringLiteral("--remove"), QStringLiteral("old")}) state.profiles[0].installed = false;
                if (arguments == QStringList{QStringLiteral("--install"), QStringLiteral("new")}) state.profiles[1].installed = true;
                return program != QLatin1String("/usr/bin/systemctl");
            }, [&state] { return state; });
        QVERIFY(!result);
        QCOMPARE(calls.size(), 4);
        QCOMPARE(calls.last(), QStringLiteral("/usr/bin/systemctl enable --now bluetooth.service"));
        QVERIFY(!state.profiles[0].installed);
        QVERIFY(state.profiles[1].installed);
    }
    void successfulCommandStillRequiresActiveService()
    {
        auto state = stoppedBluetooth();
        const bool result = HardwareTransaction::apply(state, {}, {}, {QStringLiteral("support:bluetooth")},
            [](const QString &, const QStringList &) { return true; }, [&state] { return state; });
        QVERIFY(!result);
        QVERIFY(!state.profiles[0].ready);
    }
    void refreshDoesNotStageUnexpectedServiceRepair()
    {
        auto state = stoppedBluetooth();
        state.activeServices = state.enabledServices;
        HardwareService::updateSupportState(state.profiles[0], state);
        ProfileSelection selection;
        selection.refresh(state, false);
        state.activeServices.clear();
        HardwareService::updateSupportState(state.profiles[0], state);
        selection.refresh(state, true);
        QVERIFY(selection.enablingSupport(state).isEmpty());
        QVERIFY(selection.select(QStringLiteral("support:bluetooth"), true, state));
        selection.refresh(state, true);
        QCOMPARE(selection.enablingSupport(state), QStringList{QStringLiteral("support:bluetooth")});
        state.profiles[0].applicable = false;
        selection.refresh(state, true);
        QVERIFY(selection.enablingSupport(state).isEmpty());
    }
    void retryRetainsOnlyRemainingChanges()
    {
        auto state = drivers();
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(selection.select(QStringLiteral("chwd:old"), false, state));
        QVERIFY(selection.select(QStringLiteral("chwd:new"), true, state));
        state.profiles[0].installed = false;
        selection.refresh(state, true);
        QVERIFY(selection.removingChwd(state).isEmpty());
        QCOMPARE(selection.installingChwd(state), QStringList{QStringLiteral("new")});
    }
    void diagnosticReportOmitsPrivateFields()
    {
        auto state = stoppedBluetooth();
        state.devices[0].category = DeviceCategory::Bluetooth;
        state.devices[0].name = QStringLiteral("PRIVATE_SERIAL");
        state.devices[0].address = QStringLiteral("PRIVATE_ADDRESS");
        state.devices[0].modalias = QStringLiteral("PRIVATE_ALIAS");
        state.warnings = {QStringLiteral("PRIVATE_LOG")};
        state.error = QStringLiteral("PRIVATE_ERROR");
        const auto report = DiagnosticReport::create(state);
        QVERIFY(!report.contains("PRIVATE_"));
        const auto document = QJsonDocument::fromJson(report);
        QVERIFY(document.isObject());
        QCOMPARE(document.object().value(QStringLiteral("queryFailed")).toBool(), true);
        const auto devices = document.object().value(QStringLiteral("devices")).toArray();
        QCOMPARE(devices.first().toObject().value(QStringLiteral("category")).toString(), QStringLiteral("Bluetooth"));
        QVERIFY(report.contains("bluetooth.service"));
    }
};
QTEST_GUILESS_MAIN(HardwareRegressionTest)
#include "HardwareRegressionTest.moc"
