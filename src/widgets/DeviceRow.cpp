#include "DeviceRow.h"

#include "HardwareUi.h"
#include "ProfileRow.h"

#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QMessageBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DeviceRow::DeviceRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                     const ProfileSelection &selection, bool expanded, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto *button = new QPushButton(this);
    button->setFlat(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setAccessibleName(device.name);
    button->setMinimumHeight(92);
    button->setStyleSheet(QStringLiteral(
        "QPushButton { border: 0; background: transparent; border-radius: 10px; }"
        "QPushButton:hover { background: palette(alternate-base); }"
        "QPushButton:focus { border: 1px solid palette(highlight); }"));
    auto *line = new QHBoxLayout(button);
    line->setContentsMargins(16, 12, 14, 12);
    line->setSpacing(17);
    auto *symbol = new QLabel(button);
    symbol->setFixedSize(35, 35);
    symbol->setPixmap(HardwareUi::symbol(categoryIcon(device.category), palette(), 30));
    line->addWidget(symbol);
    auto *text = new QVBoxLayout;
    text->setSpacing(5);
    auto *nameLabel = HardwareUi::heading(device.name, button, 2);
    nameLabel->setWordWrap(true);
    text->addWidget(nameLabel);
    QStringList details;
    if (!device.driver.isEmpty()) details.append(tr("Driver: %1").arg(device.driver));
    QStringList installed;
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (profile && profile->source == ProfileSource::Chwd && profile->installed)
            installed.append(HardwareUi::translatedName(*profile));
    }
    if (!installed.isEmpty()) details.append(tr("CHWD: %1").arg(installed.join(QStringLiteral(", "))));
    bool missing = false;
    bool serviceIssue = false;
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (profile && profile->source == ProfileSource::Support) {
            missing |= !profile->missingPackages.isEmpty();
            serviceIssue |= !profile->inactiveServices.isEmpty() || !profile->disabledServices.isEmpty();
        }
    }
    if (missing) details.append(tr("Missing support components"));
    else if (serviceIssue) details.append(tr("Service needs attention"));
    else if (!device.driver.isEmpty()) details.append(tr("Driver bound; function not tested"));
    else details.append(tr("Detected; function not tested"));
    if (!details.isEmpty())
        text->addWidget(HardwareUi::muted(details.join(QStringLiteral(" · ")), button, true));
    line->addLayout(text, 1);
    arrow_ = new QLabel(button);
    arrow_->setFixedSize(24, 24);
    arrow_->setPixmap(HardwareUi::symbol(expanded ? QStringLiteral("chevron-down")
                                                  : QStringLiteral("chevron-right"), palette(), 23));
    line->addWidget(arrow_);
    layout->addWidget(button);

    auto *detail = new QWidget(this);
    auto p = detail->palette();
    p.setColor(QPalette::Window, p.color(QPalette::AlternateBase));
    detail->setPalette(p);
    detail->setAutoFillBackground(true);
    auto *content = new QVBoxLayout(detail);
    content->setContentsMargins(65, 12, 22, 17);
    content->setSpacing(2);
    QStringList identifiers;
    if (!device.address.isEmpty()) identifiers.append(device.bus.toUpper() + QLatin1Char(' ') + device.address);
    if (!device.vendor.isEmpty()) identifiers.append(device.vendor);
    if (!identifiers.isEmpty()) content->addWidget(HardwareUi::muted(identifiers.join(QStringLiteral(" · ")), detail, true));
    for (const auto &id : device.profileIds) {
        const auto *profile = snapshot.profile(id);
        if (!profile) continue;
        auto *row = new ProfileRow(*profile, selection, device.recommendedProfile == id, detail);
        connect(row, &ProfileRow::profileToggled, this, &DeviceRow::profileToggled);
        content->addWidget(row);
    }
    if (device.profileIds.isEmpty())
        content->addWidget(HardwareUi::muted(tr("No additional profile"), detail));
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
    const QString executable = QStandardPaths::findExecutable(program);
    if (!program.isEmpty() && !executable.isEmpty()) {
        auto *test = new QPushButton(label, detail);
        connect(test, &QPushButton::clicked, this, [this, executable, arguments] {
            if (!QProcess::startDetached(executable, arguments))
                QMessageBox::warning(this, tr("Hardware Manager"), tr("Could not open the device tool."));
        });
        content->addWidget(test, 0, Qt::AlignLeft);
    }
    detail->setVisible(expanded);
    layout->addWidget(detail);
    connect(button, &QPushButton::clicked, this,
            [this, detail, id = device.id, expanded]() mutable {
                expanded = !expanded;
                detail->setVisible(expanded);
                arrow_->setPixmap(HardwareUi::symbol(expanded
                    ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"),
                    palette(), 23));
                emit expansionChanged(id, expanded);
            });
}
