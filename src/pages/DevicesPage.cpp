#include "DevicesPage.h"

#include "widgets/DeviceRow.h"
#include "widgets/HardwareUi.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QScrollBar>
#include <QVBoxLayout>

DevicesPage::DevicesPage(QWidget *parent) : QWidget(parent)
{
    expandedCategories_ = {DeviceCategory::Graphics, DeviceCategory::Network,
                           DeviceCategory::Audio, DeviceCategory::Bluetooth,
                           DeviceCategory::Camera};
    auto *page = new QVBoxLayout(this);
    page->setContentsMargins(30, 28, 30, 0);
    page->setSpacing(0);
    auto *heading = new QHBoxLayout;
    heading->addWidget(HardwareUi::heading(tr("Hardware devices"), this, 10));
    heading->addStretch();
    auto *recommended = new QPushButton(tr("Select recommended"), this);
    recommended->setFlat(true);
    recommended->setMinimumHeight(38);
    connect(recommended, &QPushButton::clicked, this, &DevicesPage::recommendedRequested);
    heading->addWidget(recommended);
    auto *refresh = new QPushButton(this);
    refresh->setIcon(QIcon(HardwareUi::symbol(QStringLiteral("refresh"), palette())));
    refresh->setIconSize(QSize(23, 23));
    refresh->setFixedSize(38, 38);
    refresh->setFlat(true);
    refresh->setToolTip(tr("Refresh"));
    refresh->setAccessibleName(tr("Refresh"));
    connect(refresh, &QPushButton::clicked, this, &DevicesPage::refreshRequested);
    heading->addWidget(refresh);
    auto *report = new QPushButton(tr("Diagnostic report"), this);
    connect(report, &QPushButton::clicked, this, &DevicesPage::reportRequested);
    heading->addWidget(report);
    page->addLayout(heading);
    page->addSpacing(22);

    scroll_ = new QScrollArea(this);
    auto *scroll = scroll_;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *container = new QWidget(scroll);
    content_ = new QVBoxLayout(container);
    content_->setContentsMargins(0, 8, 0, 30);
    content_->setSpacing(17);
    scroll->setWidget(container);
    page->addWidget(scroll, 1);
}

void DevicesPage::updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection)
{
    snapshot_ = snapshot;
    selection_ = selection;
    rebuild();
}

void DevicesPage::rebuild()
{
    const int previous = scroll_->verticalScrollBar()->value();
    HardwareUi::clear(content_);
    if (!snapshot_.error.isEmpty())
        content_->addWidget(HardwareUi::muted(tr("Hardware query failed: %1").arg(snapshot_.error), this, true));
    for (const auto &warning : snapshot_.warnings)
        content_->addWidget(HardwareUi::muted(warning, this, true));
    const auto *scanner = snapshot_.profile(QStringLiteral("support:scanner"));
    if (scanner && !scanner->ready) {
        auto *setup = new QPushButton(tr("Set up scanner detection"), this);
        connect(setup, &QPushButton::clicked, this, [this] {
            emit profileToggled(QStringLiteral("support:scanner"), true);
        });
        content_->addWidget(setup);
    }
    const QVector<DeviceCategory> categories{
        DeviceCategory::Graphics, DeviceCategory::Network, DeviceCategory::Audio,
        DeviceCategory::Bluetooth, DeviceCategory::Camera, DeviceCategory::Printer,
        DeviceCategory::Scanner, DeviceCategory::Tablet,
        DeviceCategory::Thunderbolt, DeviceCategory::Controller, DeviceCategory::MobileBroadband,
        DeviceCategory::SmartCard, DeviceCategory::Other
    };
    for (const auto category : categories) {
        QVector<HardwareDevice> devices;
        for (const auto &device : snapshot_.devices) {
            if (device.category == category) devices.append(device);
        }
        if (devices.isEmpty()) continue;
        auto *section = new QWidget(this);
        auto *layout = new QVBoxLayout(section);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(3);
        auto *button = new QPushButton(section);
        button->setFlat(true);
        button->setMinimumHeight(60);
        button->setCursor(Qt::PointingHandCursor);
        button->setAccessibleName(categoryTitle(category));
        button->setStyleSheet(QStringLiteral(
            "QPushButton { border: 0; background: transparent; border-radius: 9px; }"
            "QPushButton:hover { background: palette(alternate-base); }"
            "QPushButton:focus { border: 1px solid palette(highlight); }"));
        auto *row = new QHBoxLayout(button);
        row->setContentsMargins(10, 8, 12, 8);
        row->setSpacing(13);
        auto *icon = new QLabel(button);
        icon->setPixmap(HardwareUi::symbol(categoryIcon(category), palette(), 27));
        row->addWidget(icon);
        row->addWidget(HardwareUi::heading(categoryTitle(category), button, 3));
        row->addStretch();
        row->addWidget(HardwareUi::muted(QString::number(devices.size()), button));
        auto *arrow = new QLabel(button);
        arrow->setPixmap(HardwareUi::symbol(
            expandedCategories_.contains(category) ? QStringLiteral("chevron-down")
                                                   : QStringLiteral("chevron-right"), palette(), 21));
        row->addWidget(arrow);
        layout->addWidget(button);
        HardwareUi::separator(layout, section);

        auto *list = new QWidget(section);
        auto *listLayout = new QVBoxLayout(list);
        listLayout->setContentsMargins(0, 3, 0, 0);
        listLayout->setSpacing(0);
        for (const auto &device : devices) {
            auto *item = new DeviceRow(device, snapshot_, selection_,
                                       expandedDevices_.contains(device.id), list);
            connect(item, &DeviceRow::profileToggled, this, &DevicesPage::profileToggled);
            connect(item, &DeviceRow::expansionChanged, this, [this](const QString &id, bool value) {
                if (value) expandedDevices_.insert(id);
                else expandedDevices_.remove(id);
            });
            listLayout->addWidget(item);
        }
        list->setVisible(expandedCategories_.contains(category));
        layout->addWidget(list);
        connect(button, &QPushButton::clicked, this, [this, category, list, arrow] {
            const bool expanded = !expandedCategories_.contains(category);
            if (expanded) expandedCategories_.insert(category);
            else expandedCategories_.remove(category);
            list->setVisible(expanded);
            arrow->setPixmap(HardwareUi::symbol(
                expanded ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"),
                palette(), 21));
        });
        content_->addWidget(section);
    }
    if (snapshot_.devices.isEmpty() && snapshot_.error.isEmpty())
        content_->addWidget(HardwareUi::muted(tr("No detected devices"), this));
    content_->addStretch();
    QTimer::singleShot(0, scroll_, [this, previous] {
        scroll_->verticalScrollBar()->setValue(previous);
    });
}
