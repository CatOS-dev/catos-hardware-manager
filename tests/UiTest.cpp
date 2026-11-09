// Offscreen UI tests. Rows and pages are fed fixture snapshots, so these
// run without udev, pacman or CHWD.
#include "pages/DevicesPage.h"
#include "pages/ProfileCatalogPage.h"
#include "widgets/DeviceRow.h"
#include "widgets/HardwareUi.h"
#include "widgets/Presentation.h"
#include "widgets/ProfileRow.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPointer>

#include <QApplication>
#include <QtTest>

namespace {
HardwareProfile support(const QString &name, bool recommended, bool ready)
{
    HardwareProfile profile;
    profile.id = QStringLiteral("support:") + name;
    profile.name = name;
    profile.source = ProfileSource::Support;
    profile.packages = {name};
    profile.applicable = true;
    profile.recommended = recommended;
    profile.ready = ready;
    profile.installed = ready;
    profile.systemManaged = true;
    if (!ready) profile.missingPackages = {name};
    return profile;
}

HardwareProfile chwd(const QString &name, bool installed)
{
    HardwareProfile profile;
    profile.id = QStringLiteral("chwd:") + name;
    profile.chwdName = name;
    profile.name = name;
    profile.source = ProfileSource::Chwd;
    profile.applicable = true;
    profile.installed = installed;
    profile.conflictGroup = QStringLiteral("gpu");
    profile.systemManaged = true;
    return profile;
}

HardwareDevice device(const QString &id, DeviceCategory category, const QString &driver)
{
    HardwareDevice result;
    result.id = id;
    result.name = id;
    result.category = category;
    result.driver = driver;
    result.modalias = QStringLiteral("pci:v0000");
    return result;
}

// A laptop: hybrid graphics with CHWD drivers, Bluetooth that misses its
// recommended support package, and an optional camera profile.
HardwareSnapshot laptop()
{
    HardwareSnapshot snapshot;
    snapshot.chwdAvailable = true;
    snapshot.profiles = {chwd(QStringLiteral("nvidia-open-dkms"), true), chwd(QStringLiteral("nvidia-dkms"), false),
                         support(QStringLiteral("bluez"), true, false), support(QStringLiteral("camera"), false, false)};
    auto gpu = device(QStringLiteral("pci:0000:01:00.0"), DeviceCategory::Graphics, QStringLiteral("nvidia"));
    gpu.name = QStringLiteral("GeForce RTX 4060");
    gpu.profileIds = {QStringLiteral("chwd:nvidia-open-dkms"), QStringLiteral("chwd:nvidia-dkms")};
    gpu.recommendedProfile = QStringLiteral("chwd:nvidia-open-dkms");
    auto bt = device(QStringLiteral("usb:3-1"), DeviceCategory::Bluetooth, QStringLiteral("btusb"));
    bt.name = QStringLiteral("AX211 Bluetooth");
    bt.profileIds = {QStringLiteral("support:bluez")};
    auto cam = device(QStringLiteral("usb:3-4"), DeviceCategory::Camera, QStringLiteral("uvcvideo"));
    cam.profileIds = {QStringLiteral("support:camera")};
    snapshot.devices = {gpu, bt, cam};
    for (auto &profile : snapshot.profiles) profile.category = profile.source == ProfileSource::Chwd
        ? DeviceCategory::Graphics : profile.name == QLatin1String("bluez") ? DeviceCategory::Bluetooth : DeviceCategory::Camera;
    return snapshot;
}

int visibleLabelsWithText(QWidget *root, const QString &text)
{
    int count = 0;
    for (auto *label : root->findChildren<QLabel *>())
        if (!label->isHidden() && label->text() == text) ++count;
    return count;
}
}

class UiTest : public QObject {
    Q_OBJECT
private slots:
    void boundDeviceIsNormal()
    {
        HardwareSnapshot snapshot;
        ProfileSelection selection;
        const auto status = Presentation::deviceStatus(device(QStringLiteral("gpu"), DeviceCategory::Graphics,
                                                              QStringLiteral("amdgpu")), snapshot, selection);
        QCOMPARE(status.level, StatusLevel::Ok);
    }

    void missingDriverIsErrorOnlyWhenOneIsExpected()
    {
        HardwareSnapshot snapshot;
        ProfileSelection selection;
        QCOMPARE(Presentation::deviceStatus(device(QStringLiteral("gpu"), DeviceCategory::Graphics, {}),
                                            snapshot, selection).level, StatusLevel::Error);
        // libfprint and pcscd drive these from userspace.
        QCOMPARE(Presentation::deviceStatus(device(QStringLiteral("reader"), DeviceCategory::Fingerprint, {}),
                                            snapshot, selection).level, StatusLevel::Ok);
        QCOMPARE(Presentation::deviceStatus(device(QStringLiteral("card"), DeviceCategory::SmartCard, {}),
                                            snapshot, selection).level, StatusLevel::Ok);
        auto synthetic = device(QStringLiteral("system:cpu"), DeviceCategory::Cpu, {});
        QCOMPARE(Presentation::deviceStatus(synthetic, snapshot, selection).level, StatusLevel::Ok);
    }

    void onlyRecommendedSupportNeedsAttention()
    {
        HardwareSnapshot snapshot;
        snapshot.profiles = {support(QStringLiteral("audio"), false, false)};
        auto audio = device(QStringLiteral("hda"), DeviceCategory::Audio, QStringLiteral("snd_hda_intel"));
        audio.profileIds = {QStringLiteral("support:audio")};
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        QCOMPARE(Presentation::deviceStatus(audio, snapshot, selection).level, StatusLevel::Ok);
        snapshot.profiles[0].recommended = true;
        QCOMPARE(Presentation::deviceStatus(audio, snapshot, selection).level, StatusLevel::Attention);
    }

    void stagedChangesShowAsPending()
    {
        HardwareSnapshot snapshot;
        snapshot.profiles = {chwd(QStringLiteral("old"), true), chwd(QStringLiteral("new"), false)};
        auto gpu = device(QStringLiteral("gpu"), DeviceCategory::Graphics, QStringLiteral("nvidia"));
        gpu.profileIds = {QStringLiteral("chwd:old"), QStringLiteral("chwd:new")};
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        QCOMPARE(Presentation::deviceStatus(gpu, snapshot, selection).level, StatusLevel::Ok);
        QVERIFY(selection.select(QStringLiteral("chwd:new"), true, snapshot));
        QCOMPARE(Presentation::deviceStatus(gpu, snapshot, selection).level, StatusLevel::Pending);
        QCOMPARE(Presentation::profileState(snapshot.profiles[0], selection).text, QStringLiteral("Will be removed"));
        QCOMPARE(Presentation::profileState(snapshot.profiles[1], selection).text, QStringLiteral("Will be installed"));
    }

    void conflictingProfileIsManagedElsewhere()
    {
        auto power = support(QStringLiteral("power-profiles-daemon"), true, false);
        power.conflictingPackages = {QStringLiteral("tlp")};
        ProfileSelection selection;
        QCOMPARE(Presentation::profileState(power, selection).text, QStringLiteral("Managed by tlp"));
        QVERIFY(!Presentation::profileToggleable(power));
    }

    void statusColoursMeetContrastInBothThemes()
    {
        for (const bool dark : {false, true}) {
            QPalette palette;
            palette.setColor(QPalette::Window, dark ? QColor(0x202326) : QColor(0xeff0f1));
            palette.setColor(QPalette::WindowText, dark ? QColor(0xfcfcfc) : QColor(0x232627));
            for (const auto level : {StatusLevel::Neutral, StatusLevel::Ok, StatusLevel::Pending,
                                     StatusLevel::Attention, StatusLevel::Error}) {
                const QColor fg = HardwareUi::statusColor(level, palette);
                const auto lum = [](const QColor &c) {
                    const auto ch = [](qreal v) { return v <= 0.03928 ? v / 12.92 : qPow((v + 0.055) / 1.055, 2.4); };
                    return 0.2126 * ch(c.redF()) + 0.7152 * ch(c.greenF()) + 0.0722 * ch(c.blueF());
                };
                const qreal a = lum(fg), b = lum(palette.color(QPalette::Window));
                QVERIFY2((qMax(a, b) + 0.05) / (qMin(a, b) + 0.05) >= 4.5,
                         qPrintable(QStringLiteral("level %1 dark %2").arg(int(level)).arg(dark)));
            }
        }
    }

    void symbolsRenderAtDevicePixelRatio()
    {
        const auto pixmap = HardwareUi::symbol(QStringLiteral("bluetooth"), QColor(Qt::black), 24, 2.0);
        QCOMPARE(pixmap.size(), QSize(48, 48));
        QCOMPARE(pixmap.deviceIndependentSize(), QSizeF(24, 24));
    }

    void bannerSummarisesNotices()
    {
        NoticeBanner banner;
        banner.setNotices({}, {});
        QVERIFY(banner.isHidden());
        banner.setNotices({}, {QStringLiteral("first"), QStringLiteral("second")});
        QVERIFY(!banner.isHidden());
        QCOMPARE(banner.notices().size(), 2);
        banner.setNotices(QStringLiteral("broken"), {});
        QCOMPARE(banner.notices().first(), QStringLiteral("Hardware query failed: broken"));
    }

    void profileRowShowsStateOnceAndUpdatesInPlace()
    {
        auto snapshot = laptop();
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        const auto &bluez = *snapshot.profile(QStringLiteral("support:bluez"));
        ProfileRow row(bluez, selection, false);
        QCOMPARE(visibleLabelsWithText(&row, QStringLiteral("Missing components")), 1);
        auto *toggle = row.toggle();
        QVERIFY(!toggle->isChecked());
        QVERIFY(selection.select(bluez.id, true, snapshot));
        row.updateRow(bluez, selection, false);
        QCOMPARE(row.toggle(), toggle);
        QVERIFY(toggle->isChecked());
        QCOMPARE(row.stateText(), QStringLiteral("Will be installed"));
        QCOMPARE(visibleLabelsWithText(&row, QStringLiteral("Missing components")), 0);
    }

    void deviceRowKeepsSummaryAfterUpdate()
    {
        auto snapshot = laptop();
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        DeviceRow row(snapshot.devices[0], snapshot, selection, true);
        const QString summary = row.summaryText();
        QVERIFY(summary.contains(QStringLiteral("Driver: nvidia")));
        QVERIFY(summary.contains(QStringLiteral("CHWD: nvidia-open-dkms")));
        QVERIFY(selection.select(QStringLiteral("chwd:nvidia-dkms"), true, snapshot));
        row.updateRow(snapshot.devices[0], snapshot, selection);
        QCOMPARE(row.summaryText(), summary);
        QCOMPARE(row.status()->status().level, StatusLevel::Pending);
        QVERIFY(row.header()->isCheckable());
        QVERIFY(row.header()->isChecked());
    }

    void devicesPageUpdatesRowsWithoutRebuilding()
    {
        auto snapshot = laptop();
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        DevicesPage page;
        page.resize(900, 900);
        page.show();
        page.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&page));
        page.updateState(snapshot, selection);
        auto *bt = page.row(QStringLiteral("usb:3-1"));
        QVERIFY(bt);
        QCOMPARE(bt->status()->status().level, StatusLevel::Attention);
        bt->header()->setFocus();
        QCOMPARE(QApplication::focusWidget(), bt->header());
        QVERIFY(selection.select(QStringLiteral("support:bluez"), true, snapshot));
        page.updateState(snapshot, selection);
        QCOMPARE(page.row(QStringLiteral("usb:3-1")), bt);
        QCOMPARE(bt->status()->status().level, StatusLevel::Pending);
        // A new device changes the structure; focus follows the row id.
        // A rebuilt row may reuse the old address, so watch the old object.
        const QPointer<DeviceRow> old(bt);
        snapshot.devices.append(device(QStringLiteral("usb:9-9"), DeviceCategory::Controller, QStringLiteral("xpad")));
        page.updateState(snapshot, selection);
        QVERIFY(old.isNull());
        QCOMPARE(QApplication::focusWidget(), page.row(QStringLiteral("usb:3-1"))->header());
    }

    void rejectedToggleIsRevertedByResync()
    {
        auto snapshot = laptop();
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        ProfileCatalogPage page;
        page.updateState(snapshot, selection);
        auto *row = page.row(QStringLiteral("support:bluez"));
        QVERIFY(row);
        row->toggle()->setChecked(true);  // user click, but the window rejects it
        page.updateState(snapshot, selection);
        QVERIFY(!row->toggle()->isChecked());
    }

    void catalogSearchKeepsRowsForUnchangedFilter()
    {
        auto snapshot = laptop();
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        ProfileCatalogPage page;
        page.updateState(snapshot, selection);
        page.search()->setText(QStringLiteral("bluez"));
        QVERIFY(page.row(QStringLiteral("support:bluez")));
        QVERIFY(!page.row(QStringLiteral("support:camera")));
        auto *row = page.row(QStringLiteral("support:bluez"));
        page.updateState(snapshot, selection);
        QCOMPARE(page.row(QStringLiteral("support:bluez")), row);
    }

    void headersFollowApplicationPaletteChanges()
    {
        auto snapshot = laptop();
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        DevicesPage page;
        page.show();
        page.updateState(snapshot, selection);
        const QPalette original = QApplication::palette();
        QPalette dark = original;
        dark.setColor(QPalette::Window, QColor(0x202326));
        dark.setColor(QPalette::WindowText, QColor(0xfcfcfc));
        dark.setColor(QPalette::AlternateBase, QColor(0x292c30));
        QApplication::setPalette(dark);
        QCoreApplication::processEvents();
        auto *row = page.row(QStringLiteral("pci:0000:01:00.0"));
        QVERIFY(row);
        for (auto *label : row->header()->findChildren<QLabel *>()) {
            if (label->text() == QLatin1String("GeForce RTX 4060"))
                QCOMPARE(label->palette().color(QPalette::WindowText), QColor(0xfcfcfc));
        }
        QCOMPARE(row->findChildren<QWidget *>().constLast()->palette().color(QPalette::AlternateBase), QColor(0x292c30));
        QApplication::setPalette(original);
    }

    void catalogSearchMatchesDescriptionsAndPackages()
    {
        auto snapshot = laptop();
        snapshot.profiles[2].description = QStringLiteral("BlueZ stack and service");
        snapshot.profiles[2].packages = {QStringLiteral("bluez"), QStringLiteral("bluez-utils")};
        snapshot.profiles[3].chineseDescription = QStringLiteral("摄像头工具");
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        ProfileCatalogPage page;
        page.updateState(snapshot, selection);
        page.search()->setText(QStringLiteral("bluez-utils"));
        QVERIFY(page.row(QStringLiteral("support:bluez")));
        page.search()->setText(QStringLiteral("stack"));
        QVERIFY(page.row(QStringLiteral("support:bluez")));
        page.search()->setText(QStringLiteral("摄像头"));
        QVERIFY(page.row(QStringLiteral("support:camera")));
        QVERIFY(!page.row(QStringLiteral("support:bluez")));
    }

    void devicesPageHasNoStandingScannerButton()
    {
        auto snapshot = laptop();
        auto scanner = support(QStringLiteral("scanner"), false, false);
        scanner.id = QStringLiteral("support:scanner");
        snapshot.profiles.append(scanner);
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        DevicesPage page;
        page.updateState(snapshot, selection);
        for (auto *button : page.findChildren<QPushButton *>())
            QVERIFY(button->text() != QLatin1String("Set up scanner detection"));
    }

    void readyServiceProfileOffersExplicitStop()
    {
        auto snapshot = laptop();
        auto &bluez = snapshot.profiles[2];
        bluez.ready = bluez.installed = true;
        bluez.missingPackages.clear();
        bluez.services = {QStringLiteral("bluetooth.service")};
        ProfileSelection selection;
        selection.refresh(snapshot, false);
        ProfileRow row(bluez, selection, false);
        QVERIFY(row.toggle()->isHidden());  // a ready system profile is never unchecked
        QVERIFY(!row.stopButton()->isHidden());
        QSignalSpy spy(&row, &ProfileRow::serviceStopToggled);
        row.stopButton()->click();
        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.first().at(1).toBool(), true);
        QVERIFY(selection.stageServiceDisable(bluez.id, true, snapshot));
        row.updateRow(bluez, selection, false);
        QCOMPARE(row.stateText(), QStringLiteral("Services will be stopped"));
        row.stopButton()->click();
        QCOMPARE(spy.last().at(1).toBool(), false);
        // Profiles without services have nothing to stop.
        ProfileRow camera(snapshot.profiles[3], selection, false);
        QVERIFY(camera.stopButton()->isHidden());
    }
};

QTEST_MAIN(UiTest)
#include "UiTest.moc"
