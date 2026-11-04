#pragma once
#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

class ProfileRow : public QWidget {
    Q_OBJECT
public:
    ProfileRow(const HardwareProfile &profile, const ProfileSelection &selection,
               bool recommended, QWidget *parent = nullptr);
signals:
    void profileToggled(const QString &id, bool checked);
};
