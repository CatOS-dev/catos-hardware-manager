#pragma once
#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QToolButton;
class QVBoxLayout;
class StatusLabel;

class ProfileRow : public QWidget {
    Q_OBJECT
public:
    ProfileRow(const HardwareProfile &profile, const ProfileSelection &selection,
               bool recommended, QWidget *parent = nullptr);
    // Refresh texts and the checkbox without recreating widgets, so focus
    // and screen reader context stay on the row.
    void updateRow(const HardwareProfile &profile, const ProfileSelection &selection, bool recommended);
    QString profileId() const { return profileId_; }
    QCheckBox *toggle() const { return toggle_; }
    QString stateText() const;
signals:
    void profileToggled(const QString &id, bool checked);
private:
    void fillDetails(const HardwareProfile &profile);
    QString profileId_;
    QLabel *name_ = nullptr;
    QLabel *description_ = nullptr;
    StatusLabel *state_ = nullptr;
    QCheckBox *toggle_ = nullptr;
    QToolButton *detailsButton_ = nullptr;
    QWidget *details_ = nullptr;
    QVBoxLayout *detailsLayout_ = nullptr;
};
