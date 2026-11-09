#pragma once
#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;
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
    QPushButton *stopButton() const { return stop_; }
    QString stateText() const;
signals:
    void profileToggled(const QString &id, bool checked);
    void serviceStopToggled(const QString &id, bool staged);
private:
    void fillDetails(const HardwareProfile &profile);
    QString profileId_;
    QLabel *name_ = nullptr;
    QLabel *description_ = nullptr;
    StatusLabel *state_ = nullptr;
    QCheckBox *toggle_ = nullptr;
    QPushButton *stop_ = nullptr;
    bool stopStaged_ = false;
    QToolButton *detailsButton_ = nullptr;
    QWidget *details_ = nullptr;
    QVBoxLayout *detailsLayout_ = nullptr;
};
