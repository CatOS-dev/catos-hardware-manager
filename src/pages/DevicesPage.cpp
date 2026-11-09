#include "DevicesPage.h"

#include "widgets/DeviceRow.h"
#include "widgets/HardwareUi.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
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
    refreshing_ = HardwareUi::muted(tr("Refreshing…"), this);
    refreshing_->hide();
    heading->addWidget(refreshing_);
    auto *recommended = new QPushButton(tr("Select recommended"), this);
    recommended->setFlat(true);
    recommended->setMinimumHeight(38);
    connect(recommended, &QPushButton::clicked, this, &DevicesPage::recommendedRequested);
    heading->addWidget(recommended);
    refresh_ = new QPushButton(this);
    refresh_->setIcon(QIcon(HardwareUi::symbol(QStringLiteral("refresh"), palette())));
    refresh_->setIconSize(QSize(23, 23));
    refresh_->setFixedSize(38, 38);
    refresh_->setFlat(true);
    refresh_->setToolTip(tr("Refresh (F5)"));
    refresh_->setAccessibleName(tr("Refresh"));
    connect(refresh_, &QPushButton::clicked, this, &DevicesPage::refreshRequested);
    heading->addWidget(refresh_);
    auto *report = new QPushButton(tr("Diagnostic report"), this);
    connect(report, &QPushButton::clicked, this, &DevicesPage::reportRequested);
    heading->addWidget(report);
    page->addLayout(heading);
    page->addSpacing(16);
    banner_ = new NoticeBanner(this);
    page->addWidget(banner_);
    page->addSpacing(6);

    scroll_ = new QScrollArea(this);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setWidgetResizable(true);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *container = new QWidget(scroll_);
    content_ = new QVBoxLayout(container);
    content_->setContentsMargins(0, 8, 0, 30);
    content_->setSpacing(17);
    scroll_->setWidget(container);
    page->addWidget(scroll_, 1);
}

void DevicesPage::setRefreshing(bool refreshing)
{
    refreshing_->setVisible(refreshing);
    refresh_->setEnabled(!refreshing);
}

QString DevicesPage::structureKey() const
{
    QStringList parts;
    for (const auto &device : snapshot_.devices)
        parts.append(QString::number(int(device.category)) + QLatin1Char('|') + device.id
                     + QLatin1Char('|') + device.profileIds.join(QLatin1Char(',')));
    return parts.join(QLatin1Char('\n'));
}

void DevicesPage::updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection)
{
    snapshot_ = snapshot;
    selection_ = selection;
    banner_->setNotices(snapshot_.error, snapshot_.warnings);
    const QString structure = structureKey();
    if (structure == structure_ && !rows_.isEmpty()) {
        for (const auto &device : snapshot_.devices)
            if (auto *row = rows_.value(device.id)) row->updateRow(device, snapshot_, selection_);
        return;
    }
    structure_ = structure;
    rebuild();
}

void DevicesPage::rebuild()
{
    const int previous = scroll_->verticalScrollBar()->value();
    const QString focused = HardwareUi::focusedRowKey();
    rows_.clear();
    HardwareUi::clear(content_);
    for (const auto category : HardwareUi::categoryOrder()) {
        QVector<HardwareDevice> devices;
        for (const auto &device : snapshot_.devices) {
            if (device.category == category) devices.append(device);
        }
        if (devices.isEmpty()) continue;
        auto *section = new QWidget(this);
        auto *layout = new QVBoxLayout(section);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(3);
        auto *button = new RowButton(section);
        button->setChecked(expandedCategories_.contains(category));
        button->setMinimumHeight(56);
        button->setAccessibleName(categoryTitle(category));
        button->setAccessibleDescription(tr("%n device(s)", nullptr, int(devices.size())));
        auto *row = new QHBoxLayout(button);
        row->setContentsMargins(10, 8, 12, 8);
        row->setSpacing(13);
        row->addWidget(new SymbolLabel(categoryIcon(category), 27, button));
        row->addWidget(HardwareUi::heading(categoryTitle(category), button, 3));
        row->addStretch();
        row->addWidget(HardwareUi::muted(QString::number(devices.size()), button));
        auto *arrow = new SymbolLabel(expandedCategories_.contains(category)
            ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"), 21, button);
        row->addWidget(arrow);
        for (auto *child : button->findChildren<QWidget *>())
            child->setAttribute(Qt::WA_TransparentForMouseEvents);
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
            rows_.insert(device.id, item);
            listLayout->addWidget(item);
        }
        list->setVisible(expandedCategories_.contains(category));
        layout->addWidget(list);
        connect(button, &RowButton::toggled, this, [this, category, list, arrow](bool expanded) {
            if (expanded) expandedCategories_.insert(category);
            else expandedCategories_.remove(category);
            list->setVisible(expanded);
            arrow->setSymbol(expanded ? QStringLiteral("chevron-down") : QStringLiteral("chevron-right"));
        });
        content_->addWidget(section);
    }
    if (snapshot_.devices.isEmpty() && snapshot_.error.isEmpty())
        content_->addWidget(HardwareUi::muted(tr("No detected devices"), this));
    content_->addStretch();
    if (auto *row = rows_.value(focused)) row->header()->setFocus();
    QTimer::singleShot(0, scroll_, [this, previous] {
        scroll_->verticalScrollBar()->setValue(previous);
    });
}

void DevicesPage::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::DevicePixelRatioChange)
        refresh_->setIcon(QIcon(HardwareUi::symbol(QStringLiteral("refresh"), palette().color(QPalette::WindowText),
                                                   23, devicePixelRatioF())));
}
