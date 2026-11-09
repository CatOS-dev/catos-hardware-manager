#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QHash>
#include <QSet>
#include <QWidget>

class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class DeviceRow;
class NoticeBanner;

class DevicesPage : public QWidget {
    Q_OBJECT
public:
    explicit DevicesPage(QWidget *parent = nullptr);
    void updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
    void setRefreshing(bool refreshing);
    DeviceRow *row(const QString &deviceId) const { return rows_.value(deviceId); }
    NoticeBanner *banner() const { return banner_; }
signals:
    void refreshRequested();
    void reportRequested();
    void recommendedRequested();
    void profileToggled(const QString &id, bool checked);
    void serviceStopToggled(const QString &id, bool staged);
protected:
    void changeEvent(QEvent *event) override;
private:
    void rebuild();
    QString structureKey() const;
    HardwareSnapshot snapshot_;
    ProfileSelection selection_;
    QString structure_;
    QSet<DeviceCategory> expandedCategories_;
    QSet<QString> expandedDevices_;
    QHash<QString, DeviceRow *> rows_;
    NoticeBanner *banner_ = nullptr;
    QPushButton *refresh_ = nullptr;
    QLabel *refreshing_ = nullptr;
    QVBoxLayout *content_ = nullptr;
    QScrollArea *scroll_ = nullptr;
};
