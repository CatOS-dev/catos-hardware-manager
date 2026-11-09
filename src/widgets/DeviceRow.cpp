#include "DeviceRow.h"

#include "HardwareUi.h"
#include "Presentation.h"
#include "ProfileRow.h"
#include "dialogs/ThunderboltDialog.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
struct DeviceTool {
    QString program;
    QStringList arguments;
    QString label;
};

// A System Settings module is only useful when its plugin is installed;
// kcmshell6 alone opens an error window otherwise.
bool kcmInstalled(const QString &module)
{
    for (const auto &root : QCoreApplication::libraryPaths()) {
        for (const auto &group : {QStringLiteral("systemsettings"), QStringLiteral("systemsettings_qwidgets")}) {
            if (QFileInfo::exists(root + QStringLiteral("/plasma/kcms/") + group + QLatin1Char('/') + module + QStringLiteral(".so")))
                return true;
        }
    }
    return false;
}

DeviceTool settingsModule(const QString &module, const QString &label)
{
    const QString kcmshell = QStandardPaths::findExecutable(QStringLiteral("kcmshell6"));
    if (kcmshell.isEmpty() || !kcmInstalled(module)) return {};
    return {kcmshell, {module}, label};
}

DeviceTool application(const QStringList &candidates, const QString &label)
{
    for (const auto &candidate : candidates) {
        const QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty()) return {path, {}, label};
    }
    return {};
}

DeviceTool deviceTool(const HardwareDevice &device, const HardwareSnapshot &snapshot)
{
    switch (device.category) {
    case DeviceCategory::Audio:
        return settingsModule(QStringLiteral("kcm_pulseaudio"), DeviceRow::tr("Open sound settings"));
    case DeviceCategory::Bluetooth:
        return settingsModule(QStringLiteral("kcm_bluetooth"), DeviceRow::tr("Open Bluetooth settings"));
    case DeviceCategory::Printer:
        return settingsModule(QStringLiteral("kcm_printer_manager"), DeviceRow::tr("Open printer settings and test printing"));
    case DeviceCategory::Camera:
        // qv4l2 is a developer tool; prefer the KDE camera app.
        return application({QStringLiteral("kamoso"), QStringLiteral("qv4l2")}, DeviceRow::tr("Open camera test tool"));
    case DeviceCategory::Scanner:
        return application({QStringLiteral("skanpage"), QStringLiteral("simple-scan")}, DeviceRow::tr("Open document scanner"));
    case DeviceCategory::Fingerprint: {
        // Enrolling needs fprintd; offer it once the support profile is ready.
        const auto *profile = snapshot.profile(QStringLiteral("support:fingerprint"));
        if (!profile || !profile->ready || !device.profileIds.contains(profile->id)) return {};
        return settingsModule(QStringLiteral("kcm_users"), DeviceRow::tr("Enroll fingerprints"));
    }
    case DeviceCategory::Power:
        return settingsModule(QStringLiteral("kcm_powerdevilprofilesconfig"), DeviceRow::tr("Open power settings"));
    default:
        return {};
    }
}
}

DeviceRow::DeviceRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                     const ProfileSelection &selection, bool expanded, QWidget *parent)
    : QWidget(parent), deviceId_(device.id)
{
    setProperty("rowKey", device.id);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    header_ = new RowButton(this);
    header_->setMinimumHeight(84);
    auto *line = new QHBoxLayout(header_);
    line->setContentsMargins(16, 12, 14, 12);
    line->setSpacing(17);
    icon_ = new SymbolLabel(categoryIcon(device.category), 30, header_);
    line->addWidget(icon_, 0, Qt::AlignTop);
    auto *text = new QVBoxLayout;
    text->setSpacing(4);
    name_ = HardwareUi::heading(QString(), header_, 2);
    name_->setWordWrap(true);
    text->addWidget(name_);
    status_ = new StatusLabel(header_);
    text->addWidget(status_);
    summary_ = HardwareUi::muted(QString(), header_, true);
    text->addWidget(summary_);
    line->addLayout(text, 1);
    arrow_ = new SymbolLabel(QStringLiteral("chevron-right"), 22, header_);
    line->addWidget(arrow_);
    layout->addWidget(header_);
    // Labels inside the button would swallow its clicks.
    for (auto *child : header_->findChildren<QWidget *>())
        child->setAttribute(Qt::WA_TransparentForMouseEvents);

    detail_ = new QWidget(this);
    detail_->setBackgroundRole(QPalette::AlternateBase);
    detail_->setAutoFillBackground(true);
    auto *content = new QVBoxLayout(detail_);
    content->setContentsMargins(65, 12, 22, 17);
    content->setSpacing(2);
    identifiers_ = HardwareUi::muted(QString(), detail_, true);
    identifiers_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    content->addWidget(identifiers_);
    untested_ = HardwareUi::muted(tr("The driver is loaded; the device itself has not been tested."), detail_, true);
    content->addWidget(untested_);
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (!profile) continue;
        auto *row = new ProfileRow(*profile, selection, device.recommendedProfile == id, detail_);
        profiles_.insert(id, row);
        connect(row, &ProfileRow::profileToggled, this, &DeviceRow::profileToggled);
        connect(row, &ProfileRow::serviceStopToggled, this, &DeviceRow::serviceStopToggled);
        content->addWidget(row);
    }
    if (profiles_.isEmpty())
        content->addWidget(HardwareUi::muted(tr("No additional profile"), detail_));

    if (device.capabilities.contains(QStringLiteral("dual-gpu"))) {
        const auto *switcheroo = snapshot.profile(QStringLiteral("support:switcheroo-control"));
        if (switcheroo && switcheroo->ready)
            content->addWidget(HardwareUi::muted(tr("To run an application on the other GPU, choose "
                "\"Run using dedicated graphics card\" from its context menu in the launcher."), detail_, true));
    }
    if (device.category == DeviceCategory::Thunderbolt) {
        const auto *bolt = snapshot.profile(QStringLiteral("support:thunderbolt"));
        if (bolt && bolt->ready) {
            auto *manage = new QPushButton(tr("Manage Thunderbolt devices"), detail_);
            connect(manage, &QPushButton::clicked, this, [this] {
                ThunderboltDialog dialog(this);
                dialog.exec();
            });
            content->addSpacing(6);
            content->addWidget(manage, 0, Qt::AlignLeft);
        }
    }
    const DeviceTool tool = deviceTool(device, snapshot);
    if (!tool.program.isEmpty()) {
        auto *button = new QPushButton(tool.label, detail_);
        connect(button, &QPushButton::clicked, this, [this, tool] {
            if (!QProcess::startDetached(tool.program, tool.arguments))
                QMessageBox::warning(this, tr("Hardware Manager"), tr("Could not open the device tool."));
        });
        content->addSpacing(6);
        content->addWidget(button, 0, Qt::AlignLeft);
    }
    layout->addWidget(detail_);
    connect(header_, &QPushButton::toggled, this, [this](bool value) {
        setExpanded(value);
        emit expansionChanged(deviceId_, value);
    });
    header_->setChecked(expanded);
    setExpanded(expanded);
    updateRow(device, snapshot, selection);
}

void DeviceRow::setExpanded(bool expanded)
{
    detail_->setVisible(expanded);
    arrow_->setSymbol(expanded ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"));
}

void DeviceRow::updateRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                          const ProfileSelection &selection)
{
    if (device.id != deviceId_) return;
    name_->setText(device.name);
    const StatusText status = Presentation::deviceStatus(device, snapshot, selection);
    status_->setStatus(status);
    const QString summary = Presentation::deviceSummary(device, snapshot);
    summary_->setText(summary);
    summary_->setVisible(!summary.isEmpty());
    const QString identifiers = Presentation::deviceIdentifiers(device);
    identifiers_->setText(identifiers);
    identifiers_->setVisible(!identifiers.isEmpty());
    untested_->setVisible(!device.driver.isEmpty() && status.level == StatusLevel::Ok);
    header_->setAccessibleName(device.name);
    header_->setAccessibleDescription(summary.isEmpty() ? status.text
                                                        : status.text + QStringLiteral(". ") + summary);
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (auto *row = profiles_.value(id); row && profile)
            row->updateRow(*profile, selection, device.recommendedProfile == id);
    }
}

QString DeviceRow::summaryText() const
{
    return summary_->text();
}
