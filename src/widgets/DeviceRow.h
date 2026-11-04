#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

class QLabel;

class DeviceRow : public QWidget {
    Q_OBJECT
public:
    DeviceRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
              const ProfileSelection &selection, bool expanded, QWidget *parent = nullptr);
signals:
    void profileToggled(const QString &id, bool checked);
    void expansionChanged(const QString &deviceId, bool expanded);
private:
    QLabel *arrow_ = nullptr;
};
