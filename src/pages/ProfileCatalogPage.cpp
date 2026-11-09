#include "ProfileCatalogPage.h"

#include "widgets/HardwareUi.h"
#include "widgets/ProfileRow.h"

#include <QCheckBox>
#include <algorithm>
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
    search_->setAccessibleName(tr("Search profiles"));
    connect(search_, &QLineEdit::textChanged, this, [this] {
        structure_ = structureKey();
        rebuild();
    });
    page->addWidget(search_);
    page->addSpacing(8);
    scroll_ = new QScrollArea(this);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setWidgetResizable(true);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *widget = new QWidget(scroll_);
    content_ = new QVBoxLayout(widget);
    content_->setContentsMargins(0, 10, 0, 28);
    content_->setSpacing(9);
    scroll_->setWidget(widget);
    page->addWidget(scroll_, 1);
}

QVector<const HardwareProfile *> ProfileCatalogPage::visibleProfiles(DeviceCategory category) const
{
    const QString filter = search_->text().trimmed();
    QVector<const HardwareProfile *> profiles;
    for (const auto &profile : snapshot_.profiles) {
        if (profile.category != category) continue;
        if (!filter.isEmpty()) {
            const QStringList haystack = QStringList{profile.id, profile.name, profile.chineseName,
                profile.description, profile.chineseDescription} + profile.packages;
            if (!std::any_of(haystack.cbegin(), haystack.cend(), [&filter](const QString &text) {
                    return text.contains(filter, Qt::CaseInsensitive);
                }))
                continue;
        }
        profiles.append(&profile);
    }
    return profiles;
}

QString ProfileCatalogPage::structureKey() const
{
    QStringList ids;
    for (const auto category : HardwareUi::categoryOrder())
        for (const auto *profile : visibleProfiles(category)) ids.append(profile->id);
    return ids.join(QLatin1Char('\n'));
}

void ProfileCatalogPage::updateState(const HardwareSnapshot &snapshot,
                                     const ProfileSelection &selection)
{
    snapshot_ = snapshot;
    selection_ = selection;
    const QString structure = structureKey();
    if (structure == structure_ && !rows_.isEmpty()) {
        for (const auto &profile : snapshot_.profiles)
            if (auto *row = rows_.value(profile.id)) row->updateRow(profile, selection_, false);
        return;
    }
    structure_ = structure;
    rebuild();
}

void ProfileCatalogPage::rebuild()
{
    const int previous = scroll_->verticalScrollBar()->value();
    const QString focused = HardwareUi::focusedRowKey();
    rows_.clear();
    HardwareUi::clear(content_);
    for (const auto category : HardwareUi::categoryOrder()) {
        const auto profiles = visibleProfiles(category);
        if (profiles.isEmpty()) continue;
        content_->addWidget(HardwareUi::heading(categoryTitle(category), this, 2));
        HardwareUi::separator(content_, this);
        for (const auto *profile : profiles) {
            auto *row = new ProfileRow(*profile, selection_, false, this);
            connect(row, &ProfileRow::profileToggled, this, &ProfileCatalogPage::profileToggled);
            connect(row, &ProfileRow::serviceStopToggled, this, &ProfileCatalogPage::serviceStopToggled);
            rows_.insert(profile->id, row);
            content_->addWidget(row);
        }
        content_->addSpacing(13);
    }
    if (rows_.isEmpty())
        content_->addWidget(HardwareUi::muted(tr("No matching profiles"), this));
    content_->addStretch();
    if (auto *row = rows_.value(focused); row && !row->toggle()->isHidden()) row->toggle()->setFocus();
    QTimer::singleShot(0, scroll_, [this, previous] {
        scroll_->verticalScrollBar()->setValue(previous);
    });
}
