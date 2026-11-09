#include "ProfileRow.h"

#include "HardwareUi.h"
#include "Presentation.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

ProfileRow::ProfileRow(const HardwareProfile &profile, const ProfileSelection &selection,
                       bool recommended, QWidget *parent)
    : QWidget(parent), profileId_(profile.id)
{
    setProperty("rowKey", profile.id);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    auto *summary = new QWidget(this);
    auto *line = new QHBoxLayout(summary);
    line->setContentsMargins(7, 9, 7, 9);
    line->setSpacing(12);
    auto *text = new QVBoxLayout;
    text->setSpacing(5);
    name_ = HardwareUi::heading(QString(), summary);
    name_->setWordWrap(true);
    text->addWidget(name_);
    description_ = HardwareUi::muted(QString(), summary, true);
    text->addWidget(description_);
    state_ = new StatusLabel(summary);
    text->addWidget(state_);
    line->addLayout(text, 1);

    toggle_ = new QCheckBox(summary);
    connect(toggle_, &QCheckBox::toggled, this,
            [this](bool checked) { emit profileToggled(profileId_, checked); });
    line->addWidget(toggle_, 0, Qt::AlignTop);
    stop_ = new QPushButton(summary);
    stop_->setFlat(true);
    connect(stop_, &QPushButton::clicked, this, [this] { emit serviceStopToggled(profileId_, !stopStaged_); });
    line->addWidget(stop_, 0, Qt::AlignTop);

    details_ = new QWidget(this);
    detailsLayout_ = new QVBoxLayout(details_);
    detailsLayout_->setContentsMargins(17, 3, 16, 13);
    detailsLayout_->setSpacing(6);
    details_->hide();
    if (!profile.packages.isEmpty() || !profile.services.isEmpty()) {
        detailsButton_ = new QToolButton(summary);
        detailsButton_->setArrowType(Qt::RightArrow);
        detailsButton_->setToolTip(tr("Profile details"));
        detailsButton_->setCheckable(true);
        detailsButton_->setAutoRaise(true);
        detailsButton_->setAccessibleDescription(tr("Collapsed"));
        line->addWidget(detailsButton_, 0, Qt::AlignTop);
        connect(detailsButton_, &QToolButton::toggled, this, [this](bool expanded) {
            details_->setVisible(expanded);
            detailsButton_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
            detailsButton_->setAccessibleDescription(expanded ? tr("Expanded") : tr("Collapsed"));
        });
    }
    outer->addWidget(summary);
    outer->addWidget(details_);
    updateRow(profile, selection, recommended);
}

void ProfileRow::updateRow(const HardwareProfile &profile, const ProfileSelection &selection, bool recommended)
{
    if (profile.id != profileId_) return;
    const QString name = Presentation::profileName(profile, recommended);
    name_->setText(name);
    const QString description = HardwareUi::translatedDescription(profile);
    description_->setText(description);
    description_->setVisible(!description.isEmpty());
    const StatusText state = Presentation::profileState(profile, selection);
    state_->setStatus(state);
    {
        const QSignalBlocker blocker(toggle_);
        toggle_->setChecked(selection.selected(profile.id));
    }
    toggle_->setVisible(Presentation::profileToggleable(profile));
    stopStaged_ = selection.serviceDisableStaged(profile.id);
    stop_->setVisible(ProfileSelection::canDisableServices(profile));
    stop_->setText(stopStaged_ ? tr("Keep services running") : tr("Stop services"));
    stop_->setToolTip(stopStaged_ ? QString()
        : tr("Stop and disable %1. Packages stay installed.").arg(profile.services.join(QStringLiteral(", "))));
    stop_->setAccessibleName(stop_->text() + QStringLiteral(": ") + name);
    toggle_->setAccessibleName(name);
    toggle_->setAccessibleDescription(state.text);
    if (detailsButton_) detailsButton_->setAccessibleName(tr("Profile details: %1").arg(name));
    fillDetails(profile);
}

QString ProfileRow::stateText() const
{
    return state_->text();
}

void ProfileRow::fillDetails(const HardwareProfile &profile)
{
    HardwareUi::clear(detailsLayout_);
    const auto section = [this](const QString &title, const QStringList &items) {
        if (items.isEmpty()) return;
        detailsLayout_->addWidget(HardwareUi::heading(title, details_));
        detailsLayout_->addWidget(HardwareUi::muted(items.join(QStringLiteral(", ")), details_, true));
    };
    section(tr("Packages"), profile.packages);
    section(tr("Services"), profile.services);
    if (profile.source == ProfileSource::Support && profile.applicable) {
        section(tr("Missing packages"), profile.missingPackages);
        section(tr("Services not enabled"), profile.disabledServices);
        section(tr("Inactive services"), profile.inactiveServices);
    }
}
