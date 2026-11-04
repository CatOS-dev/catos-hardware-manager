#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QWidget>

class QLineEdit;
class QVBoxLayout;
class QScrollArea;

class ProfileCatalogPage : public QWidget {
    Q_OBJECT
public:
    explicit ProfileCatalogPage(QWidget *parent = nullptr);
    void updateState(const HardwareSnapshot &snapshot, const ProfileSelection &selection);
signals:
    void profileToggled(const QString &id, bool checked);
private:
    void rebuild();
    HardwareSnapshot snapshot_;
    ProfileSelection selection_;
    QLineEdit *search_ = nullptr;
    QVBoxLayout *content_ = nullptr;
    QScrollArea *scroll_ = nullptr;
};
