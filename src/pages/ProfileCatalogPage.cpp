#include "ProfileCatalogPage.h"

#include "widgets/HardwareUi.h"
#include "widgets/ProfileRow.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

ProfileCatalogPage::ProfileCatalogPage(QWidget *parent) : QWidget(parent)
{
    auto *page = new QVBoxLayout(this);
    page->setContentsMargins(30, 28, 30, 0);
    page->setSpacing(8);
    page->addWidget(HardwareUi::header(tr("Profile catalog"), nullptr, this));
    search_ = new QLineEdit(this);
    search_->setPlaceholderText(tr("Search profiles"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumHeight(38);
    connect(search_, &QLineEdit::textChanged, this, [this] { rebuild(); });
    page->addWidget(search_);
    page->addSpacing(8);
    scroll_ = new QScrollArea(this);
    auto *scroll = scroll_;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *widget = new QWidget(scroll);
    content_ = new QVBoxLayout(widget);
    content_->setContentsMargins(0, 10, 0, 28);
    content_->setSpacing(9);
    scroll->setWidget(widget);
    page->addWidget(scroll, 1);
}

void ProfileCatalogPage::updateState(const HardwareSnapshot &snapshot,
                                     const ProfileSelection &selection)
{
    snapshot_ = snapshot;
    selection_ = selection;
    rebuild();
}

void ProfileCatalogPage::rebuild()
{
    const int previous = scroll_->verticalScrollBar()->value();
    HardwareUi::clear(content_);
    const QString filter = search_->text().trimmed();
    int visible = 0;
    const QVector<DeviceCategory> categories{
        DeviceCategory::Graphics, DeviceCategory::Network, DeviceCategory::Audio,
        DeviceCategory::Bluetooth, DeviceCategory::Camera, DeviceCategory::Printer,
        DeviceCategory::Scanner, DeviceCategory::Tablet, DeviceCategory::Firmware,
        DeviceCategory::Thunderbolt, DeviceCategory::Controller, DeviceCategory::MobileBroadband,
        DeviceCategory::SmartCard, DeviceCategory::Other
    };
    for (const auto category : categories) {
        QVector<const HardwareProfile *> profiles;
        for (const auto &profile : snapshot_.profiles) {
            if (profile.category != category) continue;
            if (!filter.isEmpty() && !profile.id.contains(filter, Qt::CaseInsensitive)
                && !HardwareUi::translatedName(profile).contains(filter, Qt::CaseInsensitive))
                continue;
            profiles.append(&profile);
        }
        if (profiles.isEmpty()) continue;
        content_->addWidget(HardwareUi::heading(categoryTitle(category), this, 2));
        HardwareUi::separator(content_, this);
        for (const auto *profile : profiles) {
            auto *row = new ProfileRow(*profile, selection_, false, this);
            connect(row, &ProfileRow::profileToggled, this, &ProfileCatalogPage::profileToggled);
            content_->addWidget(row);
            ++visible;
        }
        content_->addSpacing(13);
    }
    if (visible == 0)
        content_->addWidget(HardwareUi::muted(tr("No matching profiles"), this));
    content_->addStretch();
    QTimer::singleShot(0, scroll_, [this, previous] {
        scroll_->verticalScrollBar()->setValue(previous);
    });
}
