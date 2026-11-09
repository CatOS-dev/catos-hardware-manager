#pragma once

#include "model/HardwareTypes.h"
#include "model/ProfileSelection.h"

#include <QHash>
#include <QWidget>

class QLabel;
class QPushButton;
class ProfileRow;
class StatusLabel;
class SymbolLabel;

class DeviceRow : public QWidget {
    Q_OBJECT
public:
    DeviceRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
              const ProfileSelection &selection, bool expanded, QWidget *parent = nullptr);
    // In-place refresh for the same device with the same profiles.
    void updateRow(const HardwareDevice &device, const HardwareSnapshot &snapshot,
                   const ProfileSelection &selection);
    QString deviceId() const { return deviceId_; }
    QPushButton *header() const { return header_; }
    StatusLabel *status() const { return status_; }
    QString summaryText() const;
    QList<ProfileRow *> profileRows() const { return profiles_.values(); }
signals:
    void profileToggled(const QString &id, bool checked);
    void expansionChanged(const QString &deviceId, bool expanded);
private:
    void setExpanded(bool expanded);
    QString deviceId_;
    QPushButton *header_ = nullptr;
    SymbolLabel *icon_ = nullptr;
    SymbolLabel *arrow_ = nullptr;
    QLabel *name_ = nullptr;
    StatusLabel *status_ = nullptr;
    QLabel *summary_ = nullptr;
    QWidget *detail_ = nullptr;
    QLabel *identifiers_ = nullptr;
    QLabel *untested_ = nullptr;
    QHash<QString, ProfileRow *> profiles_;
};
