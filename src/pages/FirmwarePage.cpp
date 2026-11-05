#include "FirmwarePage.h"
#include "widgets/HardwareUi.h"

#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

FirmwarePage::FirmwarePage(QWidget *parent, const QString &program) : QWidget(parent), program_(program)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 28, 30, 20);
    layout->addWidget(HardwareUi::heading(tr("Firmware updates"), this, 10));
    status_ = new QLabel(tr("Check for firmware updates to list supported devices."), this);
    status_->setWordWrap(true);
    layout->addWidget(status_);
    auto *actions = new QHBoxLayout;
    check_ = new QPushButton(tr("Check for updates"), this);
    check_->setObjectName(QStringLiteral("checkFirmware"));
    setup_ = new QPushButton(tr("Install firmware support"), this);
    update_ = new QPushButton(tr("Update selected device"), this);
    update_->setObjectName(QStringLiteral("updateFirmware"));
    update_->setEnabled(false);
    actions->addWidget(setup_);
    actions->addWidget(check_);
    actions->addWidget(update_);
    layout->addLayout(actions);
    devices_ = new QTreeWidget(this);
    devices_->setHeaderLabels({tr("Device"), tr("Current version"), tr("Available version"), tr("Update details")});
    layout->addWidget(devices_, 1);
    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(3000);
    layout->addWidget(log_);
    connect(setup_, &QPushButton::clicked, this, &FirmwarePage::refreshRequested);
    connect(check_, &QPushButton::clicked, this, [this] {
        hardware_ = {}; updates_ = {}; render(); log_->clear(); start(QStringLiteral("refresh"));
    });
    connect(devices_, &QTreeWidget::itemSelectionChanged, this, [this] {
        auto *item = devices_->currentItem();
        update_->setEnabled(available_ && !busy_ && item && item->data(0, Qt::UserRole + 1).toBool());
    });
    connect(update_, &QPushButton::clicked, this, [this] {
        auto *item = devices_->currentItem();
        if (!item || !item->data(0, Qt::UserRole + 1).toBool()) return;
        if (QMessageBox::question(this, tr("Firmware update"), tr("Apply the displayed update to %1? Keep power connected. Some updates are staged for the next restart.").arg(item->text(0))) != QMessageBox::Yes) return;
        log_->clear();
        start(QStringLiteral("update"), item->data(0, Qt::UserRole).toString());
    });
    process_ = new QProcess(this);
    connect(process_, &QProcess::readyReadStandardOutput, this, [this] {
        const auto bytes = process_->readAllStandardOutput();
        output_ += bytes;
        if (action_ == QLatin1String("update")) log_->insertPlainText(QString::fromUtf8(bytes));
    });
    connect(process_, &QProcess::readyReadStandardError, this, [this] {
        log_->insertPlainText(QString::fromUtf8(process_->readAllStandardError()));
    });
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        busy_ = false;
        status_->setText(tr("Could not start firmware manager: %1").arg(process_->errorString()));
        check_->setEnabled(available_);
    });
    connect(process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus state) {
        output_ += process_->readAllStandardOutput();
        log_->insertPlainText(QString::fromUtf8(process_->readAllStandardError()));
        busy_ = false;
        check_->setEnabled(available_);
        if (state != QProcess::NormalExit || (code != 0 && code != 2)) {
            log_->appendPlainText(QString::fromUtf8(output_));
            status_->setText(tr("Firmware operation failed. Review the details and try again."));
            return;
        }
        if (action_ == QLatin1String("refresh")) { start(QStringLiteral("get-devices")); return; }
        if (action_ == QLatin1String("update")) {
            status_->setText(tr("Firmware operation finished. Restart if requested by the device; check again to verify its version."));
            hardware_ = {}; updates_ = {}; render();
            return;
        }
        const auto document = QJsonDocument::fromJson(output_);
        // A no-op exit can legitimately contain no JSON when no devices/updates exist.
        if (code == 0 && (!document.isObject() || !document.object().value(QStringLiteral("Devices")).isArray())) {
            status_->setText(tr("Invalid firmware manager response."));
            return;
        }
        const auto values = document.object().value(QStringLiteral("Devices")).toArray();
        if (action_ == QLatin1String("get-devices")) {
            hardware_ = values;
            start(QStringLiteral("get-updates"));
        } else {
            updates_ = values;
            render();
            status_->setText(hardware_.isEmpty() ? tr("No firmware devices reported.")
                : tr("Check complete. Updates are available only for devices supported by your enabled firmware remotes."));
        }
    });
}

void FirmwarePage::updateState(const HardwareSnapshot &snapshot)
{
    available_ = snapshot.installedPackages.contains(QStringLiteral("fwupd"));
    setup_->setVisible(!available_);
    setup_->setEnabled(snapshot.isValid());
    check_->setEnabled(available_ && !busy_);
    if (!available_) status_->setText(tr("Install firmware support to discover devices and updates."));
}

void FirmwarePage::start(const QString &action, const QString &device)
{
    if (busy_) return;
    busy_ = true;
    action_ = action;
    output_.clear();
    check_->setEnabled(false);
    update_->setEnabled(false);
    status_->setText(tr("Firmware operation in progress…"));
    QStringList arguments{action, QStringLiteral("--no-unreported-check"),
        QStringLiteral("--no-remote-check"), QStringLiteral("--no-metadata-check")};
    if (action == QLatin1String("update")) {
        arguments << device << QStringLiteral("--assume-yes") << QStringLiteral("--no-reboot-check");
    } else if (action == QLatin1String("refresh")) {
        arguments << QStringLiteral("--force");
    } else arguments << QStringLiteral("--json");
    process_->start(program_, arguments);
    process_->closeWriteChannel();
}

void FirmwarePage::render()
{
    devices_->clear();
    update_->setEnabled(false);
    for (const auto &entry : hardware_) {
        const auto device = entry.toObject();
        const QString id = device.value(QStringLiteral("DeviceId")).toString();
        QJsonObject release;
        for (const auto &update : updates_) {
            const auto object = update.toObject();
            if (object.value(QStringLiteral("DeviceId")).toString() == id) {
                const auto releases = object.value(QStringLiteral("Releases")).toArray();
                if (!releases.isEmpty()) release = releases.first().toObject();
                break;
            }
        }
        auto *item = new QTreeWidgetItem(devices_, {device.value(QStringLiteral("Name")).toString(),
            device.value(QStringLiteral("Version")).toString(), release.value(QStringLiteral("Version")).toString(),
            release.value(QStringLiteral("Summary")).toString()});
        item->setToolTip(3, release.value(QStringLiteral("Description")).toString());
        item->setData(0, Qt::UserRole, id);
        item->setData(0, Qt::UserRole + 1, !id.isEmpty() && !release.isEmpty());
    }
    for (int column = 0; column < 3; ++column) devices_->resizeColumnToContents(column);
}
