#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QSet>
#include <QWidget>

class QVBoxLayout;
class QScrollArea;

class DevicesPage : public QWidget {
    Q_OBJECT
public:
    explicit DevicesPage(QWidget *parent = nullptr);
    void updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
signals:
    void refreshRequested();
    void reportRequested();
    void recommendedRequested();
    void profileToggled(const QString &id, bool checked);
private:
    void rebuild();
    HardwareSnapshot snapshot_;
    ProfileSelection selection_;
    QSet<DeviceCategory> expandedCategories_;
    QSet<QString> expandedDevices_;
    QVBoxLayout *content_ = nullptr;
    QScrollArea *scroll_ = nullptr;
};
