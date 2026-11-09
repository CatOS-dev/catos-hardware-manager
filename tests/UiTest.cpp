// Offscreen UI tests. Rows and pages are fed fixture snapshots, so these
// run without udev, pacman or CHWD.
#include "widgets/HardwareUi.h"
#include "widgets/Presentation.h"

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
};

QTEST_MAIN(UiTest)
#include "UiTest.moc"
