#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

class QVBoxLayout;
class QScrollArea;

class InstalledProfilesPage : public QWidget {
    Q_OBJECT
public:
    explicit InstalledProfilesPage(QWidget *parent = nullptr);
    void updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
signals:
    void profileToggled(const QString &id, bool checked);
private:
    QVBoxLayout *content_ = nullptr;
    QScrollArea *scroll_ = nullptr;
};
