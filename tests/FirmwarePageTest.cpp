#include "pages/FirmwarePage.h"
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest>

class FirmwarePageTest : public QObject {
    Q_OBJECT
private slots:
    void checkUpdates_data()
    {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<bool>("available");
        QTest::newRow("available") << QStringLiteral("available") << true;
        QTest::newRow("no-updates") << QStringLiteral("empty") << false;
    }
    void checkUpdates()
    {
        QFETCH(QString, mode);
        QFETCH(bool, available);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath(QStringLiteral("commands"));
        qputenv("CATOS_FIRMWARE_TEST_RECORD", path.toUtf8());
        qputenv("CATOS_FIRMWARE_TEST_MODE", mode.toUtf8());
        FirmwarePage page(nullptr, QStringLiteral(FAKE_FIRMWARE_PROGRAM));
        HardwareSnapshot state;
        state.installedPackages.insert(QStringLiteral("fwupd"));
        page.updateState(state);
        auto *check = page.findChild<QPushButton *>(QStringLiteral("checkFirmware"));
        QCOMPARE(check->text(), QStringLiteral("Check for updates"));
        check->click();
        QTRY_VERIFY_WITH_TIMEOUT(!page.isBusy(), 5000);
        auto *tree = page.findChild<QTreeWidget *>();
        QCOMPARE(tree->topLevelItemCount(), 1);
        QCOMPARE(tree->topLevelItem(0)->text(0), QStringLiteral("Test device"));
        QCOMPARE(tree->topLevelItem(0)->text(1), QStringLiteral("1.0"));
        QCOMPARE(tree->topLevelItem(0)->text(2), available ? QStringLiteral("2.0") : QString());
        tree->setCurrentItem(tree->topLevelItem(0));
        QCOMPARE(page.findChild<QPushButton *>(QStringLiteral("updateFirmware"))->isEnabled(), available);
        state.installedPackages.clear();
        page.updateState(state);
        QVERIFY(!check->isEnabled());
        QVERIFY(!page.findChild<QPushButton *>(QStringLiteral("updateFirmware"))->isEnabled());
        state.installedPackages.insert(QStringLiteral("fwupd"));
        page.updateState(state);
        QVERIFY(check->isEnabled());
        QVERIFY(!page.findChild<QPushButton *>(QStringLiteral("updateFirmware"))->isEnabled());
        QCOMPARE(tree->topLevelItemCount(), 0);
        QFile record(path);
        QVERIFY(record.open(QIODevice::ReadOnly));
        const auto commands = record.readAll();
        QVERIFY(commands.contains("refresh"));
        QVERIFY(commands.contains("get-devices"));
        QVERIFY(commands.contains("get-updates"));
        QVERIFY(commands.contains("--no-unreported-check"));
        QCOMPARE(commands.count('\n'), 3);
    }
    void losingSupportDuringConfirmationDoesNotStartUpdate()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath(QStringLiteral("commands"));
        qputenv("CATOS_FIRMWARE_TEST_RECORD", path.toUtf8());
        qputenv("CATOS_FIRMWARE_TEST_MODE", "available");
        FirmwarePage page(nullptr, QStringLiteral(FAKE_FIRMWARE_PROGRAM));
        HardwareSnapshot state;
        state.installedPackages.insert(QStringLiteral("fwupd"));
        page.updateState(state);
        page.findChild<QPushButton *>(QStringLiteral("checkFirmware"))->click();
        QTRY_VERIFY_WITH_TIMEOUT(!page.isBusy(), 5000);
        auto *tree = page.findChild<QTreeWidget *>();
        QCOMPARE(tree->topLevelItemCount(), 1);
        tree->setCurrentItem(tree->topLevelItem(0));
        auto *update = page.findChild<QPushButton *>(QStringLiteral("updateFirmware"));
        QVERIFY(update->isEnabled());
        QTimer::singleShot(0, &page, [&page, &state] {
            state.installedPackages.clear();
            page.updateState(state);
            page.findChild<QMessageBox *>()->done(QMessageBox::Yes);
        });
        update->click();
        QVERIFY(!page.isBusy());
        QVERIFY(!update->isEnabled());
        QFile record(path);
        QVERIFY(record.open(QIODevice::ReadOnly));
        const auto commands = record.readAll();
        QCOMPARE(commands.count('\n'), 3);
        QVERIFY(!commands.contains("update --"));
    }
    void invalidReplyDoesNotEnableUpdate()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("CATOS_FIRMWARE_TEST_RECORD", directory.filePath(QStringLiteral("commands")).toUtf8());
        qputenv("CATOS_FIRMWARE_TEST_MODE", "invalid");
        FirmwarePage page(nullptr, QStringLiteral(FAKE_FIRMWARE_PROGRAM));
        HardwareSnapshot state;
        state.installedPackages.insert(QStringLiteral("fwupd"));
        page.updateState(state);
        page.findChild<QPushButton *>(QStringLiteral("checkFirmware"))->click();
        QTRY_VERIFY_WITH_TIMEOUT(!page.isBusy(), 5000);
        QVERIFY(!page.findChild<QPushButton *>(QStringLiteral("updateFirmware"))->isEnabled());
        const auto labels = page.findChildren<QLabel *>();
        QStringList messages;
        for (const auto *label : labels) messages.append(label->text());
        QVERIFY(messages.contains(QStringLiteral("Invalid firmware manager response.")));
    }
};
QTEST_MAIN(FirmwarePageTest)
#include "FirmwarePageTest.moc"
