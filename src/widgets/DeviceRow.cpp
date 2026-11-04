#include "DeviceRow.h"

#include "HardwareUi.h"
#include "ProfileRow.h"

#include <QFileInfo>
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
