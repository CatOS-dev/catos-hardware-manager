#include "model/ProfileSelection.h"
#include "services/ApplyPreflight.h"
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
    void executorRequestRejectsStaleExpectationAndInvalidProfile()
    {
        auto state = drivers();
        HardwareApplyRequest request;
        QVERIFY(HardwareApplyRequestParser::read({QStringLiteral("apply"), QStringLiteral("--expect"), QStringLiteral("new"),
            QStringLiteral("--install"), QStringLiteral("old")}, request));
        QVERIFY(!HardwareApplyRequestParser::validate(request, state));
        QVERIFY(!HardwareApplyRequestParser::read({QStringLiteral("apply"), QStringLiteral("--install"), QStringLiteral("../bad")}, request));
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
    void supportConflictsAreParsedAndRejected()
    {
        QString error;
        const auto catalog = ProfileCatalog::load(QStringLiteral(PROFILE_DIRECTORY), &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        const auto it = std::find_if(catalog.cbegin(), catalog.cend(), [](const HardwareProfile &profile) {
            return profile.id == QLatin1String("support:power-profiles");
        });
        QVERIFY(it != catalog.cend());
        QCOMPARE(it->conflicts, QStringList({QStringLiteral("tlp"), QStringLiteral("tuned"),
                                             QStringLiteral("tuned-ppd"), QStringLiteral("auto-cpufreq")}));
        HardwareSnapshot state;
        state.installedPackages.insert(QStringLiteral("tlp"));
        HardwareService::attachSupportProfiles(state, catalog);
        const auto *profile = state.profile(QStringLiteral("support:power-profiles"));
        QVERIFY(profile);
        QVERIFY(!profile->conflictingPackages.isEmpty());
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(!selection.select(profile->id, true, state));
    }
    void deviceRulesCoverUsbFingerprintAndPowerCapabilities()
    {
        QString error;
        const auto catalog = ProfileCatalog::load(QStringLiteral(PROFILE_DIRECTORY), &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        const auto fingerprint = std::find_if(catalog.cbegin(), catalog.cend(), [](const HardwareProfile &profile) {
            return profile.id == QLatin1String("support:fingerprint");
        });
        QVERIFY(fingerprint != catalog.cend());
        HardwareDevice reader;
        reader.bus = QStringLiteral("usb");
        reader.vendorId = QStringLiteral("27c6");
        reader.productId = QStringLiteral("609c");
        QVERIFY(ProfileCatalog::matches(*fingerprint, reader));
        const auto power = std::find_if(catalog.cbegin(), catalog.cend(), [](const HardwareProfile &profile) {
            return profile.id == QLatin1String("support:power-profiles");
        });
        QVERIFY(power != catalog.cend());
        HardwareDevice battery;
        battery.capabilities = {QStringLiteral("battery")};
        QVERIFY(ProfileCatalog::matches(*power, battery));
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
        QVERIFY(document.object().value(QStringLiteral("warningSummary")).toArray().contains(QStringLiteral("hardware-query-warning")));
        const auto devices = document.object().value(QStringLiteral("devices")).toArray();
        QCOMPARE(devices.first().toObject().value(QStringLiteral("category")).toString(), QStringLiteral("Bluetooth"));
        QVERIFY(report.contains("bluetooth.service"));
    }
    void microcodeBootDetection_data()
    {
        QTest::addColumn<QStringList>("files");
        QTest::addColumn<int>("expected");
        const QString pacmanHooks = QStringLiteral("etc/mkinitcpio.conf=HOOKS=(base udev autodetect modconf block filesystems fsck)");
        QTest::newRow("dracut embeds microcode") << QStringList{QStringLiteral("usr/bin/dracut=")}
            << int(MicrocodeBoot::Loaded);
        QTest::newRow("mkinitcpio microcode hook") << QStringList{QStringLiteral("usr/bin/mkinitcpio="),
            QStringLiteral("etc/mkinitcpio.conf=HOOKS=(base systemd autodetect microcode modconf block filesystems)"),
            QStringLiteral("boot/limine.conf=/vmlinuz-linux")} << int(MicrocodeBoot::Loaded);
        QTest::newRow("drop-in adds hook") << QStringList{QStringLiteral("usr/bin/mkinitcpio="), pacmanHooks,
            QStringLiteral("etc/mkinitcpio.conf.d/10-microcode.conf=HOOKS=(base udev microcode block)"),
            QStringLiteral("boot/grub/grub.cfg=linux /vmlinuz-linux")} << int(MicrocodeBoot::Loaded);
        QTest::newRow("grub loads image") << QStringList{QStringLiteral("usr/bin/mkinitcpio="), pacmanHooks,
            QStringLiteral("boot/grub/grub.cfg=initrd /amd-ucode.img /initramfs-linux.img")} << int(MicrocodeBoot::Loaded);
        QTest::newRow("systemd-boot entry misses image") << QStringList{QStringLiteral("usr/bin/mkinitcpio="), pacmanHooks,
            QStringLiteral("boot/loader/entries/arch.conf=initrd /initramfs-linux.img")} << int(MicrocodeBoot::NotLoaded);
        QTest::newRow("limine entry misses image") << QStringList{QStringLiteral("usr/bin/mkinitcpio="), pacmanHooks,
            QStringLiteral("boot/limine/limine.conf=module_path: boot():/initramfs-linux.img")} << int(MicrocodeBoot::NotLoaded);
        QTest::newRow("unreadable boot partition") << QStringList{QStringLiteral("usr/bin/mkinitcpio="), pacmanHooks}
            << int(MicrocodeBoot::Unknown);
        QTest::newRow("unknown generator") << QStringList{} << int(MicrocodeBoot::Unknown);
    }
    void microcodeBootDetection()
    {
        QFETCH(QStringList, files);
        QFETCH(int, expected);
        QTemporaryDir root;
        QVERIFY(root.isValid());
        for (const auto &file : files) {
            const QString path = root.filePath(file.section(QLatin1Char('='), 0, 0));
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile output(path);
            QVERIFY(output.open(QIODevice::WriteOnly));
            output.write(file.section(QLatin1Char('='), 1).toUtf8());
        }
        QCOMPARE(int(HardwareService::microcodeBoot(root.path(), QStringLiteral("amd-ucode"))), expected);
    }
    void preflightFindsKernelsWithoutHeaders()
    {
        QTemporaryDir root;
        const auto touch = [&root](const QString &path) {
            QDir().mkpath(QFileInfo(root.filePath(path)).absolutePath());
            QFile file(root.filePath(path));
            QVERIFY(file.open(QIODevice::WriteOnly));
        };
        touch(QStringLiteral("usr/lib/modules/6.17.1-arch1-1/vmlinuz"));
        QDir().mkpath(root.filePath(QStringLiteral("usr/lib/modules/6.17.1-arch1-1/build")));
        touch(QStringLiteral("usr/lib/modules/6.17.1-2-cachyos/vmlinuz"));
        // Leftover of a removed kernel: no vmlinuz, never reported.
        QDir().mkpath(root.filePath(QStringLiteral("usr/lib/modules/6.12.0-old/extramodules")));
        QCOMPARE(Preflight::kernelsWithoutHeaders(root.path()), QStringList{QStringLiteral("6.17.1-2-cachyos")});
    }
    void preflightReadsSecureBootVariable()
    {
        QTemporaryDir root;
        const QString path = root.filePath(QStringLiteral("sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-e0e8ef33ca8c"));
        QVERIFY(!Preflight::secureBootEnabled(root.path()));
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray("\x06\x00\x00\x00\x01", 5));
        file.close();
        QVERIFY(Preflight::secureBootEnabled(root.path()));
    }
    void preflightCountsOnlyPackagesToInstall()
    {
        auto state = drivers();
        state.profiles[1].packages = {QStringLiteral("nvidia-dkms"), QStringLiteral("nvidia-utils")};
        HardwareProfile bt = bluetooth();
        bt.packages = {QStringLiteral("bluez"), QStringLiteral("bluez-utils")};
        bt.missingPackages = {QStringLiteral("bluez-utils")};
        state.profiles.append(bt);
        QCOMPARE(Preflight::packagesToInstall(state, {QStringLiteral("new")}, {bt.id}),
                 QStringList({QStringLiteral("nvidia-dkms"), QStringLiteral("nvidia-utils"), QStringLiteral("bluez-utils")}));
        QVERIFY(Preflight::needsDkms(state, {QStringLiteral("new")}));
        QVERIFY(!Preflight::needsDkms(state, {}));
    }
    void readyServiceCanBeStoppedExplicitly()
    {
        auto state = stoppedBluetooth();
        state.activeServices = state.enabledServices;
        HardwareService::updateSupportState(state.profiles[0], state);
        QVERIFY(state.profiles[0].ready);
        ProfileSelection selection;
        selection.refresh(state, false);
        // Unchecking a ready system-managed profile is not a way to disable it.
        QVERIFY(!selection.select(QStringLiteral("support:bluetooth"), false, state));
        QVERIFY(selection.stageServiceDisable(QStringLiteral("support:bluetooth"), true, state));
        QCOMPARE(selection.pendingCount(state), 1);
        selection.refresh(state, true);
        QCOMPARE(selection.disablingServices(state), QStringList{QStringLiteral("support:bluetooth")});

        HardwareApplyRequest request;
        QVERIFY(HardwareApplyRequestParser::read({QStringLiteral("apply"), QStringLiteral("--disable-services"),
                                                  QStringLiteral("support:bluetooth")}, request));
        QVERIFY(HardwareApplyRequestParser::validate(request, state));
        QStringList calls;
        const bool result = HardwareTransaction::apply(state, request,
            [&calls, &state](const QString &program, const QStringList &arguments) {
                calls.append(program + QLatin1Char(' ') + arguments.join(QLatin1Char(' ')));
                state.enabledServices.clear();
                state.activeServices.clear();
                HardwareService::updateSupportState(state.profiles[0], state);
                return true;
            }, [&state] { return state; });
        QVERIFY(result);
        QCOMPARE(calls, QStringList{QStringLiteral("/usr/bin/systemctl disable --now bluetooth.service")});
        // Packages stay installed.
        QVERIFY(state.installedPackages.contains(QStringLiteral("bluez")));
    }
    void stoppingServicesIsVerified()
    {
        auto state = stoppedBluetooth();
        state.activeServices = state.enabledServices;
        HardwareService::updateSupportState(state.profiles[0], state);
        HardwareApplyRequest request;
        request.disableServices = {QStringLiteral("support:bluetooth")};
        const bool result = HardwareTransaction::apply(state, request,
            [](const QString &, const QStringList &) { return true; }, [&state] { return state; });
        QVERIFY(!result);
    }
    void stoppingServicesNeedsReadyProfileWithServices()
    {
        auto state = stoppedBluetooth();  // service stopped: not ready
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(!selection.stageServiceDisable(QStringLiteral("support:bluetooth"), true, state));
        HardwareApplyRequest request;
        request.disableServices = {QStringLiteral("support:bluetooth")};
        QVERIFY(!HardwareApplyRequestParser::validate(request, state));
        state.activeServices = state.enabledServices;
        HardwareService::updateSupportState(state.profiles[0], state);
        state.profiles[0].services.clear();
        QVERIFY(!ProfileSelection::canDisableServices(state.profiles[0]));
        QVERIFY(!HardwareApplyRequestParser::validate(request, state));
    }
    void discardingClearsStagedServiceStops()
    {
        auto state = stoppedBluetooth();
        state.activeServices = state.enabledServices;
        HardwareService::updateSupportState(state.profiles[0], state);
        ProfileSelection selection;
        selection.refresh(state, false);
        QVERIFY(selection.stageServiceDisable(QStringLiteral("support:bluetooth"), true, state));
        selection.reset(state);
        QCOMPARE(selection.pendingCount(state), 0);
        QVERIFY(selection.stageServiceDisable(QStringLiteral("support:bluetooth"), true, state));
        selection.refresh(state, false);
        QCOMPARE(selection.pendingCount(state), 0);
    }
};
QTEST_GUILESS_MAIN(HardwareRegressionTest)
#include "HardwareRegressionTest.moc"
