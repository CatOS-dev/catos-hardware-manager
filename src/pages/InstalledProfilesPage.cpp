#include "InstalledProfilesPage.h"

#include "widgets/HardwareUi.h"
#include "widgets/ProfileRow.h"

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QVector<const HardwareProfile *> installed(const HardwareSnapshot &snapshot, ProfileSource source)
{
    QVector<const HardwareProfile *> profiles;
    for (const auto &profile : snapshot.profiles)
        if (profile.installed && profile.source == source) profiles.append(&profile);
    return profiles;
}
}

InstalledProfilesPage::InstalledProfilesPage(QWidget *parent) : QWidget(parent)
{
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(30, 28, 30, 0);
    main->addWidget(HardwareUi::header(tr("Installed profiles"), nullptr, this));
    scroll_ = new QScrollArea(this);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setWidgetResizable(true);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *widget = new QWidget(scroll_);
    content_ = new QVBoxLayout(widget);
    content_->setContentsMargins(0, 5, 0, 28);
    content_->setSpacing(9);
    scroll_->setWidget(widget);
    main->addWidget(scroll_, 1);
}

void InstalledProfilesPage::updateState(const HardwareSnapshot &snapshot,
                                        const ProfileSelection &selection)
{
    QStringList ids;
    for (const auto source : {ProfileSource::Chwd, ProfileSource::Support})
        for (const auto *profile : installed(snapshot, source)) ids.append(profile->id);
    const QString structure = ids.join(QLatin1Char('\n'));
    if (structure == structure_ && !rows_.isEmpty()) {
        for (const auto &profile : snapshot.profiles)
            if (auto *row = rows_.value(profile.id)) row->updateRow(profile, selection, false);
        return;
    }
    structure_ = structure;
    rebuild(snapshot, selection);
}

void InstalledProfilesPage::rebuild(const HardwareSnapshot &snapshot, const ProfileSelection &selection)
{
    const int previous = scroll_->verticalScrollBar()->value();
    const QString focused = HardwareUi::focusedRowKey();
    rows_.clear();
    HardwareUi::clear(content_);
    for (const auto source : {ProfileSource::Chwd, ProfileSource::Support}) {
        const auto profiles = installed(snapshot, source);
        if (profiles.isEmpty()) continue;
        content_->addWidget(HardwareUi::heading(source == ProfileSource::Chwd
            ? tr("CHWD configurations") : tr("System support"), this, 2));
        HardwareUi::separator(content_, this);
        for (const auto *profile : profiles) {
            auto *row = new ProfileRow(*profile, selection, false, this);
            connect(row, &ProfileRow::profileToggled, this, &InstalledProfilesPage::profileToggled);
            rows_.insert(profile->id, row);
            content_->addWidget(row);
        }
        content_->addSpacing(16);
    }
    if (rows_.isEmpty()) content_->addWidget(HardwareUi::muted(tr("No installed profiles"), this));
    content_->addStretch();
    if (auto *row = rows_.value(focused); row && !row->toggle()->isHidden()) row->toggle()->setFocus();
    QTimer::singleShot(0, scroll_, [this, previous] {
        scroll_->verticalScrollBar()->setValue(previous);
    });
}
