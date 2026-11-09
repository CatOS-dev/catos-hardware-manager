#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QHash>
#include <QWidget>

class QLineEdit;
class QVBoxLayout;
class QScrollArea;
class ProfileRow;

class ProfileCatalogPage : public QWidget {
    Q_OBJECT
public:
    explicit ProfileCatalogPage(QWidget *parent = nullptr);
    void updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
    QLineEdit *search() const { return search_; }
    ProfileRow *row(const QString &profileId) const { return rows_.value(profileId); }
signals:
    void profileToggled(const QString &id, bool checked);
    void serviceStopToggled(const QString &id, bool staged);
private:
    QVector<const HardwareProfile *> visibleProfiles(DeviceCategory category) const;
    QString structureKey() const;
    void rebuild();
    HardwareSnapshot snapshot_;
    ProfileSelection selection_;
    QString structure_;
    QHash<QString, ProfileRow *> rows_;
    QLineEdit *search_ = nullptr;
    QVBoxLayout *content_ = nullptr;
    QScrollArea *scroll_ = nullptr;
};
