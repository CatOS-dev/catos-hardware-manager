#include "ProfileRow.h"

#include "HardwareUi.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

ProfileRow::ProfileRow(const HardwareProfile &profile, const ProfileSelection &selection,
                       bool recommended, QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    auto *summary = new QWidget(this);
    auto *line = new QHBoxLayout(summary);
    line->setContentsMargins(7, 9, 7, 9);
    line->setSpacing(12);
    auto *text = new QVBoxLayout;
    text->setSpacing(5);
    QString name = HardwareUi::translatedName(profile);
    if (recommended) name += tr("  · Recommended");
    text->addWidget(HardwareUi::heading(name, summary));
    const QString description = HardwareUi::translatedDescription(profile);
    if (!description.isEmpty())
        text->addWidget(HardwareUi::muted(description, summary, true));
    QString state;
    if (profile.source == ProfileSource::Support) {
        if (!profile.applicable) state = tr("Not applicable");
        else if (profile.installed) state = tr("System support ready");
        else if (selection.selected(profile.id)) state = tr("Pending installation");
        else state = tr("Missing components");
    } else {
        if (profile.installed) state = tr("Installed");
        else if (!profile.applicable) state = tr("Not applicable");
    }
    if (!state.isEmpty()) text->addWidget(HardwareUi::muted(state, summary));
    line->addLayout(text, 1);

    // A disabled checkbox suggests a broken button. Only render a checkbox
    // when the profile can actually be staged for a change.
    const bool canToggle = (profile.applicable || profile.installed)
        && !(profile.systemManaged && profile.installed);
    if (canToggle) {
        auto *toggle = new QCheckBox(summary);
        toggle->setAccessibleName(name);
        toggle->setChecked(selection.selected(profile.id));
        connect(toggle, &QCheckBox::toggled, this,
                [this, id = profile.id](bool checked) { emit profileToggled(id, checked); });
        line->addWidget(toggle);
    }

    if (!profile.packages.isEmpty() || !profile.services.isEmpty()) {
        auto *detailsButton = new QToolButton(summary);
        detailsButton->setArrowType(Qt::RightArrow);
        detailsButton->setToolTip(tr("Profile details"));
        detailsButton->setAccessibleName(tr("Profile details: %1").arg(name));
        detailsButton->setCheckable(true);
        detailsButton->setAutoRaise(true);
        line->addWidget(detailsButton);

        auto *details = new QWidget(this);
        auto *detailsLayout = new QVBoxLayout(details);
        detailsLayout->setContentsMargins(17, 3, 16, 13);
        detailsLayout->setSpacing(6);
        if (!profile.packages.isEmpty()) {
            detailsLayout->addWidget(HardwareUi::heading(tr("Packages"), details));
            detailsLayout->addWidget(
                HardwareUi::muted(profile.packages.join(QStringLiteral(", ")), details, true));
        }
        if (!profile.services.isEmpty()) {
            detailsLayout->addWidget(HardwareUi::heading(tr("Services"), details));
            detailsLayout->addWidget(
                HardwareUi::muted(profile.services.join(QStringLiteral(", ")), details, true));
        }
        if (profile.source == ProfileSource::Support && profile.applicable) {
            if (!profile.missingPackages.isEmpty()) {
                detailsLayout->addWidget(HardwareUi::heading(tr("Missing packages"), details));
                detailsLayout->addWidget(HardwareUi::muted(
                    profile.missingPackages.join(QStringLiteral(", ")), details, true));
            }
            if (!profile.inactiveServices.isEmpty()) {
                detailsLayout->addWidget(HardwareUi::heading(tr("Inactive services"), details));
                detailsLayout->addWidget(HardwareUi::muted(
                    profile.inactiveServices.join(QStringLiteral(", ")), details, true));
            }
        }
        details->hide();
        outer->addWidget(summary);
        outer->addWidget(details);
        connect(detailsButton, &QToolButton::toggled, this,
                [detailsButton, details](bool expanded) {
                    details->setVisible(expanded);
                    detailsButton->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
                });
    } else {
        outer->addWidget(summary);
    }
}
