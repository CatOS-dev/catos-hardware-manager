#include "DeviceRow.h"

#include "HardwareUi.h"
#include "Presentation.h"
#include "ProfileRow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

DeviceRow::DeviceRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                     const ProfileSelection &selection, bool expanded, QWidget *parent)
    : QWidget(parent), deviceId_(device.id)
{
    setProperty("rowKey", device.id);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    header_ = new QPushButton(this);
    header_->setFlat(true);
    header_->setCheckable(true);
    header_->setCursor(Qt::PointingHandCursor);
    header_->setMinimumHeight(84);
    header_->setStyleSheet(QStringLiteral(
        "QPushButton, QPushButton:checked { border: 0; background: transparent; border-radius: 10px; }"
        "QPushButton:hover { background: palette(alternate-base); }"
        "QPushButton:focus { border: 1px solid palette(highlight); }"));
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
    auto p = detail_->palette();
    p.setColor(QPalette::Window, p.color(QPalette::AlternateBase));
    detail_->setPalette(p);
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
        content->addWidget(row);
    }
    if (profiles_.isEmpty())
        content->addWidget(HardwareUi::muted(tr("No additional profile"), detail_));

    QString program;
    QStringList arguments;
    QString label;
    switch (device.category) {
    case DeviceCategory::Audio:
        program = QStringLiteral("kcmshell6"); arguments = {QStringLiteral("kcm_pulseaudio")}; label = tr("Open sound settings"); break;
    case DeviceCategory::Bluetooth:
        program = QStringLiteral("kcmshell6"); arguments = {QStringLiteral("kcm_bluetooth")}; label = tr("Open Bluetooth settings"); break;
    case DeviceCategory::Printer:
        program = QStringLiteral("kcmshell6"); arguments = {QStringLiteral("kcm_printer_manager")}; label = tr("Open printer settings and test printing"); break;
    case DeviceCategory::Camera:
        program = QStringLiteral("qv4l2"); label = tr("Open camera test tool"); break;
    case DeviceCategory::Scanner:
        program = QStringLiteral("simple-scan"); label = tr("Open document scanner"); break;
    default: break;
    }
    const QString executable = program.isEmpty() ? QString() : QStandardPaths::findExecutable(program);
    if (!executable.isEmpty()) {
        auto *test = new QPushButton(label, detail_);
        connect(test, &QPushButton::clicked, this, [this, executable, arguments] {
            if (!QProcess::startDetached(executable, arguments))
                QMessageBox::warning(this, tr("Hardware Manager"), tr("Could not open the device tool."));
        });
        content->addSpacing(6);
        content->addWidget(test, 0, Qt::AlignLeft);
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
