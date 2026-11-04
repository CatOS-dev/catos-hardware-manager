#include "InstalledProfilesPage.h"

#include "widgets/HardwareUi.h"
#include "widgets/ProfileRow.h"

#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

InstalledProfilesPage::InstalledProfilesPage(QWidget *parent) : QWidget(parent)
{
    auto *main = new QVBoxLayout(this);
    main->setContentsMargins(30, 28, 30, 0);
    main->addWidget(HardwareUi::header(tr("Installed profiles"), nullptr, this));
    scroll_ = new QScrollArea(this);
    auto *scroll = scroll_;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *widget = new QWidget(scroll);
    content_ = new QVBoxLayout(widget);
    content_->setContentsMargins(0, 5, 0, 28);
    content_->setSpacing(9);
    scroll->setWidget(widget);
    main->addWidget(scroll, 1);
}

void InstalledProfilesPage::updateState(const HardwareSnapshot &snapshot,
                                        const ProfileSelection &selection)
{
    const int previous = scroll_->verticalScrollBar()->value();
    HardwareUi::clear(content_);
    int count = 0;
    for (const auto source : {ProfileSource::Chwd, ProfileSource::Support}) {
        QVector<const HardwareProfile *> profiles;
        for (const auto &profile : snapshot.profiles) {
            if (profile.installed && profile.source == source)
                profiles.append(&profile);
        }
        if (profiles.isEmpty()) continue;
        content_->addWidget(HardwareUi::heading(source == ProfileSource::Chwd
            ? tr("CHWD configurations") : tr("System support"), this, 2));
        HardwareUi::separator(content_, this);
        for (const auto *profile : profiles) {
            auto *row = new ProfileRow(*profile, selection, false, this);
            connect(row, &ProfileRow::profileToggled, this, &InstalledProfilesPage::profileToggled);
            content_->addWidget(row);
            ++count;
        }
        content_->addSpacing(16);
    }
    if (count == 0) content_->addWidget(HardwareUi::muted(tr("No installed profiles"), this));
    content_->addStretch();
    QTimer::singleShot(0, scroll_, [this, previous] {
        scroll_->verticalScrollBar()->setValue(previous);
    });
}
