#include "ThunderboltDialog.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {
const QString kService = QStringLiteral("org.freedesktop.bolt");
const QString kManagerPath = QStringLiteral("/org/freedesktop/bolt");
const QString kManager = QStringLiteral("org.freedesktop.bolt1.Manager");
const QString kDevice = QStringLiteral("org.freedesktop.bolt1.Device");

QVector<BoltDevice> listBoltDevices(QString *error)
{
    QVector<BoltDevice> devices;
    QDBusInterface manager(kService, kManagerPath, kManager, QDBusConnection::systemBus());
    manager.setTimeout(3000);
    const QDBusReply<QList<QDBusObjectPath>> reply = manager.call(QStringLiteral("ListDevices"));
    if (!reply.isValid()) {
        *error = reply.error().message();
        return devices;
    }
    for (const auto &path : reply.value()) {
        QDBusInterface device(kService, path.path(), kDevice, QDBusConnection::systemBus());
        device.setTimeout(3000);
        BoltDevice entry;
        entry.path = path.path();
        entry.uid = device.property("Uid").toString();
        entry.name = device.property("Name").toString();
        entry.vendor = device.property("Vendor").toString();
        entry.type = device.property("Type").toString();
        entry.status = device.property("Status").toString();
        entry.stored = device.property("Stored").toBool();
        // The host controller itself is never authorised by the user.
        if (entry.type != QLatin1String("host")) devices.append(entry);
    }
    return devices;
}
}

ThunderboltDialog::ThunderboltDialog(QWidget *parent, bool connectToBoltd) : QDialog(parent)
{
    setWindowTitle(tr("Thunderbolt devices"));
    resize(620, 360);
    auto *layout = new QVBoxLayout(this);
    message_ = new QLabel(this);
    message_->setWordWrap(true);
    layout->addWidget(message_);
    list_ = new QTreeWidget(this);
    list_->setRootIsDecorated(false);
    list_->setHeaderLabels({tr("Device"), tr("Vendor"), tr("Status")});
    list_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(list_, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    authorize_ = buttons->addButton(tr("Authorize"), QDialogButtonBox::ActionRole);
    forget_ = buttons->addButton(tr("Forget"), QDialogButtonBox::ActionRole);
    refresh_ = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
    connect(list_, &QTreeWidget::itemSelectionChanged, this, &ThunderboltDialog::updateButtons);
    connect(refresh_, &QPushButton::clicked, this, &ThunderboltDialog::reload);
    connect(authorize_, &QPushButton::clicked, this, [this] {
        const auto *device = current();
        if (!device) return;
        // A stored device only needs authorising; a new one is enrolled so
        // it is authorised automatically next time.
        if (device->stored)
            call(device->path, kDevice, QStringLiteral("Authorize"), {QString()});
        else
            call(kManagerPath, kManager, QStringLiteral("EnrollDevice"),
                 {device->uid, QStringLiteral("default"), QString()});
    });
    connect(forget_, &QPushButton::clicked, this, [this] {
        const auto *device = current();
        if (!device) return;
        if (QMessageBox::question(this, tr("Forget device"),
                tr("Forget %1? It will need to be authorized again the next time it is connected.")
                    .arg(device->name)) != QMessageBox::Yes)
            return;
        call(kManagerPath, kManager, QStringLiteral("ForgetDevice"), {device->uid});
    });
    if (connectToBoltd) reload();
    else updateButtons();
}

QString ThunderboltDialog::statusText(const BoltDevice &device)
{
    QString text;
    if (device.status == QLatin1String("authorized")) text = tr("Authorized");
    else if (device.status == QLatin1String("connected")) text = tr("Connected, not authorized");
    else if (device.status == QLatin1String("authorizing")) text = tr("Authorizing…");
    else if (device.status == QLatin1String("auth-error")) text = tr("Authorization failed");
    else if (device.status == QLatin1String("disconnected")) text = tr("Not connected");
    else text = device.status;
    if (device.stored) text += tr(" · remembered");
    return text;
}

void ThunderboltDialog::setDevices(const QVector<BoltDevice> &devices, const QString &error)
{
    devices_ = devices;
    list_->clear();
    for (const auto &device : devices_) {
        auto *item = new QTreeWidgetItem(list_, {device.name, device.vendor, statusText(device)});
        item->setData(0, Qt::UserRole, device.uid);
    }
    if (!error.isEmpty())
        message_->setText(tr("Cannot reach the Thunderbolt service (boltd): %1").arg(error));
    else if (devices_.isEmpty())
        message_->setText(tr("No Thunderbolt or USB4 peripherals are known."));
    else
        message_->setText(tr("Authorize a peripheral to allow it access to the computer. Only authorize devices you trust."));
    if (list_->topLevelItemCount() > 0) list_->setCurrentItem(list_->topLevelItem(0));
    updateButtons();
}

const BoltDevice *ThunderboltDialog::current() const
{
    const auto *item = list_->currentItem();
    if (!item) return nullptr;
    const QString uid = item->data(0, Qt::UserRole).toString();
    for (const auto &device : devices_)
        if (device.uid == uid) return &device;
    return nullptr;
}

void ThunderboltDialog::updateButtons()
{
    const auto *device = current();
    const bool connected = device && device->status != QLatin1String("disconnected");
    authorize_->setEnabled(!busy_ && connected && device->status != QLatin1String("authorized")
                           && device->status != QLatin1String("authorizing"));
    forget_->setEnabled(!busy_ && device && device->stored);
    refresh_->setEnabled(!busy_);
}

void ThunderboltDialog::reload()
{
    QString error;
    const auto devices = listBoltDevices(&error);
    setDevices(devices, error);
}

void ThunderboltDialog::call(const QString &path, const QString &interface, const QString &method,
                             const QVariantList &arguments)
{
    QDBusMessage message = QDBusMessage::createMethodCall(kService, path, interface, method);
    message.setArguments(arguments);
    // Authorisation waits for the polkit dialog, so never block the UI.
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message, 120000), this);
    busy_ = true;
    updateButtons();
    message_->setText(tr("Waiting for the Thunderbolt service…"));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();
        busy_ = false;
        const QDBusPendingReply<> reply = *call;
        reload();
        if (reply.isError())
            QMessageBox::warning(this, tr("Thunderbolt devices"),
                                 tr("The operation failed: %1").arg(reply.error().message()));
    });
}
