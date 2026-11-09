#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QHash>
#include <QWidget>

class QVBoxLayout;
class QScrollArea;
class ProfileRow;

class InstalledProfilesPage : public QWidget {
    Q_OBJECT
public:
    explicit InstalledProfilesPage(QWidget *parent = nullptr);
    void updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
    ProfileRow *row(const QString &profileId) const { return rows_.value(profileId); }
signals:
    void profileToggled(const QString &id, bool checked);
    void serviceStopToggled(const QString &id, bool staged);
private:
    void rebuild(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
    QString structure_;
    QHash<QString, ProfileRow *> rows_;
    QVBoxLayout *content_ = nullptr;
    QScrollArea *scroll_ = nullptr;
};
